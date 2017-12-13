//
// Created by jervelund on 11/21/17.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "transport_layer.h"
#include "subnetsupport.h"
#include "debug.h"
#include "network_layer.h"
#include "subnet.h"
#include "structs.h"


mlock_t *network_layer_lock;
mlock_t *write_lock;

FifoQueue listen_queue;

//Queue for data to the application layer
FifoQueue app_queue;

event_t event;

int connectionid; //do we even need this.
int num_parts;

connection connections[NUM_CONNECTIONS];


/*
 * Listen for incoming calls on the specified transport_address.
 * Blocks while waiting
 */
int listen(transport_address local_address) {
    printf("Listen starter\n");
    tpdu *t;
    event_t event;
    FifoQueueEntry e;

    long int events_we_handle = CONNECTION_REPLY; //probably not, should be changed.
    connections[connectionid].state = waiting;

    while (true) {

        printf("Waiting for event\n");
        //TODO: do so it also waits for timeout. -Mark
        Wait(&event, events_we_handle);

        switch (event.type) {
            case CONNECTION_REPLY:
                Lock(transport_layer_lock);

                e = DequeueFQ(listen_queue);
                t = ValueOfFQE(e);
                printf("Checking port vs local address. port: %i, local address: %i\n", t->destport, local_address);
                if (t->destport == local_address){
                    //Good connection
                    DeleteFQE(e);
                    Unlock(transport_layer_lock);
                    return 1;
                }
                //Bad connection
                DeleteFQE(e);
                Unlock(transport_layer_lock);
                return -1;
        }
    }
}


/*
 * Try to connect to remote host, on specified transport_address,
 * listening back on local transport address. (full duplex).
 *
 * Once you have used the connection to send(), you can then be able to receive
 * Returns the connection id - or an appropriate error code
 *
 *
 * TODO: We may get issues due to using Connection ID as index in our connections array. -Mark
 * TODO: this could be fixed by using connection id modulo lenght of the buffer? -Mark
 */
int connect(host_address remote_host, transport_address local_ta, transport_address remote_ta) {


    printf("Connecting to: %i, local ta: %i, remote ta; %i\n", remote_host, local_ta, remote_ta);
        for (int connection = 0; connection < NUM_CONNECTIONS; ++connection) {
            if (connections[connection].state == disconn) {

                printf("Found empty connection\n");
                Lock(transport_layer_lock);
                tpdu *notif = malloc(sizeof(tpdu));

                segment *s;
                s = (segment *) malloc(sizeof(segment));

                notif->type = call_req;
                notif->returnport = local_ta;
                notif->destport = remote_ta;
                notif->dest = remote_host;
                notif->source = get_ThisStation();

                FifoQueue queue;
                queue = (FifoQueue) get_queue_TtoN();

                //Request Connection

                copyTPDUtoSegment(notif, s);

                EnqueueFQ(NewFQE((void *) s), queue);
                Signal(DATA_FROM_TRANSPORT_LAYER, NULL);
                Unlock(transport_layer_lock);

                printf("Waiting for listen\n");
                //TODO check if we get reply if no, return error. -Mark
                if (listen(local_ta)) {
                    connections[connection].state = established;
                    connections[connection].local_address = local_ta;
                    connections[connection].remote_address = remote_ta;
                    connections[connection].remote_host = remote_host;
                    connections[connection].id = connection;
                    connectionid = connection;
                    //TODO Load all the informaton about the connection into some kind of notif structure.
                    printf("Connection Established\n");
                    return connection;
                }


            }
        }
        printf("Too many current connections.\n");
        return -1; // Connection failed


}

/*
 * Disconnect the connection with the supplied id.
 * returns appropriate errorcode or 0 if successfull
 */
int disconnect(int connection_id) {
    //printf("Waiting for lock\n");
    Lock(transport_layer_lock);
    printf("Disconnecting %i from %i\n", get_ThisStation(), connection_id);
    tpdu *t;
    t = malloc(sizeof(tpdu));

    FifoQueue queue;
    queue = (FifoQueue) get_queue_TtoN();

    segment *s;
    s = (segment *) malloc(sizeof(segment));

    connections[connection_id].state = disconn;

    t->type = clear_conf;
    t->destport = connections[connection_id].remote_address;

    copyTPDUtoSegment(t, s);

    EnqueueFQ(NewFQE( (void *) s), queue);
    Signal(DATA_FROM_TRANSPORT_LAYER, NULL);

    Unlock(transport_layer_lock);
    return 0;

}

/*
 * Set up a connection, so it is ready to receive data on, and wait for the main loop to signal all data received.
 * Could have a timeout for a connection - and cleanup afterwards.
 */
int receive(host_address remote_host, unsigned char *buf, unsigned int *bufsize) {


    long int events_we_handle = DATA_FOR_APPLICATION_LAYER;
    connections[connectionid].state = receiving;

    char *msg;
    msg = malloc(bufsize);

    tpdu *t;
    t = malloc(sizeof(tpdu));

    FifoQueueEntry e;

    printf("Waiting for message\n");
    while (true){
        Wait(&event, events_we_handle);
        switch (event.type){
            case DATA_FOR_APPLICATION_LAYER:
                printf("Got signal\n");
                if (check_connection(remote_host)) {
                    if (!EmptyFQ(app_queue)) {
                        for (int i = 0; i < num_parts; i++) {
                            e = DequeueFQ(app_queue);
                            t = ValueOfFQE(e);
                            printf("Fik dequet msg: %s, i: %i\n", t->payload, i);
                            if (i == 0) {
                                strcpy(msg, t->payload);
                            } else {
                                strcat(msg, t->payload);
                            }
                        }
                        //Recreated message, return success
                        memcpy(buf, msg, strlen(msg));
                        connections[connectionid].state = established;
                        return 1;
                    }
                }
                //No connection, return 0
                connections[connectionid].state = established;
                return 0;
                break;
        }
    }
}

/*
 * On connection specified, send the bytes amount of data from buf.
 * Must break the message down into chunks of a manageable size, and queue them up.
 */
int send(int connection_id, unsigned char *buf, unsigned int bytes) {
    if(check_connection(connection_id)) {

        printf("CONNETION ID: %i\n", connection_id);
        connections[connection_id].state = sending;
        int num_packets;

        FifoQueue queue;
        queue = (FifoQueue) get_queue_TtoN();

        num_packets = (bytes / TPDU_PAYLOAD_SIZE) + 1;

        printf("Number of packages we need to send: %i\n", num_packets);

        int overhead;
        overhead = (bytes % TPDU_PAYLOAD_SIZE);


        tpdu *t;
        t = (tpdu *) malloc(sizeof(tpdu));

        segment *s;
        s = (segment *) malloc(sizeof(segment));


        t->dest = connection_id;
        t->source = get_ThisStation();
        t->destport = connections[connection_id].remote_host;
        t->type = data_notiff;
        t->part = num_packets;

        copyTPDUtoSegment(t, s);
        EnqueueFQ(NewFQE( (void *) s), queue);
        printf("sending notification\n");
        Signal(DATA_FROM_TRANSPORT_LAYER, NULL);
        sleep(1);

        //TODO find a way to number what part of the message we are sending, so we can rebuild it later
        for (int i = 0; i < num_packets; i++) {
            segment *s2;
            s2 = (segment *) malloc(sizeof(segment));

            tpdu *t2;
            t2 = (tpdu *) malloc(sizeof(tpdu));

            if (i < num_packets - 1) {
                memcpy(t2->payload, buf + i * TPDU_PAYLOAD_SIZE, TPDU_PAYLOAD_SIZE);
                t2->bytes = TPDU_PAYLOAD_SIZE;
            } else {
                memcpy(t2->payload, buf + i * TPDU_PAYLOAD_SIZE, overhead);
                t2->bytes = overhead;
            }


            printf("Number of bytes we send: %i, message we send: %s\n", t2->bytes, t2->payload);

            t2->dest = connection_id;
            t2->source = get_ThisStation();
            t2->destport = connections[connection_id].remote_host;
            t2->returnport = connections[connection_id].local_address;
            t2->part = i;
            t2->type = data_tpdu;

            copyTPDUtoSegment(t2, s2);

            EnqueueFQ(NewFQE((void *) s2), queue);

        }

        printf("Sending message\n");
        for (int j = 0; j < num_packets; j++) {
            Signal(DATA_FROM_TRANSPORT_LAYER, NULL);

        }

        connections[connection_id].state = established;
        return 1;
    }
    //Error - No connection, so we don't send
    connections[connection_id].state = established;
    return -1;

}


/*
 * Main loop of the transport layer, handling the relevant events, fx. data arriving from the network layer.
 * And take care of the different types of packages that can be received
 */
void transport_layer_loop(void) {
    initialize_locks_and_queues();
    transport_layer_lock = malloc(sizeof(mlock_t));
    Init_lock(transport_layer_lock);


    int index, counter = 0;
    connectionid = 0;
    FifoQueue from_queue, for_queue;

    listen_queue = InitializeFQ();
    app_queue = InitializeFQ();

    segment *s;
    s = (segment *) malloc(sizeof(segment));

    long int events_we_handle;
    events_we_handle = DATA_FOR_TRANSPORT_LAYER;

    while (true) {
        Wait(&event, events_we_handle);
        switch (event.type) {
            case DATA_FOR_TRANSPORT_LAYER:
                printf("DATA FOR TRANSPORT LAYER\n");
                Lock(transport_layer_lock);
                from_queue = (FifoQueue) get_queue_NtoT();
                for_queue = (FifoQueue) get_queue_TtoN();

                tpdu *t;
                FifoQueueEntry e;
                e = DequeueFQ(from_queue);

                t = ValueOfFQE(e);

                printf("Checking tpdu type\n");
                switch (t->type){
                    case call_req:
                        printf("Connection request\n");
                        if (check_ports(t->destport)){
                            printf("Port good: %i\n", t->destport);
                            connections[connectionid].state = established;
                            connections[connectionid].local_address = t->destport;
                            connections[connectionid].remote_address = t->returnport;
                            connections[connectionid].remote_host = t->dest;
                            connections[connectionid].id = connectionid;

                            //Send reply back
                            t->type = call_rep;
                            swapSandD(t);

                            printf("Sending reply:\n");
                            copyTPDUtoSegment(t, s);

                            EnqueueFQ(NewFQE((void *) s), for_queue);
                            Signal(DATA_FROM_TRANSPORT_LAYER, NULL);

                        } else {
                            printf("Port bad: %i\n", t->destport);
                            t->type = call_rep;
                            t->destport = -1;

                            swapSandD(t);

                            copyTPDUtoSegment(t, s);

                            EnqueueFQ(NewFQE((void *) s), for_queue);
                            Signal(DATA_FROM_TRANSPORT_LAYER, NULL);
                        }
                        break;

                    case call_rep:
                        printf("Replying connection\n");
                        EnqueueFQ(NewFQE((void *) t), listen_queue);
                        Signal(CONNECTION_REPLY, NULL);
                        break;

                    case clear_conf:
                        printf("Clear connection\n");
                        index = check_connection(t->destport);
                        connections[index].state = disconn;
                        break;

                    case data_tpdu:
                        //Got (part of) message
                        printf("Data for application layer\n");
                        tpdu *t2;
                        t2 = malloc(sizeof(tpdu));
                        memcpy(t2, t, sizeof(tpdu));
                        printf("Queing data\n");
                        EnqueueFQ(NewFQE((void *) t2), app_queue);
                        counter++;
                        printf("Counter: %i\n", counter);

                        if(counter == num_parts){
                            //Recieved all parts of message
                            printf("Signalling receive\n");
                            Signal(DATA_FOR_APPLICATION_LAYER, NULL);
                        }
                        break;

                    case data_notiff:
                        //Notification for how many parts the message is split up in
                        printf("Notification for data\n");
                        num_parts = t->part;
                        printf("Going to get a message consisting of %i parts\n", num_parts);
                        break;
                }

                DeleteFQE(e);
                Unlock(transport_layer_lock);
                break;
        }
    }
}

//Check if connection exists, return what index it is
int check_connection(int connection_id){
    for (int i = 0; i < NUM_CONNECTIONS; i++) {
        if (connections[i].id == connection_id){
            return i;
        }
    }
    //Error - No connection
    return -1;
}

//Check if port is available
int check_ports(transport_address port){
    for (int i = 0; i < NUM_CONNECTIONS; i++) {
        if (port == connections[i].local_address){
            //Port we wanted to send to  was already in use, can't connect
            return 0;
        }
    }
    return 1;
}

void copyTPDUtoSegment(tpdu *t, segment *s){
    tpdu *t2;
    t2 = (tpdu *) malloc(sizeof(tpdu));
    memcpy(t2, t, sizeof(tpdu));
    s->data = *t2;
    s->dest = t2->dest;
    s->source = t2->source;
    //printf("sending segment to: %i, from %i\n", s->dest, s->source);
    free(t);
}

//Swaps around a tpdu's source and destination
void swapSandD(tpdu *t){
    int prevD = 0 , prevS = 0;
    prevD = t->dest;
    prevS = t->source;
    t->dest = prevS;
    t->source = prevD;
}
