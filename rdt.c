/*
 * Reliable data transfer between two stations
 *
 * Author: Jacob Aae Mikkelsen.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rdt.h"
#include "subnetsupport.h"
#include "subnet.h"
#include "fifoqueue.h"
#include "debug.h"
#include "events.h"
#include "network_layer.h"

/* En macro for at lette overførslen af korrekt navn til Activate */
#define ACTIVATE(n, f) Activate(n, f, #f)

#define MAX_SEQ 127        /* should be 2^n - 1 */
#define NR_BUFS 4

/* Globale variable */

char *StationName;         /* Globalvariabel til at overføre programnavn      */
int ThisStation;           /* Globalvariabel der identificerer denne station. */
log_type LogStyle;         /* Hvilken slags log skal systemet føre            */
boolean network_layer_enabled[NR_BUFS];

LogBuf mylog;                /* logbufferen                                     */

FifoQueue from_network_layer_queue;                /* Queue for data from network layer */
FifoQueue for_network_layer_queue;    /* Queue for data for the network layer */

mlock_t *network_layer_lock;
mlock_t *write_lock;


int ack_timer_id[16];
int timer_ids[NR_BUFS][NR_BUFS];
boolean no_nak = false; /* no nak has been sent yet */

static boolean between(seq_nr a, seq_nr b, seq_nr c) {
    boolean x = ((a <= b) && (b < c));
    boolean y = ((c < a) && (a <= b));
    boolean z = ((b < c) && (c < a));
    // TODO Omskriv så det er til at fatte!
    logLine(debug, "a==%d, b=%d, c=%d, x=%d, y=%d, z=%d\n", a, b, c, x, y, z);

    return x || y || z;
}

static void send_frame(frame_kind fk, seq_nr frame_nr, seq_nr frame_expected, packet buffer[], frame* r) {
    /* Construct and send a data, ack, or nak frame. */

    r->kind = fk;        /* kind == data, ack, or nak */
    if (fk == DATA) {
        r->info = buffer[frame_nr % NR_BUFS];
    }
    r->seq = frame_nr;        /* only meaningful for data frames */
    r->ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    if (fk == NAK) {
        no_nak = false;        /* one nak per frame, please */
    }

    to_physical_layer(r);

    if (fk == DATA) {
        start_timer(frame_nr, r->dest);
    }
    stop_ack_timer(ThisStation);        /* no need for separate ack frame */
}


void FakeTransportLayer(){
    initialize_locks_and_queues();
    char *buffer;
    int i, j;
    event_t event;
    long int events_we_handle;
    FifoQueueEntry e;
    packet p;


    FifoQueue from_queue;                /* Queue for data from network layer */
    FifoQueue for_queue;    /* Queue for data for the network layer */


    from_queue = (FifoQueue) get_from_network_layer_queue();
    for_queue = (FifoQueue) get_for_network_layer_queue();

    // Setup some messages
    for (i = 0; i < 20; i++) {
        buffer = (char *) malloc(sizeof(char) * MAX_PKT);
        sprintf(buffer, "D: %d", i);
        EnqueueFQ(NewFQE((void *) buffer), for_queue);
    }

    events_we_handle = data_for_transport_layer;

    sleep(2);

    printf("Last = %s\n", (char *)for_queue->last->val);
    i = 0;
    j = 0;

    Signal(data_from_transport_layer, NULL);
    while(true){
        Wait(&event, events_we_handle);
        switch (event.type){
            case data_for_transport_layer:

                break;
        }

    }
}

void FakeNetworkLayer(){
    initialize_locks_and_queues();
    char *buffer;
    int i, j;

    //long int events_we_handle;

    event_t event;
    FifoQueueEntry e;



    //events_we_handle = data_from_transport_layer | data_for_link_layer | data_from_link_layer;

    sleep(2);

    network_layer_main_loop();
    /*
    while(true){
        Wait(&event, events_we_handle);
        switch (event.type){
            case data_from_transport_layer:
                e = DequeueFQ(for_network_layer_queue);
                logLine(succes, "Received message: %s\n", ((char *) e->val));
                break;
            case data_for_link_layer:
                break;

        }
    }
     */
}


/* Fake network/upper layers for station 1
 *
 * Send 20 packets and receive 10 before we stop
 * */
void FakeNetworkLayer1() {
    char *buffer;
    int i, j;
    long int events_we_handle;
    event_t event;
    FifoQueueEntry e;

    from_network_layer_queue = InitializeFQ();
    for_network_layer_queue = InitializeFQ();

    // Setup some messages
    for (i = 0; i < 20; i++) {
        buffer = (char *) malloc(sizeof(char) * MAX_PKT);
        sprintf(buffer, "D: %d", i);
        EnqueueFQ(NewFQE((void *) buffer), from_network_layer_queue);
    }

    events_we_handle = network_layer_allowed_to_send | data_for_network_layer;

    // Give selective repeat a chance to start
    sleep(2);

    i = 0;
    j = 0;
    while (true) {
        // Wait until we are allowed to transmit
        Wait(&event, events_we_handle);
        switch (event.type) {
            case network_layer_allowed_to_send:
                Lock(network_layer_lock);
                if (i < 20 && network_layer_enabled[ThisStation]) {
                    // Signal element is ready
                    logLine(info, "Sending signal for message #%d\n", i);
                    network_layer_enabled[ThisStation] = false;
                    Signal(network_layer_ready, NULL);
                    i++;
                }
                Unlock(network_layer_lock);
                break;
            case data_for_network_layer:
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

/* Fake network/upper layers for station 2
 *
 * Receive 20 messages, take the first 10, lowercase first letter and send to station 1.
 * With this, some acks will be piggybacked, some will be pure acks.
 *
 **/
void FakeNetworkLayer2() {
    long int events_we_handle;
    event_t event;
    int j;
    FifoQueueEntry e;

    from_network_layer_queue = InitializeFQ();
    for_network_layer_queue = InitializeFQ();

    events_we_handle = network_layer_allowed_to_send | data_for_network_layer;

    j = 0;
    while (true) {
        // Wait until we are allowed to transmit
        Wait(&event, events_we_handle);

        switch (event.type) {
            case network_layer_allowed_to_send:
                Lock(network_layer_lock);
                if (network_layer_enabled[ThisStation] && !EmptyFQ(from_network_layer_queue)) {
                    logLine(info, "Signal from network layer for message\n");
                    network_layer_enabled[ThisStation] = false;
                    ClearEvent(network_layer_ready); // Don't want to signal too many events
                    Signal(network_layer_ready, NULL);
                }
                Unlock(network_layer_lock);
                break;
            case data_for_network_layer:
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

void log_event_received(long int event) {
    char *event_name;
    check_event(event, event_name);
    logLine(trace, "Event received %s\n", event_name);

}

void selective_repeat() {
    seq_nr ack_expected;              /* lower edge of sender's window */
    seq_nr next_frame_to_send;        /* upper edge of sender's window + 1 */
    seq_nr frame_expected;            /* lower edge of receiver's window */
    seq_nr too_far;                   /* upper edge of receiver's window + 1 */
    int i;                            /* index into buffer pool */
    frame r;                          /* scratch variable */
    packet out_buf[NR_BUFS];          /* buffers for the outbound stream */
    packet in_buf[NR_BUFS];           /* buffers for the inbound stream */
    boolean arrived[NR_BUFS];         /* inbound bit map */
    seq_nr nbuffered;                 /* how many output buffers currently used */
    event_t event;
    long int events_we_handle;
    unsigned int timer_id;

    write_lock = malloc(sizeof(mlock_t));
    network_layer_lock = (mlock_t *) malloc(sizeof(mlock_t));

    Init_lock(write_lock);
    Init_lock(network_layer_lock);


    enable_network_layer(ThisStation, network_layer_enabled, network_layer_lock);             /* initialize */
    ack_expected = 0;                   /* next ack expected on the inbound stream */
    next_frame_to_send = 0;             /* number of next outgoing frame */
    frame_expected = 0;                 /* frame number expected */
    too_far = NR_BUFS;                  /* receiver's upper window + 1 */
    nbuffered = 0;                      /* initially no packets are buffered */

    logLine(trace, "Starting selective repeat %d\n", ThisStation);

    for (i = 0; i < NR_BUFS; i++) {
        arrived[i] = false;
        ack_timer_id[i] = -1;
        for (int j = 0; j < NR_BUFS; j++) {
            timer_ids[i][j] = -1;
        }
    }


    events_we_handle = frame_arrival | timeout | network_layer_ready;


    /*
    // If you are in doubt how the event numbers should be, comment in this, and you will find out.
    printf("%#010x\n", 1);
    printf("%#010x\n", 2);
    printf("%#010x\n", 4);
    printf("%#010x\n", 8);
    printf("%#010x\n", 16);
    printf("%#010x\n", 32);
    printf("%#010x\n", 64);
    printf("%#010x\n", 128);
    printf("%#010x\n", 256);
    printf("%#010x\n", 512);
    printf("%#010x\n", 1024);
    */

    while (true) {
        // Wait for any of these events

        Wait(&event, events_we_handle);
        log_event_received(event.type);

        switch (event.type) {
            case network_layer_ready:        /* accept, save, and transmit a new frame */
                logLine(trace, "Network layer delivers frame - lets send it\n");
                nbuffered = nbuffered + 1;        /* expand the window */
                from_network_layer(&out_buf[next_frame_to_send % NR_BUFS], from_network_layer_queue, network_layer_lock); /* fetch new packet */

                r.source = ThisStation;
                if (ThisStation == 2){
                    r.dest = 1;
                } else {
                    r.dest = 2;
                }
                send_frame(DATA, next_frame_to_send, frame_expected, out_buf, &r);        /* transmit the frame */
                inc(next_frame_to_send);        /* advance upper window edge */
                break;

            case frame_arrival:        /* a data or control frame has arrived */
                from_physical_layer(&r);        /* fetch incoming frame from physical layer */
                if (r.kind == DATA) {
                    /* An undamaged frame has arrived. */
                    if ((r.seq != frame_expected) && no_nak) {
                        send_frame(NAK, 0, frame_expected, out_buf, &r);
                    } else {
                        start_ack_timer(ThisStation); //TODO
                    }
                    if (between(frame_expected, r.seq, too_far) && (arrived[r.seq % NR_BUFS] == false)) {
                        /* Frames may be accepted in any order. */
                        arrived[r.seq % NR_BUFS] = true;        /* mark buffer as full */
                        in_buf[r.seq % NR_BUFS] = r.info;        /* insert data into buffer */
                        while (arrived[frame_expected % NR_BUFS]) {
                            /* Pass frames and advance window. */
                            to_network_layer(&in_buf[frame_expected % NR_BUFS], for_network_layer_queue, network_layer_lock);
                            no_nak = true;
                            arrived[frame_expected % NR_BUFS] = false;
                            inc(frame_expected);        /* advance lower edge of receiver's window */
                            inc(too_far);        /* advance upper edge of receiver's window */
                            start_ack_timer(ThisStation);        /* to see if (a separate ack is needed */ //TODO
                        }
                    }
                }
                if ((r.kind == NAK) && between(ack_expected, (r.ack + 1) % (MAX_SEQ + 1), next_frame_to_send)) {
                    send_frame(DATA, (r.ack + 1) % (MAX_SEQ + 1), frame_expected, out_buf, &r);
                }

                logLine(info, "Are we between so we can advance window? ack_expected=%d, r.ack=%d, next_frame_to_send=%d\n", ack_expected, r.ack, next_frame_to_send);
                while (between(ack_expected, r.ack, next_frame_to_send)) {
                    logLine(debug, "Advancing window %d\n", ack_expected);
                    nbuffered = nbuffered - 1;                /* handle piggybacked ack */
                    stop_timer(ack_expected % NR_BUFS, ThisStation);     /* frame arrived intact */
                    inc(ack_expected);                        /* advance lower edge of sender's window */
                }
                break;

            case timeout: /* Ack timeout or regular timeout*/
                // Check if it is the ack_timer
                timer_id = event.timer_id;
                logLine(trace, "Timeout with id: %d - acktimer_id is %d\n", timer_id, ack_timer_id);
                logLine(info, "Message from timer: '%s'\n", (char *) event.msg);

                if (timer_id == ack_timer_id[r.dest]) { // Ack timer timer out TODO
                    logLine(debug, "This was an ack-timer timeout. Sending explicit ack.\n");
                    free(event.msg);
                    ack_timer_id[r.dest] = -1; // It is no longer running TODO
                    send_frame(ACK, 0, frame_expected, out_buf, &r);        /* ack timer expired; send ack */
                } else {
                    int timed_out_seq_nr = atoi((char *) event.msg);


                    logLine(debug, "Timeout for frame - need to resend frame %d\n", timed_out_seq_nr);
                    send_frame(DATA, timed_out_seq_nr, frame_expected, out_buf, &r);
                }
                break;
        }

        if (nbuffered < NR_BUFS) {
            enable_network_layer(ThisStation, network_layer_enabled, network_layer_lock);
        } else {
            disable_network_layer(ThisStation, network_layer_enabled, network_layer_lock);
        }
    }
}

void print_frame(frame *s, char *direction) {
    char temp[MAX_PKT + 1];

    switch (s->kind) {
        case ACK:
            logLine(info, "%s: ACK frame. Ack seq_nr=%d\n", direction, s->ack);
            break;
        case NAK:
            logLine(info, "%s: NAK frame. Nak seq_nr=%d\n", direction, s->ack);
            break;
        case DATA:
            packet_to_string(&(s->info), temp);
            logLine(info, "%s: DATA frame [seq=%d, ack=%d, kind=%d, (%s)] \n", direction, s->seq, s->ack, s->kind, temp);
            break;
    }
}

int from_physical_layer(frame *r) {
    r->seq = 0;
    r->kind = DATA;
    r->ack = 0;

    int source, dest, length;

    logLine(trace, "Receiving from subnet in station %d\n", ThisStation);
    FromSubnet(&source, &dest, (char *) r, &length);
    print_frame(r, "received");

    r->source = source;
    r->dest = dest;

    return 0;
}


void to_physical_layer(frame *r) {

    if (ThisStation == r->dest) {
        ToSubnet(ThisStation, r->source, (char *) r, sizeof(frame));
    } else {
        ToSubnet(ThisStation, r->dest, (char *) r, sizeof(frame));
    }
    print_frame(r, "sending");
}


void start_timer(seq_nr k, int station) {
    char *msg;
    msg = (char *) malloc(100 * sizeof(char));
    sprintf(msg, "%d", k); // Save seq_nr in message

    timer_ids[k % NR_BUFS][station] = SetTimer(frame_timer_timeout_millis, (void *) msg);

    logLine(trace, "start_timer for seq_nr=%d timer_ids=[%d, %d, %d, %d] %s\n", k, timer_ids[0][station], timer_ids[1][station], timer_ids[2][station], timer_ids[3][station], msg);

}


void stop_timer(seq_nr k, int station) {
    int timer_id;
    char *msg;
    timer_id = timer_ids[k][station];
    logLine(trace, "stop_timer for seq_nr %d med id=%d\n", k, timer_id);

    if (StopTimer(timer_id, (void *) &msg)) {
        logLine(trace, "timer %d stoppet. msg: %s \n", timer_id, msg);
        free(msg);
    } else {
        logLine(trace, "timer %d kunne ikke stoppes. Maaske er den timet ud?timer_ids=[%d, %d, %d, %d] \n", timer_id, timer_ids[0][station], timer_ids[1][station], timer_ids[2][station], timer_ids[3][station]);
    }
}

void start_ack_timer(int station) {
    if (ack_timer_id[station] == -1) {
        logLine(trace, "Starting ack-timer\n");
        char *msg;
        msg = (char *) malloc(100 * sizeof(char));
        strcpy(msg, "Ack-timer");
        ack_timer_id[station] = SetTimer(act_timer_timeout_millis, (void *) msg);
        logLine(debug, "Ack-timer startet med id %d\n", ack_timer_id[station]);
    }
}

void stop_ack_timer(int station) {
    char *msg;

    logLine(trace, "stop_ack_timer\n");
    if (StopTimer(ack_timer_id[station], (void *) &msg)) {
        logLine(trace, "timer %d stoppet. msg: %s \n", ack_timer_id[station], msg);
        free(msg);
    }
    ack_timer_id[station] = -1;
}


int main(int argc, char *argv[]) {

    StationName = argv[0];
    ThisStation = atoi(argv[1]);

    if (argc == 3)
        printf("Station %d: arg2 = %s\n", ThisStation, argv[2]);

    mylog = InitializeLB("mytest");

    LogStyle = synchronized;

    printf("Starting network simulation\n");


    /* processerne aktiveres */
    /*
    ACTIVATE(1, FakeNetworkLayer1);
    ACTIVATE(2, FakeNetworkLayer2);
    ACTIVATE(1, selective_repeat);
    ACTIVATE(2, selective_repeat);
    */

    ACTIVATE(1, FakeTransportLayer);
    ACTIVATE(1, FakeNetworkLayer);
    /* simuleringen starter */
    Start();
    exit(0);
}
