/******************************************************************************
 * File: uMQTT_pub_test.c
 * Description: Program to create a test Linux socket broker connection and
 *              send a publish message before disconnecting from the broker.
 * Author: Steven Swann - swannonline@googlemail.com
 *
 * Copyright (c) swannonline, 2013-2014
 *
 * This file is part of uMQTT.
 *
 * uMQTT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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

#include "uMQTT.h"
#include "uMQTT_client.h"
#include "uMQTT_linux_client.h"
#include "uMQTT_helper.h"
#include "../inc/log.h"

int main(int argc, char *argv[]) {
  log_level(LOG_DEBUG);

  struct broker_conn *conn;

  int ret = 0;

  if (argc > 1) {
    /* argv[1] should be the ip address of the broker */
    log_std(LOG_INFO, "Broker IP: %s\n", argv[1]);
    init_linux_socket_connection(&conn, argv[1], strlen(argv[1]),
        MQTT_BROKER_PORT);
  } else {
    init_linux_socket_connection(&conn, MQTT_BROKER_IP,
        sizeof(MQTT_BROKER_IP), MQTT_BROKER_PORT);
  }
  if (!conn) {
    log_std(LOG_ERROR, "Initialising connection");
    return -1;
  }

  struct linux_broker_socket *skt = (struct linux_broker_socket *)conn->context;

  ret = broker_connect(conn);
  if (ret) {
    log_std(LOG_ERROR, "Initialising broker connection");
    return -1;
  } else {
    log_std(LOG_INFO, "Connected to broker:\nip: %s port: %d", skt->ip, skt->port);
  }

  /* publish packet */
  struct mqtt_packet *pub_pkt = construct_default_packet(PUBLISH,
      (uint8_t *)"uMQTT test PUBLISH packet",
      sizeof("uMQTT test PUBLISH packet"));
  if (!pub_pkt) {
    log_std(LOG_ERROR, "Initialising PUBLISH packet");
    goto free;
  }

  finalise_packet(pub_pkt);

  print_packet_hex_debug(pub_pkt);
  ret = conn->send_method(conn, pub_pkt);
  if (ret) {
    log_std(LOG_ERROR, "Sending PUBLISH message failed");
    goto free;
  }

free:
  free_packet(pub_pkt);

  broker_disconnect(conn);
  free_connection(conn);

  return ret;
}

