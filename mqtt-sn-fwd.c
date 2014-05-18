/*
  MQTT-SN command-line publishing client
  Copyright (C) 2013 Nicholas Humfrey

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include "mqtt-sn.h"

const char *mqtt_sn_gw_addr = "127.0.0.1";
int mqtt_sn_gw_port = 1883;
int mqtt_sn_local_port = 1883;
uint16_t keep_alive = 10;
uint8_t debug = FALSE;
uint8_t keep_running = TRUE;
int sock_gw;
int sock_udp;
#define MAXBUF 65536

static void usage()
{
    fprintf(stderr, "Usage: mqtt-sn-fwd [opts]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -d             Enable debug messages.\n");
    fprintf(stderr, "  -g <ip>        MQTT-SN gateway to connect to.\n");
    fprintf(stderr, "  -p <port>      port on the MQTT-SN gateway, default is '%d'.\n", mqtt_sn_gw_port);
    fprintf(stderr, "  -l <port>      IPV6 port to listen on for incoming UDP, default is '%d'.\n", mqtt_sn_local_port);
    fprintf(stderr, "  -k <keepalive> keep alive in seconds for this client. Defaults to %d.\n", keep_alive);
    exit(-1);
}

static void parse_opts(int argc, char** argv)
{
    int ch;

    // Parse the options/switches
    while ((ch = getopt(argc, argv, "1cdh:i:k:p:t:T:v?")) != -1)
      switch (ch) {
        case 'd':
            debug = TRUE;
	    break;
        case 'g':
            mqtt_sn_gw_addr = optarg;
	    break;
        case 'k':
            keep_alive = atoi(optarg);
	    break;
        case 'p':
	  mqtt_sn_gw_port = atoi(optarg);
	  break;
        case 'l':
	  mqtt_sn_local_port = atoi(optarg);
	  break;
        case '?':
        default:
            usage();
        break;
    }

    // Missing Parameter?
    if (!mqtt_sn_gw_addr || !mqtt_sn_gw_port || !mqtt_sn_local_port) {
        usage();
    }
}

static void termination_handler (int signum)
{
    switch(signum) {
        case SIGHUP:  fprintf(stderr, "Got hangup signal."); break;
        case SIGTERM: fprintf(stderr, "Got termination signal."); break;
        case SIGINT:  fprintf(stderr, "Got interupt signal."); break;
    }

    // Signal the main thead to stop
    keep_running = FALSE;
}

static inline void die(char* msg)
{
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static inline void setup_signal_handlers(void)
{
    signal(SIGTERM, termination_handler);
    signal(SIGINT, termination_handler);
    signal(SIGHUP, termination_handler);
}

static inline void set_non_blocking(int sock)
{
  int flags;
  if ((flags = fcntl(sock, F_GETFL)) < 0)
    die("Failed to read flags of a socket");
  flags |= O_NONBLOCK;
  if (fcntl(sock, F_SETFL, flags) == -1)
    die("Failed to set non blocking mode for a socket");
}

int udp_server_start()
{
  int sock;
  int status;
  struct sockaddr_in6 sin6;
  int sin6len;

  sock = socket(PF_INET6, SOCK_DGRAM,0);

  sin6len = sizeof(struct sockaddr_in6);

  memset(&sin6, 0, sin6len);

  sin6.sin6_port = htons(mqtt_sn_local_port);
  sin6.sin6_family = AF_INET6;
  sin6.sin6_addr = in6addr_any;

  //  set_non_blocking(sock);
  status = bind(sock, (struct sockaddr *)&sin6, sin6len);
  if(status == -1)
    die("Failed to bind UDP socket");

  //  status = getsockname(sock, (struct sockaddr *)&sin6, &sin6len);

  //  printf("%d\n", ntohs(sin6.sin6_port));

  return sock;
}

void sock_destroy(int sock)
{
  shutdown(sock, 2);
  close(sock);
}

#define max(a, b) ((a > b) ? a : b)
#define GW_HDR_SIZE 3 + 1 + 4 + 16

void send_keepalive(void)
{}

int create_gw_socket(void)
{}

void process_up_buff(char *buf, int s, char *addr, int port)
{
  buf[0] = GW_HDR_SIZE;
  buf[1] = 0xFE;
  buf[2] = 0x00;
  buf[3] = 0x00;
  memcpy(buf + 4, addr, 16);
  memcpy(buf + 4 + 16, (char *) &port, 4);
  send(sock_gw, buf, GW_HDR_SIZE + s, 0);
}

void process_down_buff(char* buf, int s)
{
  struct sockaddr_in6 d_addr;
  socklen_t len = sizeof(d_addr);  
  ssize_t res;
  int sock = socket(PF_INET6, SOCK_DGRAM, 0);
  memset (&d_addr, 0, sizeof(d_addr));
  d_addr.sin6_family = AF_INET6;
  d_addr.sin6_port = htons (*(((int32_t*) (buf + 4 + 16))));
  buf[4+16] = 0;
  if (inet_pton (AF_INET6, buf + 4, &d_addr.sin6_addr) != 1)
    die("inet_pton");
  len = sizeof(d_addr);
  sendto (sock, buf + GW_HDR_SIZE, s - GW_HDR_SIZE, 0, (struct sockaddr*) &d_addr, len);
}

void handle_traffic(void)
{
    char buff_up[MAXBUF + GW_HDR_SIZE];
    char buff_down[MAXBUF + GW_HDR_SIZE];
    int s;
    time_t now = time(NULL);
    struct timeval tv;
    fd_set rfd;
    int ret;
    int gw = sock_gw;
    int local = sock_udp;

    FD_ZERO(&rfd);
    FD_SET(gw, &rfd);
    FD_SET(local, &rfd);

    tv.tv_sec = keep_alive;
    tv.tv_usec = 0;

    //    ret = select(FD_SETSIZE, &rfd, NULL, NULL, &tv);
    ret = select(max(gw, local) + 1, &rfd, NULL, NULL, &tv);
    if ((ret < 0) && (errno != EINTR))
      die("Select failed");
    
    if (ret > 0) {
      if (FD_ISSET(gw, &rfd)) {
	s = recv(gw, buff_down, MAXBUF, 0);
	process_down_buff(buff_down, s);
      }
      if (FD_ISSET(local, &rfd)) {
	struct sockaddr_in6 sin6;
	socklen_t sin6len;
	sin6len = sizeof(struct sockaddr_in6);
	s = recvfrom(local, buff_up + GW_HDR_SIZE, MAXBUF, 0, 
		     (struct sockaddr * __restrict__)&sin6,
		     (socklen_t * __restrict__)&sin6len);
	process_up_buff(buff_up, s, (char *) sin6.sin6_addr.s6_addr, sin6.sin6_port);
      }
    } else
      send_keepalive();
}

int main(int argc, char* argv[])
{
    parse_opts(argc, argv);
    mqtt_sn_set_debug(debug);
    setup_signal_handlers();
    sock_gw = create_gw_socket();
    sock_udp = udp_server_start();

    while(keep_running)
      handle_traffic();

    sock_destroy(sock_udp);
    sock_destroy(sock_gw);
    return 0;
}
