//
// Created by morten on 11/4/17.
//

#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include "events.h"
#include "rdt.h"

#define NR_BUFS 5

FifoQueue queue_TtoN;   //Queue from Transport layer to Network Layer
FifoQueue queue_NtoT;   //Queue from Network Layer to Transport Layer


typedef struct {
    int station;        // Address of station
    int *connections;  // Addresses of connected stations
} forwarding_field;

typedef struct {
    int size;
    forwarding_field *table;
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
void signal_link_layer_if_allowed( int address, FifoQueue queue );

/* Fetch a segment from the network layer for transmission on the channel. */
void from_network_layer(datagram *d,  FifoQueue from_network_layer_queue, mlock_t *network_layer_lock);

/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(datagram *d,  FifoQueue for_network_layer_queue, mlock_t *network_layer_lock);

/* Allow the network layer to cause a network_layer_ready event. */
void enable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock);

/* Forbid the network layer from causing a network_layer_ready event. */
void disable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock);

void packet_to_string(segment *data, char *buffer);

void datagram_to_string(datagram *d, char *buffer);

FifoQueue *get_queue_NtoT();

FifoQueue *get_queue_TtoN();

boolean *get_network_layer_enabled();

void FakeNetworkLayer1();

void FakeNetworkLayer2();

void FakeNetworkLayer();

void init_forwardtable(forwarding_table *Table);

int round_robin(int *connections);

void dequeuePacket( segment *p, FifoQueue queue);

void dequeueData( datagram *d, FifoQueue queue);

void copyPackettoDatagram(datagram *d, segment *p);

#endif //NETWORK_LAYER_H
