//
// Created by morten on 12/11/17.
//

#ifndef DM557_PROJEKT_2017_STRUCTS_H
#define DM557_PROJEKT_2017_STRUCTS_H

#define TPDU_PAYLOAD_SIZE 64

#define TPDU_SIZE 8

#define NUM_CONNECTIONS 4

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
    //char            m;   //What is m and q?
    //char            q;
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

typedef unsigned int seq_nr;        /* sequence or ack numbers */


typedef struct {
    tpdu data;
    int source;
    int dest;
} segment;        /* segment definition */


/* For now only DATAGRAM is used, but for dynamic routing, ROUTERINFO is defined */
typedef enum {DATAGRAM, ROUTERINFO} datagram_kind;        /* datagram_kind definition */


typedef struct {                        /* datagrams are transported in this layer */
    segment data;   /* Data from the transport layer segment  */
    //datagram_kind kind;                   /* what kind of a datagram is it? */
    int from;                                                /* From station address */
    int to;                                                /* To station address */
    int globalDest;
    int globalSource;
} datagram;

typedef enum {
    DATA, ACK, NAK
} frame_kind;        /* frame_kind definition */


typedef struct {        /* frames are transported in this layer */
    frame_kind kind;        /* what kind of a frame is it? */
    seq_nr seq;           /* sequence number */
    seq_nr ack;           /* acknowledgement number */
    datagram info;          /* the network layer segment */
    //int sendTime;
    //int recvTime;

    int source;         /* Source station */
    int dest;           /* Destination station */
} frame;


#endif //DM557_PROJEKT_2017_STRUCTS_H
