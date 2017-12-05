/*
 * rdt.h
 *
 *  Created on: Aug 18, 2017
 *      Author: jacob
 */

#include "fifoqueue.h"


#ifndef RDT_H_
#define RDT_H_


#define FRAME_TIMER_TIMEOUT_MILLIS  250
#define ACT_TIMER_TIMEOUT_MILLIS     50

#define MAX_PKT 8       /* determines packet size in bytes */

typedef enum {
    false, true
} boolean;        /* boolean type */

typedef unsigned int seq_nr;        /* sequence or ack numbers */


typedef struct {
    char data[MAX_PKT];

    int source;
    int dest;
} packet;        /* packet definition */



/* For now only DATAGRAM is used, but for dynamic routing, ROUTERINFO is defined */
typedef enum {DATAGRAM, ROUTERINFO} datagram_kind;        /* datagram_kind definition */


typedef struct {                        /* datagrams are transported in this layer */
    packet data;   /* Data from the transport layer segment  */
    datagram_kind kind;                   /* what kind of a datagram is it? */
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
    datagram info;          /* the network layer packet */
    int sendTime;
    int recvTime;

    int source;         /* Source station */
    int dest;           /* Destination station */
} frame;

/* init_frame fills in default initial values in a frame. Protocols should
 * call this function before creating a new frame. Protocols may later update
 * some of these fields. This initialization is not strictly needed, but
 * makes the simulation trace look better, showing unused fields as zeros.
 */
void init_frame(frame *s, int count);

/* Go get an inbound frame from the physical layer and copy it to r. */
int from_physical_layer(frame *r);

/* Pass the frame to the physical layer for transmission. */
void to_physical_layer(frame *r, int dest);

/* Start the clock running and enable the timeout event. */
void start_timer(seq_nr k, int station);

/* Stop the clock and disable the timeout event. */
void stop_timer(seq_nr k, int station);

/* Start an auxiliary timer and enable the ack_timeout event. */
void start_ack_timer(int station);

/* Stop the auxiliary timer and disable the ack_timeout event. */
void stop_ack_timer(int station);
/* In case of a timeout event, it is possible to find out the sequence
 * number of the frame that timed out (this is the sequence number parameter
 * in the start_timer function). For this, the simulator must know the maximum
 * possible value that a sequence number may have. Function init_max_seqnr()
 * tells the simulator this value. This function must be called before
 * start_simulator() function. When a timeout event occurs, function
 * get_timedout_seqnr() returns the sequence number of the frame that timed out.
 */
void init_max_seqnr(unsigned int o);

unsigned int get_timedout_seqnr(void);

void selective_repeat(void);

int get_ThisStation();

FifoQueue get_from_queue();

FifoQueue get_for_queue();

/* Macro inc is expanded in-line: Increment k circularly. */
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0

#endif /* RDT_H_ */
