//
// Created by jervelund on 11/21/17.
//

#ifndef __TRANSPORT_LAYER_H__
#define __TRANSPORT_LAYER_H__


#include "subnetsupport.h"
#include "structs.h"

mlock_t *transport_layer_lock;


/*
 * Listen for incomming calls on the specified transport_address.
 * Blocks while waiting
 */
int listen(transport_address local_address);


/*
 * Try to connect to remote host, on specified transport_address,
 * listening back on local transport address. (full duplex).
 *
 * Once you have used the connection to send(), you can then be able to receive
 * Returns the connection id - or an appropriate error code
 */
int connect(host_address remote_host, transport_address local_ta, transport_address remote_ta);

/*
 * Disconnect the connection with the supplied id.
 * returns appropriate errorcode or 0 if successfull
 */
int disconnect(int connection_id);

/*
 * Set up a connection, so it is ready to receive data on, and wait for the main loop to signal all data received.
 * Could have a timeout for a connection - and cleanup afterwards.
 */
int receive(host_address remote_host, unsigned char *buf, unsigned int *bufsize);

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

//Check if connection exists
int check_connection(int connection_id);

//Check if port is available
int check_ports(transport_address port);

void copyTPDUtoSegment(tpdu *t, segment *s);

#endif /* __TRANSPORT_LAYER_H__ */