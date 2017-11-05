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



void initialize_locks_and_queues(){

}

int forward(int toAddress){

}

void network_layer_main_loop(){
    long int events_we_handle = network_layer_allowed_to_send | data_for_network_layer | data_from_transport_layer;
    event_t event;


    while (true) {
        Wait(&event, events_we_handle);
        switch (event.type) {
            case network_layer_allowed_to_send:
                break;
            case data_for_network_layer:
                break;
            case data_from_transport_layer:
                break;

        }
    }
}

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
