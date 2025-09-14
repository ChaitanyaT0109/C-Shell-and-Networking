#ifndef SHAM_H
#define SHAM_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>
#include <stdarg.h>
#include <openssl/evp.h>
#include <fcntl.h>
#include <errno.h>

#define PAYLOAD_SIZE 1024
#define RTO_MS 500 // Retransmission Timeout in milliseconds

// SHAM Packet Flags
#define SYN 0x1
#define ACK 0x2
#define FIN 0x4

#pragma pack(push, 1)
typedef struct sham_header {
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window_size;
} sham_header_t;

typedef struct sham_packet {
    sham_header_t header;
    char data[PAYLOAD_SIZE];
} sham_packet_t;
#pragma pack(pop)

// Global logger file pointer
FILE *log_fp;

// Sets up the logger if the RUDP_LOG environment variable is set
void setup_logging(const char* role) {
    log_fp = NULL;
    if (getenv("RUDP_LOG") != NULL) {
        if (strcmp(role, "client") == 0) {
            log_fp = fopen("client_log.txt", "w");
        } else {
            log_fp = fopen("server_log.txt", "w");
        }
        if (log_fp == NULL) {
            perror("fopen log file failed");
        }
    }
}

// TO log a formatted message with a timestamp
void log_event(const char *format, ...) {
    if (log_fp == NULL) return;

    char time_buffer[30];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t curtime = tv.tv_sec;
    strftime(time_buffer, 30, "%Y-%m-%d %H:%M:%S", localtime(&curtime));

    fprintf(log_fp, "[%s.%06ld] [LOG] ", time_buffer, tv.tv_usec);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_fp, format, args);
    va_end(args);
    
    fprintf(log_fp, "\n");
    fflush(log_fp);
}

// MD5 checksum
void calculate_md5(const char* filename) {
    unsigned char c[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    int i;
    FILE *inFile = fopen(filename, "rb");
    EVP_MD_CTX *mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL) {
        printf("MD5 Error: %s can't be opened.\n", filename);
        return;
    }

    mdContext = EVP_MD_CTX_new();
    if (mdContext == NULL) {
        printf("MD5 Error: Failed to create MD5 context.\n");
        fclose(inFile);
        return;
    }

    if (EVP_DigestInit_ex(mdContext, EVP_md5(), NULL) != 1) {
        printf("MD5 Error: Failed to initialize MD5.\n");
        EVP_MD_CTX_free(mdContext);
        fclose(inFile);
        return;
    }

    while ((bytes = fread(data, 1, 1024, inFile)) != 0) {
        if (EVP_DigestUpdate(mdContext, data, bytes) != 1) {
            printf("MD5 Error: Failed to update MD5.\n");
            EVP_MD_CTX_free(mdContext);
            fclose(inFile);
            return;
        }
    }

    if (EVP_DigestFinal_ex(mdContext, c, &md_len) != 1) {
        printf("MD5 Error: Failed to finalize MD5.\n");
        EVP_MD_CTX_free(mdContext);
        fclose(inFile);
        return;
    }
    
    printf("MD5: ");
    for(i = 0; i < (int)md_len; i++) {
        printf("%02x", c[i]);
    }
    printf("\n");
    
    EVP_MD_CTX_free(mdContext);
    fclose(inFile);
}

#endif
