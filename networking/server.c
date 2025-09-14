#include "sham.h"

uint32_t file_transfer_mode(int sockfd, struct sockaddr_in* client_addr, const char* default_output_file, double loss_rate, uint32_t expected_seq);
uint32_t chat_mode(int sockfd, struct sockaddr_in* client_addr, double loss_rate);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [--chat] [loss_rate]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    setup_logging("server");

    // Argument Parsing
    int port = atoi(argv[1]);
    int is_chat_mode = 0;
    double loss_rate = 0.0;
    if (argc > 2) {
        if (strcmp(argv[2], "--chat") == 0) {
            is_chat_mode = 1;
            if (argc > 3) loss_rate = atof(argv[3]);
        } else {
            loss_rate = atof(argv[2]);
        }
    }

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Create and bind socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket"); exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); close(sockfd); exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d\n", port);
    
    // 3-Way Handshake
    sham_packet_t packet;
    recvfrom(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr *)&client_addr, &client_len);
    uint32_t client_seq = ntohl(packet.header.seq_num);
    printf("RCV SYN SEQ=%u\n", client_seq);
    log_event("RCV SYN SEQ=%u", client_seq);

    srand(time(NULL));
    uint32_t server_seq = rand();
    packet.header = (sham_header_t){htonl(server_seq), htonl(client_seq + 1), SYN | ACK, 0};
    printf("SND SYN-ACK SEQ=%u ACK=%u\n", server_seq, client_seq + 1);
    log_event("SND SYN-ACK SEQ=%u ACK=%u", server_seq, client_seq + 1);
    sendto(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr *)&client_addr, client_len);

    recvfrom(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr *)&client_addr, &client_len);
    printf("RCV ACK FOR SYN\n");
    log_event("RCV ACK FOR SYN");

    printf("Connection Established.\n");

    uint32_t client_fin_seq = 0;
    if (is_chat_mode) {
        client_fin_seq = chat_mode(sockfd, &client_addr, loss_rate);
    } else {
        // Default filename, can be overwritten by client
        client_fin_seq = file_transfer_mode(sockfd, &client_addr, "received_file.txt", loss_rate, client_seq + 1);
    }

//############## LLM Generated Code Begins ##############    
    
    // 4-Way Handshake (Teardown)
    printf("Closing connection...\n");
    printf("RCV FIN SEQ=%u\n", client_fin_seq);
    log_event("RCV FIN SEQ=%u", client_fin_seq);
    
    packet.header = (sham_header_t){htonl(server_seq + 1), htonl(client_fin_seq + 1), ACK, 0};
    printf("SND ACK FOR FIN\n");
    log_event("SND ACK FOR FIN");
    sendto(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr *)&client_addr, client_len);

    packet.header = (sham_header_t){htonl(server_seq + 1), 0, FIN, 0};
    printf("SND FIN SEQ=%u\n", server_seq + 1);
    log_event("SND FIN SEQ=%u", server_seq + 1);
    sendto(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr *)&client_addr, client_len);
    
    // Wait for final ACK
    recvfrom(sockfd, &packet.header, sizeof(packet.header), 0, (struct sockaddr *)&client_addr, &client_len);
    printf("RCV ACK FOR FIN\n");
    log_event("RCV ACK FOR FIN");

    printf("Connection Closed.\n");
    close(sockfd);
    if(log_fp) fclose(log_fp);
    return 0;
}

//############## LLM Generated Code Ends ################

uint32_t file_transfer_mode(int sockfd, struct sockaddr_in* client_addr, const char* default_output_file, double loss_rate, uint32_t expected_seq) {
    sham_packet_t packet;
    socklen_t client_len = sizeof(*client_addr);
    char output_file[256];
    strcpy(output_file, default_output_file); // Default filename
    uint32_t fin_seq = 0;
    
    // Receive filename packet from client
    int len = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)client_addr, &client_len);
    if (len > 0 && (packet.header.flags & 0x8)) { // Filename packet
        strncpy(output_file, (char*)packet.data, sizeof(output_file) - 1);
        output_file[sizeof(output_file) - 1] = '\0';
        log_event("RCV FILENAME=%s", output_file);
        
        // ACK for filename
        sham_header_t ack_header = {0, htonl(expected_seq + len - sizeof(sham_header_t)), ACK, 0};
        sendto(sockfd, &ack_header, sizeof(ack_header), 0, (struct sockaddr *)client_addr, client_len);
        expected_seq += len - sizeof(sham_header_t);
    }
    
    FILE *fp = fopen(output_file, "wb");
    if (!fp) { perror("fopen"); return 0; }

    while (1) {
        int len = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)client_addr, &client_len);
        if (len <= 0) continue;

        if (packet.header.flags & FIN) {
            fin_seq = ntohl(packet.header.seq_num);
            break;
        }

        uint32_t received_seq = ntohl(packet.header.seq_num);
        int data_len = len - sizeof(sham_header_t);


//############## LLM Generated Code Begins ##############        

        // Simulate packet loss
        if (loss_rate > 0 && ((double)rand() / RAND_MAX) < loss_rate) {
            log_event("DROP DATA SEQ=%u", received_seq);
            continue; // Drop packet
        }
 
        log_event("RCV DATA SEQ=%u LEN=%d", received_seq, data_len);

//############## LLM Generated Code Ends ################


        if (received_seq == expected_seq) {
            fwrite(packet.data, 1, data_len, fp);
            expected_seq += data_len; // Slide window
        }

        // Send cumulative ACK
        sham_header_t ack_header = {0, htonl(expected_seq), ACK, 0};
        log_event("SND ACK=%u WIN=0", expected_seq);
        sendto(sockfd, &ack_header, sizeof(ack_header), 0, (struct sockaddr*)client_addr, client_len);
    }
    fclose(fp);
    calculate_md5(output_file);
    return fin_seq; // Return FIN sequence number for teardown
}


uint32_t chat_mode(int sockfd, struct sockaddr_in* client_addr, double loss_rate) {
    (void)loss_rate;
    printf("Chat mode started. Type '/quit' to exit.\n");
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    fd_set read_fds;
    char buffer[PAYLOAD_SIZE];
    sham_packet_t packet;
    socklen_t client_len = sizeof(*client_addr);

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

            strcpy(packet.data, buffer);
            packet.header = (sham_header_t){0, 0, 0, 0};
            sendto(sockfd, &packet, sizeof(sham_header_t) + strlen(buffer), 0, (struct sockaddr*)client_addr, client_len);
        }

        // Check for incoming packets
        if (FD_ISSET(sockfd, &read_fds)) {
            memset(packet.data, 0, PAYLOAD_SIZE);
            int len = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)client_addr, &client_len);
            if (len > 0) {
                 if (packet.header.flags & FIN) {
                     return ntohl(packet.header.seq_num); // Return FIN sequence
                 }
                 printf("Received: %s\n", packet.data);
            }
        }
    }
    return 0;
}
