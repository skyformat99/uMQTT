/******************************************************************************
 * File: uMQTT.c
 * Description: MicroMQTT (uMQTT) library implementation suitable for
 *              constrained environments.
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
#include <stdlib.h>
#include <string.h>

#include "uMQTT.h"

/**
 * \brief Function to allocate memory for an mqtt_client.
 * \param client Pointer to the address of the new mqtt_client struct.
 */
void init_client(struct mqtt_client **client_p) {
  struct mqtt_client *client;

  if (!(client = calloc(1, sizeof(struct mqtt_client)))) {
    printf("Error: Allocating space for MQTT client failed.\n");
    //free_pkt(pkt);
  }

  *client_p = client;

  return;
}

/**
 * \brief Function to allocate memory for an mqtt packet.
 * \param pkt Pointer to the address of the new packet.
 */
void init_packet(struct mqtt_packet **pkt_p) {
  struct mqtt_packet *pkt;

  if (!(pkt = calloc(1, sizeof(struct mqtt_packet)))) {
    printf("Error: Allocating space for MQTT packet failed.\n");
    free_pkt(pkt);
  }

  *pkt_p = pkt;

  return;
}

/**
 * \brief Function to allocate memory for mqtt packet headers
 *        including both fixed and variable header components.
 * \param pkt Pointer to the address of the packet containing headers.
 * \param type The type of packet to be created.
 * \return Length of new packet headers
 */
int init_packet_header(struct mqtt_packet *pkt, ctrl_pkt_type type) {

  unsigned int fix_len = 0;
  unsigned int var_len = 0;

  /* allocate fixed header memory - always same size*/
  fix_len = sizeof(struct pkt_fixed_header);
  if (!(pkt->fixed = calloc(1, fix_len))) {
    printf("Error: Allocating space for fixed header failed.\n");
    free_pkt_fixed_header(pkt->fixed);
  }

  //pkt->fixed->ps.reserved = 6;
  pkt->fixed->connect.type = type;

  switch (type) {
    case CONNECT:
      /* variable header */
      var_len = sizeof(struct connect_variable_header);

      /* allocate variable header */
      if (!(pkt->variable = calloc(1, var_len))) {
        printf("Error: Allocating space for variable header failed.\n");
        free_pkt_variable_header(pkt->variable);
      }
      pkt->variable->connect.name_len = 0x04;
      memcpy(pkt->variable->connect.proto_name, MQTT_PROTO_NAME, 0x04);
      pkt->variable->connect.proto_level = MQTT_PROTO_LEVEL;

      break;

    case PUBLISH:
      /* variable header */
      var_len = sizeof(struct publish_variable_header);
      break;

    default:
      printf("Error: MQTT packet type not currently supported.\n");
      return 0;
  }

  /* debug */
  printf("Length of fixed packet header: %d\n", fix_len);
  printf("Length of variable packet header: %d\n", var_len);

  pkt->length = fix_len + var_len;

  encode_remaining_pkt_len(pkt, var_len);

  return pkt->length;
}

/**
 * \brief Function to allocate memory for mqtt packet payload.
 * \param pkt Pointer to the address of the packet containing payload.
 * \param type The type of payload to be created.
 * \return Length of packet payload
 */
int init_packet_payload(struct mqtt_packet *pkt, ctrl_pkt_type type) {

  unsigned int pay_len = 0;

  switch (type) {
    case CONNECT:

      /* allocate payload memory */
      if (!(pkt->payload = calloc(1, MQTT_MAX_PAYLOAD_LEN))) {
        printf("Error: Allocating space for payload.\n");
        //free_pkt_variable_header(pkt->variable);
      }

      struct utf8_enc_str *client_id = (struct utf8_enc_str *)&pkt->payload->data;

      memcpy(&client_id->utf8_str, &MQTT_CLIENT_ID, sizeof(MQTT_CLIENT_ID));

      client_id->length = sizeof(MQTT_CLIENT_ID) - 1;
      pkt->payload->length = sizeof(struct utf8_enc_str) + client_id->length - 1;

      break;

    case PUBLISH:

    default:
      printf("Error: MQTT packet type not currently supported.\n");
      return 0;
  }

  /* debug */
  printf("Length of payload: %d\n", pkt->payload->length);

  pkt->length += pkt->payload->length;

  encode_remaining_pkt_len(pkt, pkt->length + pkt->payload->length);

  return pkt->length;
}

/**
 * \brief Function to encode the remaining length of an MQTT packet, after the fixed header,
 *        into the fixed packet header - see section 2.2.3 of the MQTT spec.
 * \param pkt The packet whose length to encode.
 * \param len The length that should be encoded.
 */
void encode_remaining_pkt_len(struct mqtt_packet *pkt, unsigned int len) {
  int i = 0;
  do {
    pkt->fixed->remain_len[i] = len % 128;
    len /= 128;

    if (len > 0) {
      pkt->fixed->remain_len[i] |= 128;
    }
    i++;
  } while (len > 0 && i < 4);

  return;
}

/**
 * \brief Function to decode the remain_len variable in the fixed header of an MQTT packet
 *        into an int - see section 2.2.3 of the MQTT spec.
 * \param pkt The packet whose length to decode.
 * \return The length that should be encoded.
 */
unsigned int decode_remaining_pkt_len(struct mqtt_packet *pkt) {
  int i = 0;
  unsigned int len = 0;
  unsigned int product = 1;
  do {
    len += (pkt->fixed->remain_len[i] & 127) * product;

    if (product > 128*128*128) {
      printf("Error: Malformed remaining length.\n");
      return 0;
    }
    product *= 128;

  } while ((pkt->fixed->remain_len[i++] & 128) != 0 && i < 4);

  return len;
}

/**
 * \brief Function to print memory in hex.
 * \param ptr The memory to start printing.
 * \param bytes The number of bytes to print.
 */
void print_memory_bytes_hex(void *ptr, int bytes) {
  int i;

  printf("%d bytes starting at address 0x%X\n", (bytes + 1), &ptr);
  for (i = 0; i <= bytes; i++) {
    printf("0x%02X ", ((uint8_t *)ptr)[i]);
  }
  printf("\n");

  return;
}

/**
 * \brief Function to free memory allocated to struct mqtt_packet.
 * \param pkt The packet to free.
 */
void free_pkt(struct mqtt_packet *pkt) {
  if (pkt) {
    if (pkt->fixed) {
      free(pkt->fixed);
    }

    if (pkt->variable) {
      free(pkt->variable);
    }

    if (pkt->payload) {
      free(pkt->variable);
    }

    free(pkt);
  }

  return;
}
  
/**
 * \brief Function to free memory allocated to struct pkt_fixed_header.
 * \param fix The fixed header to free.
 */
void free_pkt_fixed_header(struct pkt_fixed_header *fix) {
  if (fix) {
    free(fix);
  }

  return;
}

/**
 * \brief Function to free memory allocated to struct pkt_variable_header.
 * \param var The variable header to free.
 */
void free_pkt_variable_header(struct pkt_variable_header *var) {
  if (var) {
    free(var);
  }

  return;
}

/**
 * \brief Function to free memory allocated to struct pkt_payload.
 * \param pld The payload to free.
 */
void free_pkt_payload(struct pkt_payload *pld) {
  if (pld) {
    free(pld);
  }

  return;
}
