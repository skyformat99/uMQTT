/******************************************************************************
 * File: uMQTT_sub.c
 * Description: MicroMQTT (uMQTT) sublish application using linux based sockets
 *              to connect to the broker.
 *              constrained environments.
 * Author: Steven Swann - swannonline@googlemail.com
 *
 * Copyright (c) swannonline, 2013-2014
 *
 * This file is part of uMQTT.
 *
 * uMQTT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as sublished by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * uMQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uMQTT.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <string.h>

#include <getopt.h>

#include "uMQTT.h"
#include "uMQTT_helper.h"
#include "uMQTT_linux_client.h"

/* ip of test.mosquitto.org */
#define MQTT_BROKER_IP        "85.119.83.194\0"
#define MQTT_BROKER_PORT      1883

#define MAX_TOPIC_LEN 1024
#define MAX_MSG_LEN 1024

/*
 * \brief function to print help
 */
static int print_usage() {

  fprintf(stderr,
      "uMQTT_sub is an application that connects to an MQTT broker and sends a user defined\n"
      "sublish pocket before disconnecting\n"
      "\n"
      "Usage: uMQTT_sub [options]\n\n"
      "General options:\n"
      " -h [--help]              : Displays this help and exits\n"
      " -v [--verbose]           : Verbose logging\n"
      "\n"
      "Publish options:\n"
      " -t [--topic] <topic>     : Change the default topic. Default: uMQTT_PUB\n"
      "\n"
      "Broker options:\n"
      " -b [--broker] <broker-IP>: Change the default broker IP - only IP addresses are\n"
      "                            currently supported. Default: test.mosquitto.org\n"
      " -p [--port] <port>       : Change the default port. Default: 1883\n"
      "\n");

  return 0;
}

int main(int argc, char **argv) {

  int ret;
  int c, option_index = 0;
  char topic[MAX_TOPIC_LEN] = UMQTT_DEFAULT_TOPIC;
  char broker_ip[16] = MQTT_BROKER_IP;
  int broker_port = MQTT_BROKER_PORT;
  int verbose = 0;

  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"help",   no_argument,             0, 'h'},
    {"verbose", no_argument,            0, 'v'},
    {"topic", required_argument,        0, 't'},
    {"broker", required_argument,       0, 'b'},
    {"port", required_argument,         0, 'p'},
    {0, 0, 0, 0}
  };

  /* get arguments */
  while (1)
  {
    if ((c = getopt_long(argc, argv, "hvt:b:p:", long_options, &option_index)) != -1) {

      switch (c) {
        case 'h':
          return print_usage();
          break;

        case 'v':
          /* set verbose */
          verbose = 1;
          break;

        case 't':
          /* Set topic */
          if (optarg) {
            strcpy(topic, optarg);
          } else {
            printf("Error: The topic flag should be followed by a topic.\n");
            return print_usage();
          }
          break;

        case 'b':
          /* change the default broker ip */
          if (optarg) {
            strcpy(broker_ip, optarg);
          } else {
            printf("Error: The broker flag should be followed by an IP address.\n");
            return print_usage();
          }
          break;

        case 'p':
          /* change the default port */
          if (optarg) {
            broker_port = *optarg;
          } else {
            printf("Error: The port flag should be followed by a port.\n");
            return print_usage();
          }
          break;
      }
    } else {
      break;
    }
  }

  struct broker_conn *conn;

  if (verbose) {
    printf("Initialising socket connection\n");
  }
  init_linux_socket_connection(&conn, broker_ip, sizeof(broker_ip), broker_port);
  if (!conn) {
    printf("Error: Initialising socket connection\n");
    return -1;
  }

  if (verbose) {
    printf("Connecting to broker\n");
  }

  struct linux_broker_socket *skt = '\0';
  if ((ret = broker_connect(conn))) {
    printf("Error: Initialising socket connection\n");
    free_connection(conn);
    return ret;
  } else {
    skt = (struct linux_broker_socket *)conn->context;
    if (verbose) {
      printf("Connected to broker:\nip: %s port: %d\n", skt->ip, skt->port);
    }
  }

  if (verbose) {
    printf("Subscribing to the following topics:\n");
    printf("Topic: %s\n", topic);
  }

  /* Find actual length of topic and subscribe */
  const char *end = strchr(topic, '\0');
  if (!end || (ret = broker_subscribe(conn, topic, end - topic))) {

    printf("Error: Subscribing to topic.\n");
    ret = UMQTT_ERROR;
    goto free;
  }
 
  /* Start listening for packets */
  struct mqtt_packet *pkt = NULL;
  if (init_packet(&pkt)) {
    printf("Error: Initialising packet\n");
    ret = UMQTT_ERROR;
    goto free;
  }

  while (1) {
    pkt->len = conn->receive_method(conn, &pkt->raw); 
    disect_raw_packet(pkt);
    if (verbose) {
      print_packet(pkt);
    } else {
      /*  process packet */
    }

  }

free:
  broker_disconnect(conn);
  free_connection(conn);
  free_packet(pkt);
  return ret;
}
