#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ======= Client Configuration =======
static const char *SERVER_IP = "147.27.71.9";  // Must match server
static const int PORT = 8080;
static const int TEST_DURATION = 30;             // Must match server
static const int BUFFER_SIZE = 65536;            // 64KB send buffer
// ====================================

int main() {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[ERROR] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "[ERROR] Invalid server IP address\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[ERROR] Connection to server failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[STATUS] Connected to server %s:%d\n", SERVER_IP, PORT);

    // Prepare buffer
    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    memset(buffer, 'A', BUFFER_SIZE);

    // Timing
    struct timeval start, now;
    gettimeofday(&start, NULL);
    size_t total_sent = 0;

    while (1) {
        gettimeofday(&now, NULL);
        double elapsed = (now.tv_sec - start.tv_sec) +
                         (now.tv_usec - start.tv_usec) / 1e6;
        if (elapsed >= TEST_DURATION)
            break;

        ssize_t sent = send(sockfd, buffer, BUFFER_SIZE, 0);
        if (sent < 0) {
            perror("[ERROR] Send failed");
            break;
        }
        total_sent += sent;
    }

    double mb_sent = total_sent / 1e6;
    double mbps = (total_sent * 8.0) / (TEST_DURATION * 1e6);

    printf("[RESULTS] Total sent: %.2f MB\n", mb_sent);
    printf("[RESULTS] Average throughput: %.2f Mbps\n", mbps);

    free(buffer);
    close(sockfd);
    return 0;
}
