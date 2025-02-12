/******************************************************************************
 * Aping - Enhanced Ping Utility
 *
 * Features:
 *  - Interactive prompt for hostname/IP
 *  - Raw socket ICMP Echo Request/Reply
 *  - RTT measurement
 *  - Packet loss statistics
 *  - TTL display
 *  - Reverse DNS lookup (via system calls or getnameinfo)
 *  - WHOIS lookup
 *  - (Optional) Geolocation Lookup
 *
 * Compile:
 *   gcc -o Aping Aping.c
 * Run:
 *   sudo ./Aping
 *
 * Licensed under the GPL-3 License.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include <netinet/ip.h>    /* for IP_MAXPACKET */
#include <netinet/ip_icmp.h>

#define PACKET_SIZE     64
#define PING_COUNT      4
#define TIMEOUT_SEC     1
#define TIMEOUT_USEC    0

#define COLOR_RESET     "\033[0m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_RED       "\033[31m"
#define COLOR_BOLD      "\033[1m"

#define CHECK_ERR(x, msg) \
    if ((x) < 0) { \
        perror(msg); \
        exit(EXIT_FAILURE); \
    }

/******************************************************************************
 * Calculate ICMP checksum
 ******************************************************************************/
unsigned short calculate_checksum(unsigned short *buf, int length) {
    unsigned long sum = 0;
    while (length > 1) {
        sum += *buf++;
        length -= 2;
    }
    if (length == 1) {
        unsigned short tmp = 0;
        *(unsigned char *)&tmp = *(unsigned char *)buf;
        sum += tmp;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

/******************************************************************************
 * Send ICMP Echo Request and measure RTT
 ******************************************************************************/
struct timeval send_ping(int sockfd, struct sockaddr_in *dest_addr, const char *ip_addr, int seq) {
    char packet[PACKET_SIZE];
    memset(packet, 0, PACKET_SIZE);

    struct icmphdr *icmp_hdr = (struct icmphdr *)packet;

    /* ICMP header setup */
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0;
    icmp_hdr->un.echo.id = getpid() & 0xFFFF;
    icmp_hdr->un.echo.sequence = seq;
    icmp_hdr->checksum = 0;

    /* Dummy data to fill the packet */
    const char *msg = "ApingData";
    memcpy(packet + sizeof(struct icmphdr), msg, strlen(msg));

    /* Calculate checksum */
    icmp_hdr->checksum = calculate_checksum((unsigned short *)packet, PACKET_SIZE);

    /* Record send time */
    struct timeval send_time;
    gettimeofday(&send_time, NULL);

    /* Send the packet */
    ssize_t bytes_sent = sendto(sockfd, packet, PACKET_SIZE, 0,
                               (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in));
    if (bytes_sent < 0) {
        perror("sendto() failed");
    }

    return send_time;
}

/******************************************************************************
 * Receive ICMP Echo Reply and extract RTT
 ******************************************************************************/
int receive_ping(int sockfd, struct sockaddr_in *source_addr, int sequence, double *rtt, int *ttl) {
    char buffer[IP_MAXPACKET];
    memset(buffer, 0, IP_MAXPACKET);

    socklen_t addr_len = sizeof(struct sockaddr_in);
    struct timeval recv_time;

    ssize_t bytes_received = recvfrom(sockfd, buffer, IP_MAXPACKET, 0,
                                      (struct sockaddr *)source_addr, &addr_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN) {
            return -1;  // Timeout
        }
        perror("recvfrom() failed");
        return -1;
    }

    gettimeofday(&recv_time, NULL);

    /* Extract IP and ICMP headers */
    struct iphdr *ip_hdr = (struct iphdr *)buffer;
    int ip_hdr_len = ip_hdr->ihl * 4;
    struct icmphdr *icmp_hdr = (struct icmphdr *)(buffer + ip_hdr_len);

    if (icmp_hdr->type == ICMP_ECHOREPLY && icmp_hdr->un.echo.id == (getpid() & 0xFFFF) && icmp_hdr->un.echo.sequence == sequence) {
        /* Get TTL value from IP header */
        if (ttl) {
            *ttl = ip_hdr->ttl;
        }
        return 0;  // Success
    }

    return -1;  // Invalid reply
}

/******************************************************************************
 * Reverse DNS lookup
 ******************************************************************************/
void reverse_dns_lookup(const char *ip_addr) {
    printf(COLOR_CYAN "=== Reverse DNS Lookup ===" COLOR_RESET "\n");

    char command[256];
    snprintf(command, sizeof(command), "dig -x %s +short", ip_addr);
    printf("Running: %s\n", command);
    fflush(stdout);
    system(command);
}

/******************************************************************************
 * WHOIS lookup
 ******************************************************************************/
void whois_lookup(const char *ip_addr) {
    printf(COLOR_CYAN "=== WHOIS Lookup ===" COLOR_RESET "\n");

    char command[256];
    snprintf(command, sizeof(command), "whois %s", ip_addr);
    printf("Running: %s\n", command);
    fflush(stdout);
    system(command);
}

/******************************************************************************
 * Geolocation lookup (via ipinfo.io)
 ******************************************************************************/
void geolocation_lookup(const char *ip_addr) {
    printf(COLOR_CYAN "=== Geolocation Lookup ===" COLOR_RESET "\n");

    char command[256];
    snprintf(command, sizeof(command), "curl -s https://ipinfo.io/%s | jq .", ip_addr);
    printf("Running: %s\n", command);
    fflush(stdout);
    system(command);
}

/******************************************************************************
 * Main program loop
 ******************************************************************************/
int main() {
    printf(COLOR_BOLD "=========================================\n");
    printf("     Aping - Enhanced Ping Utility\n");
    printf("=========================================\n" COLOR_RESET);

    /* Ask for the target IP/hostname */
    char target_host[256];
    printf("Enter hostname or IP to ping: ");
    fflush(stdout);
    if (scanf("%255s", target_host) != 1) {
        fprintf(stderr, COLOR_RED "Error reading input.\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    /* Resolve the target address */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    int ret = getaddrinfo(target_host, NULL, &hints, &res);
    if (ret != 0 || !res) {
        fprintf(stderr, COLOR_RED "getaddrinfo: %s\n" COLOR_RESET, gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

    /* Convert IP address to string */
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));

    printf(COLOR_GREEN "Resolved %s to %s\n" COLOR_RESET, target_host, ip_str);

    /* Create raw socket */
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    CHECK_ERR(sockfd, "socket() error (need root privileges)");

    /* Set socket timeout */
    struct timeval timeout = { TIMEOUT_SEC, TIMEOUT_USEC };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    /* Prepare destination address structure */
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr = addr_in->sin_addr;

    freeaddrinfo(res);

    /* Track ping statistics */
    int sent_count = 0, recv_count = 0;
    double total_rtt = 0.0;
    double min_rtt = 999999.0, max_rtt = 0.0;

    printf(COLOR_BOLD "Pinging %s (%s) with %d bytes of data:\n\n" COLOR_RESET, target_host, ip_str, PACKET_SIZE);

    /* Send pings and collect statistics */
    for (int i = 0; i < PING_COUNT; i++) {
        struct timeval send_time = send_ping(sockfd, &dest_addr, ip_str, i + 1);
        sent_count++;

        int ttl = 0;
        double rtt = 0.0;

        /* Wait for reply */
        int status = receive_ping(sockfd, &dest_addr, i + 1, &rtt, &ttl);

        struct timeval recv_time;
        gettimeofday(&recv_time, NULL);

        double send_ms = (double)send_time.tv_sec * 1000.0 + (double)send_time.tv_usec / 1000.0;
        double recv_ms = (double)recv_time.tv_sec * 1000.0 + (double)recv_time.tv_usec / 1000.0;
        rtt = recv_ms - send_ms;

        if (status == 0) {
            recv_count++;
            if (rtt < min_rtt) min_rtt = rtt;
            if (rtt > max_rtt) max_rtt = rtt;
            total_rtt += rtt;

            printf("Reply from %s: seq=%d time=%.2f ms TTL=%d\n", ip_str, i + 1, rtt, ttl);
        } else {
            printf("Request timed out for seq=%d\n", i + 1);
        }

        sleep(1);
    }

    /* Calculate packet loss and average RTT */
    int lost_count = sent_count - recv_count;
    double packet_loss = ((double)lost_count / sent_count) * 100.0;
    double avg_rtt = (recv_count == 0) ? 0.0 : (total_rtt / recv_count);

    printf("\n" COLOR_BOLD "=== %s ping statistics ===" COLOR_RESET "\n", ip_str);
    printf("%d packets transmitted, %d received, %.2f%% packet loss\n", sent_count, recv_count, packet_loss);
    if (recv_count > 0) {
        printf("rtt min/avg/max = %.2f/%.2f/%.2f ms\n", min_rtt, avg_rtt, max_rtt);
    }
    close(sockfd);

    /* Additional lookups */
    reverse_dns_lookup(ip_str);
    whois_lookup(ip_str);
    geolocation_lookup(ip_str);

    printf(COLOR_BOLD "\nDone.\n" COLOR_RESET);
    return 0;
}
