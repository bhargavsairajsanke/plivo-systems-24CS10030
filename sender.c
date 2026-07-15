#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <stdint.h>

#define MAX_SEQ 100000

// History buffer to store frames for retransmission
struct {
    uint8_t payload[164]; 
    int valid;
} history[MAX_SEQ];

int main(void) {
    // 1. Setup incoming media socket from harness (47010)
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47010);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof(in_addr)) < 0) {
        perror("bind 47010");
        return 1;
    }

    // 2. Setup incoming feedback socket from relay (47004)
    int fb_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in fb_addr = {0};
    fb_addr.sin_family = AF_INET;
    fb_addr.sin_port = htons(47004);
    fb_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(fb_fd, (struct sockaddr *)&fb_addr, sizeof(fb_addr)) < 0) {
        perror("bind 47004");
        return 1;
    }

    // 3. Setup outgoing socket to relay (47001)
    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay = {0};
    relay.sin_family = AF_INET;
    relay.sin_port = htons(47001);
    relay.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct pollfd fds[2];
    fds[0].fd = in_fd;  fds[0].events = POLLIN;
    fds[1].fd = fb_fd;  fds[1].events = POLLIN;

    unsigned char buf[2048];

    for (;;) {
        if (poll(fds, 2, -1) < 0) continue;

        // Lane A: New frame from harness
        if (fds[0].revents & POLLIN) {
            ssize_t n = recvfrom(in_fd, buf, sizeof(buf), 0, NULL, NULL);
            if (n == 164) {
                uint32_t seq;
                memcpy(&seq, buf, 4);
                seq = ntohl(seq);
                
                if (seq < MAX_SEQ) {
                    memcpy(history[seq].payload, buf, 164);
                    history[seq].valid = 1;
                }
                // Forward immediately
                sendto(out_fd, buf, 164, 0, (struct sockaddr *)&relay, sizeof(relay));
            }
        }

        // Lane B: NACK received from receiver
        if (fds[1].revents & POLLIN) {
            ssize_t n = recvfrom(fb_fd, buf, sizeof(buf), 0, NULL, NULL);
            if (n >= 4) {
                uint32_t seq;
                memcpy(&seq, buf, 4);
                seq = ntohl(seq);
                
                // Retransmit if we have it
                if (seq < MAX_SEQ && history[seq].valid) {
                    sendto(out_fd, history[seq].payload, 164, 0, (struct sockaddr *)&relay, sizeof(relay));
                }
            }
        }
    }
    return 0;
}