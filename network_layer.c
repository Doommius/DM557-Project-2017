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
FifoQueue queue_LtoN;   //Queue from Link Layer to Network Layer
FifoQueue queue_NtoL;   //Queue from Network Layer to Link Layer

mlock_t *network_layer_lock;
mlock_t *write_lock;


boolean network_layer_enabled[NR_BUFS];

/*
static forwarding_table table[] {
    {1, {2, 3},},
    {2, {1, 4},},
    {3, {1, 4},},
    {4, {2, 3},},
};
*/

/* Make sure all locks and queues are initialized properly */
void initialize_locks_and_queues() {

    queue_TtoN = InitializeFQ();
    queue_NtoT = InitializeFQ();
    queue_LtoN = InitializeFQ();
    queue_NtoL = InitializeFQ();


    write_lock = malloc(sizeof(mlock_t));
    network_layer_lock = (mlock_t *) malloc(sizeof(mlock_t));

    Init_lock(write_lock);
    Init_lock(network_layer_lock);
}


/*
 * Initializes a global table, that all stations can read. The table is hardcoded
 * and
 */
void init_forwardtable(forwarding_table *thisTable) {
    thisTable->table = (forwarding_field *) malloc(sizeof(forwarding_field) * 4);

    for (int i = 0; i < 4; ++i) {
        thisTable->table[i].connections = (int *) malloc(sizeof(int) * 2);
    }

    thisTable->table[0].station = 1;
    thisTable->table[0].connections[0] = 2;
    thisTable->table[0].connections[1] = 3;

    thisTable->table[1].station = 2;
    thisTable->table[1].connections[0] = 1;
    thisTable->table[1].connections[1] = 4;

    thisTable->table[2].station = 3;
    thisTable->table[2].connections[0] = 1;
    thisTable->table[2].connections[1] = 4;

    thisTable->table[3].station = 4;
    thisTable->table[3].connections[0] = 2;
    thisTable->table[3].connections[1] = 3;
//    thisTable->table = (struct forwarding_field){{1, {2, 3}},
//                        {2, {1, 4}},
//                        {3, {1, 4}},
//                        {4, {2, 3}}};

//    table->table[0] = {1, {2, 3}};
//    table->table[1] = {2, {1, 4}};
//    table->table[2] = {3, {1, 4}},
//            table->table[3] =               {4, {2, 3}};

    thisTable->size = sizeof(thisTable->table) / sizeof(forwarding_field);
    printf("Size of forwarding table: %i\n", thisTable->size);
}

/**
 * Randomly send the packages to one of the connecting hosts.
 * @param connections List of all outgoing connections for the host.
 * @return Host that the package should be sent too.
 */
int round_robin(int connections[]) {
    int random_index = rand() % (sizeof(connections) / sizeof(connections[0]));
    return connections[random_index];
}

/* Where should a datagram be sent to. Transport layer knows only end hosts,
 * but we will need to inform the link layer where it should forward it to */

int forward(int toAddress) {
    forwarding_table table;

    // Initialize forwarding table
    init_forwardtable(&table);

    // Find the station
    for (int station = 0; station < table.size; ++station) {
        if (get_ThisStation() == table.table[station].station) {
            for (int connection = 0; connection < (sizeof(table.table[station].connections) /
                                                   table.table[station].connections[0]); ++connection) {
                if (table.table[station].connections[connection] == toAddress) {
                    return toAddress;
                }
            }
            return round_robin(table.table[station].connections);
        }
    }
}


/* Listen to relevant events for network layer, and act upon them */
void network_layer_main_loop() {
    long int events_we_handle = NETWORK_LAYER_ALLOWED_TO_SEND | DATA_FOR_NETWORK_LAYER | DATA_FROM_TRANSPORT_LAYER | DONE_SENDING;
    event_t event;
    FifoQueueEntry e;
    datagram d;
    int i, j;

    int ThisStation;
    ThisStation = get_ThisStation();

    FifoQueue for_queue;
    FifoQueue from_queue;

    FifoQueue from_network_layer_queue;
    FifoQueue for_network_layer_queue;

    from_network_layer_queue = get_from_queue();
    for_network_layer_queue = get_for_queue();


    i = 0;
    j = 0;
    while (true) {
        Wait(&event, events_we_handle);
        switch (event.type) {
            case NETWORK_LAYER_ALLOWED_TO_SEND:
                Lock(network_layer_lock);

                //printf("Network layer allowed to send\n");

                if (network_layer_enabled[ThisStation] && !EmptyFQ(from_network_layer_queue)) {
                    // Signal element is ready
                    logLine(info, "Sending signal for message #%d\n", i);
                    network_layer_enabled[ThisStation] = false;
                    Signal(NETWORK_LAYER_READY, NULL);
                }
                Unlock(network_layer_lock);
                //printf("Kom ud af lock\n");
                break;

            case DATA_FOR_NETWORK_LAYER:
                //printf("Data for network layer\n");

                Lock(network_layer_lock);
                for_network_layer_queue = get_for_queue();
                from_queue = (FifoQueue) get_queue_NtoT();

                packet *p2;

                e = DequeueFQ(for_network_layer_queue);

                p2 = e->val;

                //Test til at se hvor den vil forwarde hen:
                //printf("Pakke fra: %i til: %i, skal forwardes igennem: %i\n", p2->source, p2->dest, forward(p2->dest));



                EnqueueFQ(NewFQE((void *) p2), from_queue);

                //printf("Signalerer transport laget\n");

                Signal(DATA_FOR_TRANSPORT_LAYER, NULL);

                Unlock(network_layer_lock);

                break;

            case DATA_FROM_TRANSPORT_LAYER:
                //printf("Fik signal, laver datagram:\n");

                Lock(network_layer_lock);
                for_queue = (FifoQueue) get_queue_TtoN();
                from_queue = (FifoQueue) get_queue_NtoT();

                //TODO Skal ikke være en packet, men et datagram
                packet *p;

                e = DequeueFQ(for_queue);

                p = e->val;

                //Test til at se hvor den vil forwarde hen:
                printf("Pakke fra: %i til: %i, skal forwardes igennem: %i\n", p->source, p->dest, forward(p->dest));


                EnqueueFQ(e, from_network_layer_queue);
                signal_link_layer_if_allowed(p->dest);
                Unlock(network_layer_lock);

                //printf("Gjort klar til selective repeat\n");
                break;

            case DONE_SENDING:
                sleep(5);
                Stop();
                break;

        }

    }
}

/*
 * If there is data that should be sent, this function will check that the
 * relevant queue is not empty, and that the link layer has allowed us to send
 * to the neighbour
 */
void signal_link_layer_if_allowed(int address) {
    FifoQueue queue;
    queue = get_from_queue();
    int ThisStation;
    ThisStation = get_ThisStation();

    //printf("Network layer enabled for address? %i. Network layer enabled for ThisStation? %i\n", network_layer_enabled[address], network_layer_enabled[ThisStation]);

    if (!EmptyFQ(queue) && network_layer_enabled[ThisStation]) {
        Signal(NETWORK_LAYER_ALLOWED_TO_SEND, NULL);
    }
}


void packet_to_string(packet *data, char *buffer) {
    strncpy(buffer, (char *) data->data, MAX_PKT);
    buffer[MAX_PKT] = '\0';
}

void from_network_layer(packet *p, FifoQueue from_network_layer_queue, mlock_t *network_layer_lock) {
    FifoQueueEntry e;

    Lock(network_layer_lock);
    packet *p2;
    p2 = from_network_layer_queue->first->val;

    //printf("from_network_layer, packet data: %s\n", p2->data);

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

    //printf("Packet info %s\n", p->data);

    buffer = (char *) malloc(sizeof(char) * (1 + MAX_PKT));
    packet_to_string(p, buffer);

    EnqueueFQ(NewFQE((void *) buffer), for_network_layer_queue);


    Unlock(network_layer_lock);

    Signal(DATA_FOR_NETWORK_LAYER, NULL);
}

void enable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock) {
    Lock(network_layer_lock);
    logLine(trace, "enabling network layer\n");
    network_layer_allowance_list[station] = true;
    Signal(NETWORK_LAYER_ALLOWED_TO_SEND, NULL);
    Unlock(network_layer_lock);
}

void disable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock) {
    Lock(network_layer_lock);
    logLine(trace, "disabling network layer\n");
    network_layer_allowance_list[station] = false;
    Unlock(network_layer_lock);
}

/**
 * NetworktoTransport
 * @return
 */
FifoQueue *get_queue_NtoT() {
    return (FifoQueue *) queue_NtoT;
}

/**
 * TransporttoNetwork
 * @return
 */
FifoQueue *get_queue_TtoN() {
    return (FifoQueue *) queue_TtoN;
}

