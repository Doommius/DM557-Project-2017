//
// Created by jervelund on 11/21/17.
//

#ifndef __TRANSPORT_LAYER_H__
#define __TRANSPORT_LAYER_H__


#define TPDU_PAYLOAD_SIZE 128;

#define TPDU_SIZE 8;

typedef enum
{
    call_req,
    call_acc,
    tcredit,
    clear_req,
    clear_conf,
    data_tpdu
} tpdu_type;

typedef struct tpdu_s
{
    char            m;
    char            q;
    tpdu_type       type;
    unsigned int    bytes;
    char            payload[TPDU_PAYLOAD_SIZE];
} tpdu_t;

typedef struct transport_packet_s
{
    char    vc_id;
    char    data[TPDU_SIZE];
} transport_packet_t;

typedef enum
{
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


typedef struct connection_s
{
    transport_address   local_address;
    transport_address   remote_address;
    connection_state    state;
    long                timer;
    unsigned char      *user_buf_addr;
    unsigned int        byte_count;
    unsigned int        clr_req_received;
    unsigned int        credits;
} connection_t;

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
int receive(char, unsigned char *buf, unsigned int *);

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

#endif /* __TRANSPORT_LAYER_H__ */