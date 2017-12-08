//
// Created by jervelund on 11/21/17.
//

#ifndef __TRANSPORT_LAYER_H__
#define __TRANSPORT_LAYER_H__


#include "subnetsupport.h"
#include "rdt.h"

#define TPDU_PAYLOAD_SIZE 128

#define TPDU_SIZE 8

#define NUM_CONNECTIONS 4

mlock_t *transport_layer_lock;


typedef enum {
    call_req,
    call_rep,
    tcredit,
    clear_req,
    clear_conf,
    data_tpdu
} tpdu_type;

typedef struct {
    int             destport;
    int             returnport;
    char            m;   //What is m and q?
    char            q;
    unsigned int    dest;
    tpdu_type       type;
    unsigned int    bytes;
    char            payload[TPDU_PAYLOAD_SIZE];
} tpdu;


//Legacy code.
typedef struct {
    char    vc_id;
    char    data[TPDU_SIZE];
} transport_packet;

typedef enum {
    disconn,
    receiving,
    established,
    sending,
    idle,
    waiting,
    queued
} connection_state;

/* Similar to port numbers when working with sockets */
typedef int transport_address;

/* You may already have this in the NL  */
typedef unsigned int host_address;


typedef struct {
    transport_address   local_address;
    transport_address   remote_address;
    connection_state    state;
    host_address        remote_host;
    long                timer;
    unsigned char      *user_buf_addr;
    int                 id;
    unsigned int        byte_count;
    unsigned int        clr_req_received;
    unsigned int        credits;
} connection;

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