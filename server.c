#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ========== Server Configuration ==========
static const char *SERVER_IP = "192.168.1.100";  // Change to your server's IP
static const int PORT = 8080;
static const int TEST_DURATION = 30;      // 30-second test duration
static const int INTERVAL_PRINT = 2;      // 2-second status updates
static const int BUFFER_SIZE = 65536;     // 64KB receive buffer
static const int MAX_PENDING = 5;         // Max queued connections
// ==========================================

void handle_client(int client_socket) {
    struct timeval start, now, last_print;
    size_t total_bytes = 0, interval_bytes = 0;
    char buffer[BUFFER_SIZE];
    
    gettimeofday(&start, NULL);
    last_print = start;
    
    printf("\n[STATUS] Speed test initiated\n");

    while (1) {
        gettimeofday(&now, NULL);
        double elapsed = (now.tv_sec - start.tv_sec) + 
                        (now.tv_usec - start.tv_usec) / 1e6;

        // Exit after test duration
        if (elapsed >= TEST_DURATION) {
            printf("[STATUS] Test duration completed\n");
            break;
        }

        // Receive client data
        ssize_t received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            printf("[WARNING] Client connection lost\n");
            break;
        }

        // Update metrics
        total_bytes += received;
        interval_bytes += received;

        // Calculate time since last print
        double since_last = (now.tv_sec - last_print.tv_sec) +
                          (now.tv_usec - last_print.tv_usec) / 1e6;

        // Print throughput statistics
        if (since_last >= INTERVAL_PRINT) {
            double current = (interval_bytes * 8.0) / (since_last * 1e6);
            double average = (total_bytes * 8.0) / (elapsed * 1e6);
            
            printf("[PROGRESS] %.1fs elapsed\n", elapsed);
            printf("  Current throughput: %7.2f Mbps\n", current);
            printf("  Average throughput: %7.2f Mbps\n\n", average);
            
            interval_bytes = 0;
            last_print = now;
        }
    }

    // Final report
    double final_throughput = (total_bytes * 8.0) / (TEST_DURATION * 1e6);
    printf("[RESULTS] Total transferred: %.2f MB\n", total_bytes / 1e6);
    printf("[RESULTS] Average throughput: %.2f Mbps\n", final_throughput);
    close(client_socket);
}

int main() {
    // Validate server IP
    if (strcmp(SERVER_IP, "127.0.0.1") == 0) {
        fprintf(stderr, "[ERROR] Server cannot run on localhost\n");
        exit(EXIT_FAILURE);
    }

    // Create server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) {
        perror("[ERROR] Socket configuration failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configure network settings
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("[ERROR] Binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, MAX_PENDING) < 0) {
        perror("[ERROR] Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[STATUS] Server operational\n");
    printf("IP Address: %s\n", SERVER_IP);
    printf("Port: %d\n", PORT);
    printf("Test Duration: %d seconds\n", TEST_DURATION);

    // Main server loop
    while (1) {
        printf("\n[STATUS] Awaiting client connection...\n");
        
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            perror("[WARNING] Accept error");
            continue;
        }

        // Get client IP
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("[STATUS] Connection from %s\n", client_ip);

        // Handle client and close connection
        handle_client(client_fd);
        printf("[STATUS] %s disconnected\n", client_ip);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
