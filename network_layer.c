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

FifoQueue from_network_layer_queue;                /* Queue for data from network layer */
FifoQueue for_network_layer_queue;    /* Queue for data for the network layer */

mlock_t *network_layer_lock;
mlock_t *write_lock;

mlock_t *link_layer_lock;

forwarding_table table;


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

    from_network_layer_queue = InitializeFQ();
    for_network_layer_queue = InitializeFQ();


    write_lock = malloc(sizeof(mlock_t));
    network_layer_lock = (mlock_t *) malloc(sizeof(mlock_t));
    link_layer_lock = (mlock_t *) malloc(sizeof(mlock_t));

    Init_lock(write_lock);
    Init_lock(network_layer_lock);
    Init_lock(link_layer_lock);
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

    thisTable->size = 4;
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


    // Find the station
    for (int station = 0; station < table.size; ++station) {
        if (get_ThisStation() == table.table[station].station) {
            for (int connection = 0; connection < 2; ++connection) {
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
    //initialize_locks_and_queues();
    long int events_we_handle = NETWORK_LAYER_ALLOWED_TO_SEND | DATA_FOR_NETWORK_LAYER | DATA_FROM_TRANSPORT_LAYER | DONE_SENDING;
    event_t event;
    FifoQueueEntry e;
    datagram d;
    int i, j;

    //Initializing forwarding table
    init_forwardtable(&table);



    int ThisStation;
    ThisStation = get_ThisStation();

    if (ThisStation != 1 && ThisStation != 4){
        initialize_locks_and_queues();
    }

    FifoQueue for_queue;
    FifoQueue from_queue;

    for_queue = (FifoQueue) get_queue_TtoN();
    from_queue = (FifoQueue) get_queue_NtoT();

    FifoQueue from_network_layer_queue;
    FifoQueue for_network_layer_queue;

    from_network_layer_queue = get_from_queue();
    for_network_layer_queue = get_for_queue();


    i = 0;
    j = 0;

    //printf("Starter main loop for station %i\n", ThisStation);

    while (true) {
        Wait(&event, events_we_handle);
        switch (event.type) {
            case NETWORK_LAYER_ALLOWED_TO_SEND:
                Lock(network_layer_lock);

                printf("Network layer allowed to send\n");

                if (network_layer_enabled[ThisStation] && !EmptyFQ(from_network_layer_queue)) {
                    printf("Signaller NETWORK_LAYER_READY\n");
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

                printf("DATA FOR NETWORK LAYER\n");
                Lock(network_layer_lock);
                printf("Fik lock\n");

                //for_network_layer_queue = get_for_queue();
                //from_network_layer_queue = get_from_queue();
                //from_queue = (FifoQueue) get_queue_NtoT();

                packet *p2;
                datagram *d2;

                e = DequeueFQ(for_network_layer_queue);
                printf("Fik dequeuet\n");

                d2 = e->val;

                printf("Fik datagram\n");
                p2 = d2->data;
                printf("p2 source: %i, dest: %i, current station: %i\n", p2->source, p2->dest, ThisStation);

                if (p2->dest == ThisStation){
                    //Got message to us from link layer
                    printf("Fik besked til os fra link lag, data: %s, source: %i, dest: %i\n", p2->data, p2->source, p2->dest);

                    //disable_network_layer(ThisStation, network_layer_enabled, link_layer_lock);
                    //printf("Enqueuer ny packet til transport laget\n");
                    EnqueueFQ(NewFQE((void *) p2), from_queue);
                    Signal(DATA_FOR_TRANSPORT_LAYER, NULL);
                } else {
                    d2->to = forward(p2->dest);
                    d2->from = ThisStation;
                    printf("Fik besked der ikke var til os i link lag, sender videre. Source: %i, Dest: %i\n", d2->from, d2->to);
                    EnqueueFQ(NewFQE((void *) d2), from_network_layer_queue);
                    signal_link_layer_if_allowed(d2->to);
                }

                Unlock(network_layer_lock);

                break;

            case DATA_FROM_TRANSPORT_LAYER:
                printf("Fik signal, laver datagram:\n");

                Lock(network_layer_lock);
                for_queue = (FifoQueue) get_queue_TtoN();
                from_queue = (FifoQueue) get_queue_NtoT();


                //TODO Skal ikke være en packet, men et datagram
                packet *p;
                datagram *d;
                e = DequeueFQ(for_queue);

                d = (datagram *) malloc(sizeof(datagram));

                p = e->val;
                d->data = p;
                d->from = ThisStation;
                //d->to = p->dest;
                d->to = forward(p->dest);

                printf("Sender besked fra transport laget, fra: %i, til: %i\n", d->from, d->to);

                //printf("Størrelse af datagram: %i\n", sizeof(datagram));

                EnqueueFQ(NewFQE((void *) d), from_network_layer_queue);
                signal_link_layer_if_allowed(d->to);
                Unlock(network_layer_lock);

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

    printf("Network layer enabled for address? %i. Network layer enabled for ThisStation? %i\n", network_layer_enabled[address], network_layer_enabled[ThisStation]);

    //if (!EmptyFQ(queue) && network_layer_enabled[ThisStation]) {
    if (!EmptyFQ(queue)) {
        enable_network_layer(address, network_layer_enabled, link_layer_lock);
        //printf("Signalling NETWORK_LAYER_ALLOWED_TO_SEND, empty queue? %i, network_layer_enabled? %i\n", EmptyFQ(queue), network_layer_enabled[ThisStation]);
        //Signal(NETWORK_LAYER_ALLOWED_TO_SEND, NULL);
    }
}

void datagram_to_string(datagram *d, char *buffer){
    strncpy(buffer, (char *) d->data->data, MAX_PKT);
    buffer[MAX_PKT] = '\0';
}


void packet_to_string(packet *data, char *buffer) {
    strncpy(buffer, (char *) data->data, MAX_PKT);
    buffer[MAX_PKT] = '\0';
}

void from_network_layer(datagram *d, FifoQueue from_network_layer_queue, mlock_t *network_layer_lock) {
    FifoQueueEntry e;

    Lock(network_layer_lock);
    e = DequeueFQ(from_network_layer_queue);
    Unlock(network_layer_lock);

    if (!e) {
        logLine(error, "ERROR: We did not receive anything from the queue, like we should have\n");
    } else {
        memcpy(d, (char *) ValueOfFQE(e), sizeof(datagram));

        //printf("Value of datagram BEFORE free: Source: %i, Dest: %i, Info: %s\n", d->from, d->to, d->data->data);
        free((void *) ValueOfFQE(e));

        //printf("Value of datagram AFTER free: Source: %i, Dest: %i, Info: %s\n", d->from, d->to, d->data->data);

        DeleteFQE(e);

        //printf("Value of datagram AFTER DELETE: Source: %i, Dest: %i, Info: %s\n", d->from, d->to, d->data->data);


    }
}

void to_network_layer(datagram *d, FifoQueue for_network_layer_queue, mlock_t *network_layer_lock) {
    char *buffer;
    Lock(network_layer_lock);

    //printf("Packet info %s, source: %i, dest: %i\n",p->data, p->source, p->dest);
    datagram *d2;

    d2 = (datagram *) malloc(sizeof(datagram));

    d2->data = d->data;
    d2->from = d->from;
    d2->to = d->to;
    d2->kind = d->kind;
    printf("to_network_layer, source: %i, dest: %i, data: %s\n", d2->from, d2->to, d2->data->data);


    //printf("Queuer ny packet til for_network_layer_queue\n");
    EnqueueFQ(NewFQE((void *) d2), for_network_layer_queue);

    Unlock(network_layer_lock);

    Signal(DATA_FOR_NETWORK_LAYER, NULL);
}

void enable_network_layer(int station, boolean network_layer_allowance_list[], mlock_t *network_layer_lock) {
    Lock(network_layer_lock);
    logLine(trace, "enabling network layer\n");
    network_layer_allowance_list[station] = true;

    printf("Allowed network layer for station: %i, %i\n", station, network_layer_allowance_list[station]);
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

