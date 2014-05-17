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

#include "mqtt-sn.h"

const char *mqtt_sn_gw_addr = "127.0.0.1";
const char *mqtt_sn_gw_port = "1883";
uint16_t keep_alive = 10;
uint8_t debug = FALSE;
uint8_t keep_running = TRUE;

static void usage()
{
    fprintf(stderr, "Usage: mqtt-sn-fwd [opts]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -d             Enable debug messages.\n");
    fprintf(stderr, "  -g <ip>        MQTT-SN gateway to connect to.\n");
    fprintf(stderr, "  -p <port>      port on the MQTT-SN gateway.\n");
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
            mqtt_sn_gw_port = optarg;
	    break;
        case '?':
        default:
            usage();
        break;
    }

    // Missing Parameter?
    if (!mqtt_sn_gw_addr || !mqtt_sn_gw_port) {
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

int main(int argc, char* argv[])
{
    int sock, timeout;

    // Parse the command-line options
    parse_opts(argc, argv);

    // Enable debugging?
    mqtt_sn_set_debug(debug);

    // Work out timeout value for main loop
    if (keep_alive) {
        timeout = keep_alive / 2;
    } else {
        timeout = 10;
    }

    // Setup signal handlers
    signal(SIGTERM, termination_handler);
    signal(SIGINT, termination_handler);
    signal(SIGHUP, termination_handler);

    // Create a TCP socket
    //    sock = 
    if (sock) {
        // Keep processing packets until process is terminated
        while(keep_running) {

            }
        }

        close(sock);

    mqtt_sn_cleanup();

    return 0;
}
