//
// Created by jervelund on 11/21/17.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "transport_layer.h"
#include "subnetsupport.h"
#include "debug.h"
#include "network_layer.h"
#include "subnet.h"
#include "rdt.h"


mlock_t *network_layer_lock;
mlock_t *write_lock;

event_t event;

int connectionid; //do we even need this.

connection connections[NUM_CONNECTIONS];


/*
 * Listen for incoming calls on the specified transport_address.
 * Blocks while waiting
 */
int listen(transport_address local_address) {
    tpdu *packet;
    event_t event;
    FifoQueue from_queue;                /* Queue for data from network layer */
    FifoQueueEntry e;
    char *data;
    Lock(network_layer_lock);
    from_queue = (FifoQueue) get_queue_NtoT();

    long int events_we_handle = DATA_FOR_TRANSPORT_LAYER; //probably not, should be changed.

    while (true) {
        //TODO: do so it also waits for timeout.
        Wait(&event, events_we_handle);

        switch (event.type) {
            case DATA_FOR_TRANSPORT_LAYER:
                if ((int) event.msg == get_ThisStation()) {
                    Lock(transport_layer_lock);
                    e = DequeueFQ(queue_NtoT);
                    if (!e) {
                        printf("Error with queue");
                    } else {
                        memcpy(packet, ValueOfFQE(e), sizeof(packet));
                        free(ValueOfFQE(e));
                        DeleteFQ(e);
                        if (packet->bytes) {
                            printf("connection accepted");
                            Unlock(transport_layer_lock);
                            return 0;
                        } else {
                            printf("connection failed.");
                            Unlock(transport_layer_lock);
                            return -1;
                        }
                    }
                }
                break;
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
 * TODO: We may get issues due to using Connection ID as index in our connections array.
 * TODO: this could be fixed by using connection id modulo lenght of the buffer?
 */
int connect(host_address remote_host, transport_address local_ta, transport_address remote_ta) {
    Lock(transport_layer_lock);
    tpdu *data = malloc(sizeof(tpdu));

    data->type = call_req;
    data->returnport = local_ta;
    data->destport = remote_ta;
    data->dest = remote_host;

    EnqueueFQ(NewFQE(data), queue_TtoN);
    Signal(DATA_FOR_NETWORK_LAYER, NULL);
    Unlock(transport_layer_lock);

    if (listen(local_ta)) {
        for (int connection = 0; connection < NUM_CONNECTIONS; ++connection) {
            if (connections[connection].state == disconn) {
                connections[connection].state = established;
                connections[connection].local_address = local_ta;
                connections[connection].remote_address = remote_ta;
                connections[connection].remote_host = remote_host;
                connections[connection].id = connection;
                printf("Connection Established");
                //TODO Load all the informaton about the connection into some kind of data structure.
                return connection;
            }
        }
        printf("Too many current connections.\n");
        return -2; // Too many connections
    } else {
        printf("Connection failed.\n");
        return -1;
    }
}

/*
 * Disconnect the connection with the supplied id.
 * returns appropriate errorcode or 0 if successfull
 *
 * TODO; Do we need to clear the element from the connections array,
 * TODO; I'm thinking it'll be overwritten in the next iteration due to modulo calculation?
 *
 */
int disconnect(int connection_id) {
    Lock(transport_layer_lock);
    tpdu *data = malloc(sizeof(tpdu));

    connections[connection_id].state = disconn;

    data->type = clear_conf; //TODO maybe make custom type?
    data->destport = connections[connection_id].remote_address;
    EnqueueFQ(NewFQE(data), queue_TtoN);
    signal_link_layer_if_allowed(DATA_FOR_NETWORK_LAYER, NULL);
    Unlock(transport_layer_lock);
    return 0;

}

/*
 * Set up a connection, so it is ready to receive data on, and wait for the main loop to signal all data received.
 * Could have a timeout for a connection - and cleanup afterwards.
 */
int receive(host_address remote_host, unsigned char *buf, unsigned int *bufsize) {

    Wait(&event, DATA_FOR_APPLICATION_LAYER);

    segment *segment;

    segment = DequeueFQ(get_queue_NtoT)->val;
    memcpy(buf, segment->data.payload, bufsize);
    //dequeue from network to transport queue.

}

/*
 * On connection specified, send the bytes amount of data from buf.
 * Must break the message down into chunks of a manageable size, and queue them up.
 */
int send(int connection_id, unsigned char *buf, unsigned int bytes) {
    int num_packets;

    FifoQueue queue;
    queue = (FifoQueue) get_queue_TtoN();

    num_packets =  (bytes / TPDU_PAYLOAD_SIZE)+1;

    printf("Number of package we need to send: %i\n", num_packets);

    int overhead;
    overhead = (bytes % TPDU_PAYLOAD_SIZE);

    for (int i = 0; i < num_packets; i++) {

        tpdu *t;
        t = malloc(sizeof(tpdu));

        if (i < num_packets - 1) {
            memcpy(t->payload, buf+i*TPDU_PAYLOAD_SIZE, TPDU_PAYLOAD_SIZE);
            t->bytes = TPDU_PAYLOAD_SIZE;
        } else {
            memcpy(t->payload, buf+i*TPDU_PAYLOAD_SIZE, overhead);
            t->bytes = overhead;
        }

        printf("Message: %s\n", t->payload);
        t->dest = connection_id;

        EnqueueFQ(NewFQE((void *) t), queue);

        for (int j = 0; j < num_packets; j++) {
            Signal(DATA_FROM_TRANSPORT_LAYER, NULL);
        }
    }
}


/*
 * Main loop of the transport layer, handling the relevant events, fx. data arriving from the network layer.
 * And take care of the different types of packages that can be received
 */
void transport_layer_loop(void) {
    transport_layer_lock = malloc(sizeof(mlock_t));
    Init_lock(transport_layer_lock);

    long int events_we_handle;
    events_we_handle = DATA_FOR_TRANSPORT_LAYER | DATA_FROM_APPLICATION_LAYER;
    while (true){
        Wait(&event, events_we_handle);
        switch (event.type){
            case DATA_FOR_TRANSPORT_LAYER:
                printf("DATA FOR TRANSPORT LAYER");
                Lock(transport_layer_lock);



                Unlock(transport_layer_lock);
                break;
            case DATA_FROM_APPLICATION_LAYER:
                printf("DATA FROM APPLICATION LAYER");
                break;
        }
    }
}


