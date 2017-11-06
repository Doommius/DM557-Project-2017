//
// Created by morten on 11/4/17.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rdt.h"
#include "subnetsupport.h"
#include "subnet.h"
#include "fifoqueue.h"
#include "debug.h"
#include "events.h"
#include "network_layer.h"


FifoQueue queue_TtoN;   //Queue from Transport layer to Network Layer
FifoQueue queue_NtoT;   //Queue from Network Layer to Transport Layer



/* Make sure all locks and queues are initialized properly */
void initialize_locks_and_queues(){

    queue_TtoN = InitializeFQ();
    queue_NtoT= InitializeFQ();
}

/* Where should a datagram be sent to. Transport layer knows only end hosts,
 * but we will need to inform the link layer where it should forward it to */
int forward(int toAddress){

}

/* Listen to relevant events for network layer, and act upon them */
void network_layer_main_loop(){
    long int events_we_handle = network_layer_allowed_to_send | data_from_transport_layer | data_for_link_layer | data_from_link_layer;
    event_t event;
    FifoQueueEntry e;
    datagram d;

    FifoQueue for_queue;
    FifoQueue from_queue;


    while (true) {
        Wait(&event, events_we_handle);
        switch (event.type) {
            case network_layer_allowed_to_send:

                break;

            case data_from_link_layer:


                break;
            case data_from_transport_layer:
                printf("Fik signal, laver datagram:\n");
                for_queue = (FifoQueue) get_queue_TtoN();
                from_queue = (FifoQueue) get_queue_NtoT();

                e = DequeueFQ(for_queue);
                strcpy(d.data, e->val);
                printf("Datagram data: %s\n", d.data);

                //printf("First: %s\n Last: %s\n", (char *) for_queue->first->val, (char *) for_queue->last->val);
                //TODO Det her skal ikke være her, det er bare en test
                printf("Signalerer transport laget\n");
                Signal(data_for_transport_layer, NULL);

                break;

        }
    }
}

/* If there is data that should be sent, this function will check that the
 * relevant queue is not empty, and that the link layer has allowed us to send
 * to the neighbour  */
void signal_link_layer_if_allowed(int address){

}





void packet_to_string(packet *data, char *buffer) {
    strncpy(buffer, (char *) data->data, MAX_PKT);
    buffer[MAX_PKT] = '\0';

}

void from_network_layer(packet *p, FifoQueue from_network_layer_queue, mlock_t *network_layer_lock) {
    FifoQueueEntry e;

    Lock(network_layer_lock);
    e = DequeueFQ(from_network_layer_queue);
    Unlock(network_layer_lock);

    if (!e) {
        logLine(error, "ERROR: We did not receive anything from the queue, like we should have\n");
    } else {
        memcpy(p, (char *) ValueOfFQE(e), sizeof(packet));
        free((void *) ValueOfFQE(e));
        DeleteFQE(e);
    }
}


void to_network_layer(packet *p, FifoQueue for_network_layer_queue, mlock_t *network_layer_lock) {
    char *buffer;
    Lock(network_layer_lock);

    buffer = (char *) malloc(sizeof(char) * (1 + MAX_PKT));
    packet_to_string(p, buffer);

    EnqueueFQ(NewFQE((void *) buffer), for_network_layer_queue);

    Unlock(network_layer_lock);

    Signal(data_for_network_layer, NULL);
}

void enable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock) {
    Lock(network_layer_lock);
    logLine(trace, "enabling network layer\n");
    network_layer_allowance_list[station] = true;
    Signal(network_layer_allowed_to_send, NULL);
    Unlock(network_layer_lock);
}

void disable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock) {
    Lock(network_layer_lock);
    logLine(trace, "disabling network layer\n");
    network_layer_allowance_list[station] = false;
    Unlock(network_layer_lock);
}


//TODO Bedre funktions navne
//TODO But how? aren't they descriptive as they are?

FifoQueue *get_queue_NtoT(){
    return (FifoQueue *) queue_NtoT;
}

FifoQueue *get_queue_TtoN(){
    return (FifoQueue *) queue_TtoN;
}

