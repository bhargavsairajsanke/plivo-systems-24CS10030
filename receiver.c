#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>
#include <stdint.h>

#define MAX_SEQ 100000

int received[MAX_SEQ] = {0};
double last_nack_time[MAX_SEQ] = {0};

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

int main(void) {
    char *t0_str = getenv("T0");
    char *delay_str = getenv("DELAY_MS");
    if (!t0_str || !delay_str) {
        fprintf(stderr, "Missing environment variables\n");
        return 1;
    }
    
    double t0 = atof(t0_str);
    double delay_sec = atof(delay_str) / 1000.0;

   
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47002);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof(in_addr)) < 0) {
        perror("bind 47002");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in player = {0};
    player.sin_family = AF_INET;
    player.sin_port = htons(47020);
    player.sin_addr.s_addr = inet_addr("127.0.0.1");

    
    int fb_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay_fb = {0};
    relay_fb.sin_family = AF_INET;
    relay_fb.sin_port = htons(47003);
    relay_fb.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct pollfd fds[1];
    fds[0].fd = in_fd; 
    fds[0].events = POLLIN;

    unsigned char buf[2048];
    uint32_t highest_seq = 0;

    for (;;) {
        
        int p = poll(fds, 1, 10);
        double now = get_time();

        if (p > 0 && (fds[0].revents & POLLIN)) {
            ssize_t n = recvfrom(in_fd, buf, sizeof(buf), 0, NULL, NULL);
            if (n == 164) {
                uint32_t seq;
                memcpy(&seq, buf, 4);
                seq = ntohl(seq);

                if (seq < MAX_SEQ && !received[seq]) {
                    received[seq] = 1;
                    
                    sendto(out_fd, buf, n, 0, (struct sockaddr *)&player, sizeof(player));
                    if (seq > highest_seq) {
                        highest_seq = seq;
                    }
                }
            }
        }

        
        uint32_t start_seq = (highest_seq > 100) ? highest_seq - 100 : 0;
        for (uint32_t i = start_seq; i < highest_seq; i++) {
            if (!received[i]) {
                double deadline = t0 + delay_sec + (i * 0.020);
                
               
                if (now + 0.010 < deadline) {
                  
                    if (now - last_nack_time[i] > 0.060) {
                        uint32_t net_seq = htonl(i);
                        sendto(fb_fd, &net_seq, 4, 0, (struct sockaddr *)&relay_fb, sizeof(relay_fb));
                        last_nack_time[i] = now;
                    }
                }
            }
        }
    }
    return 0;
}