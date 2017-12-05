//
// Created by jervelund on 11/21/17.
//

#include "transport_layer.h"
#include "subnetsupport.h"
#include "debug.h"
#include "network_layer.h"
#include "subnet.h"
#include <immintrin.h>
#include "rdt.h"


mlock_t *network_layer_lock;
mlock_t *write_lock;

connection connections[NUM_CONNECTIONS];


/*
 * Listen for incomming calls on the specified transport_address.
 * Blocks while waiting
 */
int listen(transport_address local_address) {
    event_t event;
    FifoQueue from_queue;                /* Queue for data from network layer */
    FifoQueueEntry e;
    char* data;
    Lock(network_layer_lock);
    from_queue = (FifoQueue) get_queue_NtoT();

    long int events_we_handle = DATA_FOR_TRANSPORT_LAYER;

    //This is a holding function for now.
    boolean shouldbreak = true;
    while (true) {

        //TODO: do so it also waits for timeout.
        Wait(&event, events_we_handle);

//        receive("test", data, 1000);

        logLine(succes, "Received message: %s\n", data);

        //something something done listening.
        if(false){
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
 */
int connect(host_address remote_host, transport_address local_ta, transport_address remote_ta) {
    int response;

    response = listen(remote_ta);
}

/*
 * Disconnect the connection with the supplied id.
 * returns appropriate errorcode or 0 if successfull
 */
int disconnect(int connection_id) {
    Lock(transport_layer_lock);
    tpdu_t *data = malloc(sizeof(tpdu_t));
    data->type = clear_conf;
    data->destport = connections[connection_id].remote_address;
    EnqueueFQ(NewFQE(data),queue_TtoN);
    signal_link_layer_if_allowed(DATA_FOR_NETWORK_LAYER,NULL);
    Unlock(transport_layer_lock);
    return 0;

}

/*
 * Set up a connection, so it is ready to receive data on, and wait for the main loop to signal all data received.
 * Could have a timeout for a connection - and cleanup afterwards.
 */
int receive(char port, unsigned char *data, unsigned int *data_size) {
    packet *p;

    p = DequeueFQ(get_queue_NtoT)->val;

    data = (malloc(8 * sizeof(unsigned char *)));
    data = p->data;
    //dequeue from network to transport queue.

}

/*
 * On connection specified, send the bytes amount of data from buf.
 * Must break the message down into chunks of a manageble size, and queue them up.
 */
int send(int connection_id, unsigned char *buf, unsigned int bytes) {

}


/*
 * Main loop of the transport layer, handling the relevant events, fx. data arriving from the network layer.
 * And take care of the different types of packages that can be received
 */
void transport_layer_loop(void);


