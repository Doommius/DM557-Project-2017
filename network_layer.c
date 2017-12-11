//
// Created by morten on 11/4/17.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "subnetsupport.h"
#include "subnet.h"
#include "debug.h"
#include "events.h"
#include "network_layer.h"
#include "structs.h"


FifoQueue queue_TtoN;   //Queue from Transport layer to Network Layer
FifoQueue queue_NtoT;   //Queue from Network Layer to Transport Layer

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
    thisTable->size = 4;
    //printf("Size of forwarding table: %i\n", thisTable->size);
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
    int i;

    //Initializing forwarding table
    init_forwardtable(&table);

    int ThisStation;
    ThisStation = get_ThisStation();

    if (ThisStation != 1 && ThisStation != 4){
        initialize_locks_and_queues();
    }

    FifoQueue for_queue;
    FifoQueue from_queue;

    from_queue = (FifoQueue) get_queue_NtoT();

    FifoQueue from_network_layer_queue;
    FifoQueue for_network_layer_queue;

    from_network_layer_queue = get_from_queue();
    for_network_layer_queue = get_for_queue();


    i = 0;

    printf("Starter main loop for station %i\n", ThisStation);

    while (true) {
        Wait(&event, events_we_handle);
        switch (event.type) {
            case NETWORK_LAYER_ALLOWED_TO_SEND:
                Lock(network_layer_lock);

                printf("NETWORK LAYER ALLOWED TO SEND\n");

                if (!EmptyFQ(from_network_layer_queue) && network_layer_enabled[ThisStation]) {
                    // Signal element is ready
                    logLine(info, "Sending signal for message #%d\n", i);
                    Signal(NETWORK_LAYER_READY, NULL);
                }
                Unlock(network_layer_lock);
                break;

            case DATA_FOR_NETWORK_LAYER:
                printf("Got message from link layer\n");
                Lock(network_layer_lock);

                datagram *d2;

                d2 = (datagram *) malloc(sizeof(datagram));

                dequeueData(d2, for_network_layer_queue);

                printf("");

                if(!d2){
                    break;
                }

                printf("GLOBALDESTINATION: %i\n", d2->globalDest);

                if (d2->globalDest == ThisStation ){

                    //printf("Got message for us, sending to transport layer. Message: %s\n", d2->data.data);
                    /*
                    segment *p2;
                    p2 = (segment *) malloc(sizeof(segment));
                    p2->source = d2->globalSource;
                    p2->dest = d2->globalDest;
                    */


                    EnqueueFQ(NewFQE((void *) &(d2->data)), from_queue);
                    printf("Enqueuet package til transport lag\n");
                    Signal(DATA_FOR_TRANSPORT_LAYER, NULL);

                } else {
                    int prevSource;
                    prevSource = d2->from;

                    d2->to = forward(d2->globalDest);
                    d2->from = ThisStation;
                    printf("Got message that was not for us: %i, from: %i, containing the message: %s. Forwarding to: %i\n", ThisStation, prevSource, d2->data.data.payload, d2->to);
                    EnqueueFQ(NewFQE((void *) d2), from_network_layer_queue);
                    signal_link_layer_if_allowed(d2->to, from_network_layer_queue);
                }

                Unlock(network_layer_lock);

                break;

            case DATA_FROM_TRANSPORT_LAYER:
                printf("DATA FROM TRANSPORT LAYER\n");

                Lock(network_layer_lock);
                for_queue = (FifoQueue) get_queue_TtoN();

                printf("Is queue empty? %i\n", EmptyFQ(for_queue));
                datagram *d;
                segment *s;

                s = (segment *) malloc(sizeof(segment));

                d = (datagram *) malloc(sizeof(datagram));


                dequeueSegment(s, for_queue);

                printf("Fik dequeuet, message: %s\n", s->data.payload);


                //printf("dequeuet segment, fra: %i, til %i, msg: %s\n", p->source, p->dest, p->data);



                //d->data = p;

                copySegmenttoDatagram(d, s);

                d->from = ThisStation;
                d->globalSource = ThisStation;
                d->globalDest = d->data.dest;
                d->to = forward(d->data.dest);
                //printf("Sending from: %i, to: %i, msg: %s\n", ThisStation, d->to, d->data.payload);

                printf("Queueing datagram with globaldest: %i, globalsource: %i\n", d->globalDest, d->globalSource);
                EnqueueFQ(NewFQE((void *) d), from_network_layer_queue);
                signal_link_layer_if_allowed(d->to, from_network_layer_queue);
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
void signal_link_layer_if_allowed(int address, FifoQueue queue) {

    printf("Signal link layer?\n");
    if (!EmptyFQ(queue) && network_layer_enabled[address])  {
        printf("Signalling link layer\n");
        Signal(NETWORK_LAYER_READY, NULL);
    }
}

void datagram_to_string(datagram *d, char *buffer){
    //strncpy(buffer, (char *) d->data.data, MAX_PKT);
    buffer[MAX_PKT] = '\0';
}


void packet_to_string(segment *data, char *buffer) {
    //strncpy(buffer, (char *) data->data, MAX_PKT);
    buffer[MAX_PKT] = '\0';
}

boolean *get_network_layer_enabled(){
    return network_layer_enabled;
}

void from_network_layer(datagram *d, FifoQueue from_network_layer_queue, mlock_t *network_layer_lock) {
    FifoQueueEntry e;

    Lock(network_layer_lock);
    e = DequeueFQ(from_network_layer_queue);
    Unlock(network_layer_lock);

    if (!e) {
        logLine(error, "ERROR: We did not receive anything from the queue, like we should have\n");
    } else {
        memcpy(d, (datagram *) ValueOfFQE(e), sizeof(datagram));
        free( (void *) ValueOfFQE(e));
        DeleteFQE(e);
    }
}

void to_network_layer(datagram *d, FifoQueue for_network_layer_queue, mlock_t *network_layer_lock) {
    Lock(network_layer_lock);

    datagram *d2;

    d2 = (datagram *) malloc(sizeof(datagram));
    memcpy(d2, d, sizeof(datagram));

/*
    d2->data = d->data;
    d2->from = d->from;
    d2->to = d->to;
    d2->kind = d->kind;
    d2->globalDest = d->globalDest;
*/

    //printf("Enqueueing datagram with message: %s, from: %i, to: %i\n", d2->data->data, d2->from, d2->to);
    EnqueueFQ(NewFQE((void *) d2), for_network_layer_queue);

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

void dequeueSegment(segment *s, FifoQueue queue){

    printf("Dequeuer\n");
    FifoQueueEntry e;
    e = DequeueFQ(queue);
    memcpy(s, e->val, sizeof(segment));
    DeleteFQE(e);
}

void dequeueData(datagram *d, FifoQueue queue){

    FifoQueueEntry e;
    e = DequeueFQ(queue);
    if (!e){
        printf("Nothing in queue for network layer\n");
    }
    else {
        memcpy(d, e->val, sizeof(datagram));
        DeleteFQE(e);
    }

}

void copySegmenttoDatagram(datagram *d, segment *s){
    segment *s2;
    s2 = malloc(sizeof(segment));
    memcpy(s2, s, sizeof(segment));
    d->data = *s2;
    //free(t);
}


/* Fake network/upper layers for station 2
 *
 * Receive 20 messages, take the first 10, lowercase first letter and send to station 1.
 * With this, some acks will be piggybacked, some will be pure acks.
 *
 **/
void FakeNetworkLayer2() {
    long int events_we_handle;
    event_t event;
    int j, ThisStation;
    FifoQueueEntry e;

    ThisStation = get_ThisStation();
    events_we_handle = NETWORK_LAYER_ALLOWED_TO_SEND | DATA_FOR_NETWORK_LAYER;

    j = 0;
    while (true) {
        // Wait until we are allowed to transmit
        Wait(&event, events_we_handle);

        switch (event.type) {
            case NETWORK_LAYER_ALLOWED_TO_SEND:
                Lock(network_layer_lock);
                if (network_layer_enabled[ThisStation] && !EmptyFQ(from_network_layer_queue)) {
                    logLine(info, "Signal from network layer for message\n");
                    network_layer_enabled[ThisStation] = false;
                    ClearEvent(NETWORK_LAYER_READY); // Don't want to signal too many events
                    Signal(NETWORK_LAYER_READY, NULL);
                }
                Unlock(network_layer_lock);
                break;
            case DATA_FOR_NETWORK_LAYER:
                Lock(network_layer_lock);

                e = DequeueFQ(for_network_layer_queue);
                logLine(succes, "Received message: %s\n", ((char *) e->val));

                if (j < 10) {
                    ((char *) e->val)[0] = 'd';


                    EnqueueFQ(e, from_network_layer_queue);
                }

                Unlock(network_layer_lock);


                j++;
                logLine(info, "j: %d\n", j);
                break;
        }
        if (EmptyFQ(from_network_layer_queue) && j >= 20) {
            logLine(succes, "Stopping - received 20 messages and sent 10\n");
            sleep(5);
            Stop();
        }

    }
}


/* Fake network/upper layers for station 1
 *
 * Send 20 packets and receive 10 before we stop
 * */
void FakeNetworkLayer1() {
    char *buffer;
    int i, j, ThisStation;
    long int events_we_handle;
    event_t event;
    FifoQueueEntry e;

    ThisStation = get_ThisStation();
    // Setup some messages
    for (i = 0; i < 20; i++) {
        buffer = (char *) malloc(sizeof(char) * MAX_PKT);
        sprintf(buffer, "D: %d", i);
        EnqueueFQ(NewFQE((void *) buffer), from_network_layer_queue);
    }

    events_we_handle = NETWORK_LAYER_ALLOWED_TO_SEND | DATA_FOR_NETWORK_LAYER;

    // Give selective repeat a chance to start
    sleep(2);

    i = 0;
    j = 0;
    while (true) {
        // Wait until we are allowed to transmit
        Wait(&event, events_we_handle);
        switch (event.type) {
            case NETWORK_LAYER_ALLOWED_TO_SEND:
                Lock(network_layer_lock);
                if (i < 20 && network_layer_enabled[ThisStation]) {
                    // Signal element is ready
                    logLine(info, "Sending signal for message #%d\n", i);
                    network_layer_enabled[ThisStation] = false;
                    Signal(NETWORK_LAYER_READY, NULL);
                    i++;
                }
                Unlock(network_layer_lock);
                break;
            case DATA_FOR_NETWORK_LAYER:
                Lock(network_layer_lock);

                e = DequeueFQ(for_network_layer_queue);
                logLine(succes, "Received message: %s\n", ((char *) e->val));

                Unlock(network_layer_lock);

                j++;
                break;
        }

        if (i >= 20 && j >= 10) {
            logLine(info, "Station %d done. - (\'sleep(5)\')\n", ThisStation);
            /* A small break, so all stations can be ready */
            sleep(5);
            Stop();
        }
    }
}

void FakeNetworkLayer(){
    sleep(2);
    network_layer_main_loop();
}