# Aping
Enhanced ping utility
-------------------------------
# Aping - Enhanced Ping Utility

## Overview

**Aping** is an advanced network diagnostic tool written in **C**. It extends the traditional **ping** command by offering more detailed features such as packet loss statistics, round-trip time (RTT) calculations, Time-to-Live (TTL) values, reverse DNS lookups, WHOIS queries, and optional geolocation information.

Aping provides an interactive terminal interface that allows users to ping hosts or IP addresses and gather detailed network diagnostic data.

## Features

- **Interactive Mode**: Prompts the user for a hostname or IP address to ping.
- **Packet Loss and RTT Statistics**: Displays packet loss percentage, minimum, average, and maximum RTT values.
- **TTL Information**: Displays the TTL (Time-to-Live) for each reply.
- **Reverse DNS Lookup**: Resolves the IP address to a domain name using `dig`.
- **WHOIS Lookup**: Performs a WHOIS query to gather network information about the host.
- **Geolocation Lookup (Optional)**: Uses `curl` to access geolocation services (e.g., `ipinfo.io`).

## Requirements

- **Root privileges**: Required for using raw sockets (for sending ICMP packets).
- **Dependencies**:
  - `dig`: For reverse DNS lookups.
  - `whois`: For WHOIS queries.
  - `curl` and `jq`: For geolocation lookups.

## Installation

1. **Clone or download the source code.**

2. **Install dependencies** (if not already installed):
   ```bash
   sudo apt-get install dig whois curl jq
   ```
3. **Compilation**
   ```bash
   gcc -o Aping Aping.c
   ```
4. **Run Aping (requires root privileges):**
   ```bash
   sudo ./Aping
   ```
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
## Example Output

```bash
$ sudo ./Aping
=========================================
     Aping - Enhanced Ping Utility
=========================================
Enter hostname or IP to ping: 8.8.8.8
Resolved 8.8.8.8 to 8.8.8.8

Pinging 8.8.8.8 (8.8.8.8) with 64 bytes of data:

Reply from 8.8.8.8: seq=1 time=10.56 ms TTL=56
Reply from 8.8.8.8: seq=2 time=10.34 ms TTL=56
Reply from 8.8.8.8: seq=3 time=10.22 ms TTL=56
Reply from 8.8.8.8: seq=4 time=10.43 ms TTL=56

=== 8.8.8.8 ping statistics ===
4 packets transmitted, 4 received, 0.0% packet loss
rtt min/avg/max = 10.22/10.39/10.56 ms

=== Reverse DNS Lookup ===
8.8.8.8 not found in DNS records.

=== WHOIS Lookup ===
Running: whois 8.8.8.8
[WHOIS information...]

=== Geolocation Lookup ===
Running: curl -s https://ipinfo.io/8.8.8.8 | jq .
[Geolocation data...]
```

## License

This program is licensed under the GPL-3 License. You are free to modify, distribute, and use this code for personal or commercial purposes with the condition that the copyright notice and license text are included in all copies of the software.
