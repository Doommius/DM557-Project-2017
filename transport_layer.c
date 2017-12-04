//
// Created by jervelund on 11/21/17.
//

#include <caca_conio.h>
#include <stdlib.h>
#include <string.h>
#include "transport_layer.h"
#include "subnetsupport.h"
#include "debug.h"
#include "network_layer.h"
#include "subnet.h"


int connectionid; //do we even need this.

mlock_t *network_layer_lock;
mlock_t *write_lock;


/*
 * Listen for incomming calls on the specified transport_address.
 * Blocks while waiting
 */
int listen(transport_address local_address) {
    tpdu_t* packet;
    FifoQueueEntry e;
    event_t event;
    long int events_we_handle = DATA_FOR_TRANSPORT_LAYER;

    while(true){
        //TODO: do so it also waits for timeout.
        Wait(&event, events_we_handle);

        if((int)event.msg ==  get_ThisStation()){
            Lock(transport_layer_lock);
            e = DequeueFQ(queue_NtoT);
            if(!e){
                printf("Error with queue");
            }else{
                memcpy(packet, ValueOfFQE(e),sizeof(packet));
                free(ValueOfFQE( e ));
                DeleteFQ(e);
                if(packet->bytes){
                    printf("connection accepted");
                    Unlock(transport_layer_lock);
                    return 0;
                }else{
                    printf("connection failed.");
                    Unlock(transport_layer_lock);
                    return -1;
                }
            }
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


}

/*
 * Disconnect the connection with the supplied id.
 * returns appropriate errorcode or 0 if successfull
 */
int disconnect(int connection_id) {

}

/*
 * Set up a connection, so it is ready to receive data on, and wait for the main loop to signal all data received.
 * Could have a timeout for a connection - and cleanup afterwards.
 */
int receive(char unsure, unsigned char *data, unsigned int *timeout_sec) {


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
int send(int connection_id, unsigned char *buf, unsigned int bytes);


/*
 * Main loop of the transport layer, handling the relevant events, fx. data arriving from the network layer.
 * And take care of the different types of packages that can be received
 */
void transport_layer_loop(void);


