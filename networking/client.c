#include "sham.h"

uint32_t file_transfer_mode(int sockfd, struct sockaddr_in* server_addr, uint32_t seq, uint32_t ack, const char* input_file, const char* output_file_name);
void chat_mode(int sockfd, struct sockaddr_in* server_addr, uint32_t seq, uint32_t ack);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> [--chat | <input_file> <output_name>] [loss_rate]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setup_logging("client");

    // Argument parsing
    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int is_chat_mode = 0;
    const char* input_file = NULL;
    const char* output_file_name = NULL;
    if (strcmp(argv[3], "--chat") == 0) {
        is_chat_mode = 1;
    } else {
        if (argc < 5) {
             fprintf(stderr, "Usage: %s <server_ip> <port> <input_file> <output_name> [loss_rate]\n", argv[0]);
             exit(EXIT_FAILURE);
        }
        input_file = argv[3];
        output_file_name = argv[4];
    }

    int sockfd;
    struct sockaddr_in server_addr;
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket"); exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // --- 3-Way Handshake ---
    sham_packet_t packet;
    srand(time(NULL));
    uint32_t client_seq = rand();

    packet.header = (sham_header_t){htonl(client_seq), 0, SYN, 0};
    printf("SND SYN SEQ=%u\n", client_seq);
    log_event("SND SYN SEQ=%u", client_seq);
    sendto(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    recvfrom(sockfd, &packet.header, sizeof(packet.header), 0, NULL, NULL);
    uint32_t server_seq = ntohl(packet.header.seq_num);
    printf("RCV SYN-ACK SEQ=%u ACK=%u\n", server_seq, ntohl(packet.header.ack_num));
    log_event("RCV SYN-ACK SEQ=%u ACK=%u", server_seq, ntohl(packet.header.ack_num));

    client_seq++;
    
    packet.header = (sham_header_t){htonl(client_seq), htonl(server_seq + 1), ACK, 0};
    printf("SND ACK FOR SYN\n");
    log_event("SND ACK FOR SYN");
    sendto(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    printf("Connection Established.\n");

    if (is_chat_mode) {
        chat_mode(sockfd, &server_addr, client_seq, server_seq + 1);
    } else {
        client_seq = file_transfer_mode(sockfd, &server_addr, client_seq, server_seq + 1, input_file, output_file_name);
    }

    // --- 4-Way Handshake ---
    printf("Closing connection...\n");
    packet.header = (sham_header_t){htonl(client_seq), 0, FIN, 0};
    printf("SND FIN SEQ=%u\n", client_seq);
    log_event("SND FIN SEQ=%u", client_seq);
    sendto(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    recvfrom(sockfd, &packet.header, sizeof(packet.header), 0, NULL, NULL); // ACK
    printf("RCV ACK FOR FIN\n");
    log_event("RCV ACK FOR FIN");
    
    recvfrom(sockfd, &packet.header, sizeof(packet.header), 0, NULL, NULL); // FIN
    uint32_t server_fin_seq = ntohl(packet.header.seq_num);
    printf("RCV FIN SEQ=%u\n", server_fin_seq);
    log_event("RCV FIN SEQ=%u", server_fin_seq);

    packet.header = (sham_header_t){htonl(client_seq + 1), htonl(server_fin_seq + 1), ACK, 0};
    printf("SND ACK FOR FIN\n");
    log_event("SND ACK FOR FIN");
    sendto(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    printf("Connection Closed.\n");
    close(sockfd);
    if(log_fp) fclose(log_fp);
    return 0;
}

uint32_t file_transfer_mode(int sockfd, struct sockaddr_in* server_addr, uint32_t seq, uint32_t ack_base, const char* input_file, const char* output_file_name) {
    (void)ack_base; // Suppress unused parameter warning
    FILE *fp = fopen(input_file, "rb");
    if (!fp) { perror("fopen"); return seq; }

    struct timeval tv = { .tv_sec = 0, .tv_usec = RTO_MS * 1000 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Send filename packet
    sham_packet_t packet;
    strncpy((char*)packet.data, output_file_name, PAYLOAD_SIZE - 1);
    packet.data[PAYLOAD_SIZE - 1] = '\0';
    int filename_len = strlen(output_file_name) + 1;
    packet.header = (sham_header_t){htonl(seq), 0, 0x8, 0}; // Filename flag
    
    log_event("SND FILENAME=%s", output_file_name);
    sendto(sockfd, &packet, sizeof(sham_header_t) + filename_len, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    
    // Wait for filename ACK
    recvfrom(sockfd, &packet.header, sizeof(packet.header), 0, NULL, NULL);
    seq += filename_len;

    int bytes_read;
    while((bytes_read = fread(packet.data, 1, PAYLOAD_SIZE, fp)) > 0) {
        packet.header = (sham_header_t){htonl(seq), 0, 0, 0};

// ############## LLM Generated Code Begins ##############        
        while (1) {
            log_event("SND DATA SEQ=%u LEN=%d", seq, bytes_read);
            sendto(sockfd, &packet, sizeof(sham_header_t) + bytes_read, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));

            // Wait for ACK
            int len = recvfrom(sockfd, &packet.header, sizeof(packet.header), 0, NULL, NULL);
            if (len > 0 && (packet.header.flags & ACK) && ntohl(packet.header.ack_num) == seq + bytes_read) {
                log_event("RCV ACK=%u", ntohl(packet.header.ack_num));
                seq += bytes_read;
                break; // ACK ok, next packet
            } else {
                // Timeout or wrong ACK
                log_event("TIMEOUT SEQ=%u", seq);
                log_event("RETX DATA SEQ=%u LEN=%d", seq, bytes_read);
            }
        }
    }
    fclose(fp);
    return seq;
}
// ############## LLM Generated Code Ends ################

void chat_mode(int sockfd, struct sockaddr_in* server_addr, uint32_t seq, uint32_t ack) {
    (void)ack;
    printf("Chat mode started. Type '/quit' to exit.\n");
    fcntl(sockfd, F_SETFL, O_NONBLOCK); // Non-blocking socket

    fd_set read_fds;
    char buffer[PAYLOAD_SIZE];
    sham_packet_t packet;


// ############## LLM Generated Code Begins ##############
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);

        select(sockfd + 1, &read_fds, NULL, NULL, NULL);

        // Check for user input
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            fgets(buffer, PAYLOAD_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            if (strcmp(buffer, "/quit") == 0) break;
            
            packet.header = (sham_header_t){htonl(seq), 0, 0, 0};
            strcpy(packet.data, buffer);
            sendto(sockfd, &packet, sizeof(sham_header_t) + strlen(buffer), 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
        }

        // Check for incoming packets
        if (FD_ISSET(sockfd, &read_fds)) {
            memset(packet.data, 0, PAYLOAD_SIZE);
            int len = recvfrom(sockfd, &packet, sizeof(packet), 0, NULL, NULL);
            if (len > 0) {
                if (packet.header.flags & FIN) break; // Server closed
                printf("Received: %s\n", packet.data);
            }
        }
    }
}

// ############## LLM Generated Code Ends ################
