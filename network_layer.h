//
// Created by morten on 11/4/17.
//

#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include "events.h"
#include "rdt.h"

#define NR_BUFS 4

/* For now only DATAGRAM is used, but for dynamic routing, ROUTERINFO is defined */
typedef enum {DATAGRAM, ROUTERINFO} datagram_kind;        /* datagram_kind definition */

typedef struct {                        /* datagrams are transported in this layer */
    char data[128];   /* Data from the transport layer segment  */
    datagram_kind kind;                   /* what kind of a datagram is it? */
    int from;                                                /* From station address */
    int to;                                                /* To station address */
} datagram;

typedef struct {
    int station;        // Address of station
    int connections[];  // Addresses of connected stations
} forwarding_field;

typedef struct {
    int size;
    forwarding_field table[];
} forwarding_table;



/* Make sure all locks and queues are initialized properly */
void initialize_locks_and_queues();

/* Where should a datagram be sent to. Transport layer knows only end hosts,
 * but we will need to inform the link layer where it should forward it to */
int forward(int toAddress);

/* Listen to relevant events for network layer, and act upon them */
void network_layer_main_loop();

/* If there is data that should be sent, this function will check that the
 * relevant queue is not empty, and that the link layer has allowed us to send
 * to the neighbour  */
void signal_link_layer_if_allowed( int address);

/* Fetch a packet from the network layer for transmission on the channel. */
void from_network_layer(packet *p,  FifoQueue from_network_layer_queue, mlock_t *network_layer_lock);

/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(packet *p,  FifoQueue for_network_layer_queue, mlock_t *network_layer_lock);

/* Allow the network layer to cause a network_layer_ready event. */
void enable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock);

/* Forbid the network layer from causing a network_layer_ready event. */
void disable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock);

void packet_to_string(packet *data, char *buffer);
FifoQueue *get_queue_NtoT();

FifoQueue *get_queue_TtoN();

#endif //NETWORK_LAYER_H
