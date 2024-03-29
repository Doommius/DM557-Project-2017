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
#include "debug.h"
#include "events.h"
#include "network_layer.h"
#include "transport_layer.h"
#include "application_layer.h"
#include "structs.h"

/* En macro for at lette overførslen af korrekt navn til Activate */
#define ACTIVATE(n, f) Activate(n, f, #f)

#define MAX_SEQ 127        /* should be 2^n - 1 */
#define NR_BUFS 5
#define MESSAGES 2



/* Globale variable */

char *StationName;         /* Globalvariabel til at overføre programnavn      */
int ThisStation;           /* Globalvariabel der identificerer denne station. */
log_type LogStyle;         /* Hvilken slags log skal systemet føre            */

LogBuf mylog;                /* logbufferen                                     */

FifoQueue from_network_layer_queue;                /* Queue for data from network layer */
FifoQueue for_network_layer_queue;    /* Queue for data for the network layer */

mlock_t *network_layer_lock;
mlock_t *write_lock;



int ack_timer_id[16];
int timer_ids[NR_BUFS][NR_BUFS];
boolean no_nak[NR_BUFS]; /* no nak has been sent yet */


/**
 * This one does this
 * ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a))
 * eg. if any one number is between one the the others it returns True
 * @param a
 * @param b
 * @param c
 * @return
 */
static boolean between(seq_nr a, seq_nr b, seq_nr c) {
    // x = a leq b and b les c
    // if b is the middle number
    boolean x = ((a <= b) && (b < c));
    // y = c less a and a leq b
    // if a is the middle number?
    boolean y = ((c < a) && (a <= b));
    // z = b less c and c less a
    // if c is the middle number
    boolean z = ((b < c) && (c < a));

    logLine(debug, "a==%d, b=%d, c=%d, x=%d, y=%d, z=%d\n", a, b, c, x, y, z);

    if(x){
        logLine(debug, "between function: seq_nr b (%d) is between a(%d) and c(%d)\n",b,a,c);
    }
    if(y){
        logLine(debug, "between function: seq_nr a (%d) is between b(%d) and c(%d)\n",a,b,c);
    }
    if(z){
        logLine(debug, "between function: seq_nr c (%d) is between a(%d) and b(%d)\n",c,a,b);
    }


    // return true is any of them are between.
    return x || y || z;
}

static void send_frame(frame_kind fk, seq_nr frame_nr, seq_nr frame_expected, datagram buffer[], int dest) {
    /* Construct and send a data, ack, or nak frame. */

    frame s;

    s.kind = fk;        /* kind == data, ack, or nak */
    if (fk == DATA) {
        s.info = buffer[frame_nr % NR_BUFS];
        printf("1. DATA\n");
    }

    s.seq = frame_nr;        /* only meaningful for data frames */
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    if (fk == NAK) {
        no_nak[dest] = false;        /* one nak per frame, please */
        printf("NAK\n");
    }

    to_physical_layer(&s, dest);

    printf("Sending frame from: %i, to %i, with gD: %i, gS: %i\n", ThisStation, dest, s.info.globalDest, s.info.globalSource);

    if (fk == DATA) {
        start_timer(frame_nr, dest);
        printf("2. DATA\n");
    }
    printf("5. stopping ack timer: %i\n", dest);
    stop_ack_timer(dest);        /* no need for separate ack frame */
}

FifoQueue get_from_queue(){
    return (FifoQueue) from_network_layer_queue;
}

FifoQueue get_for_queue(){
    return (FifoQueue) for_network_layer_queue;
}

//Sends messages
void FakeTransportLayer1(){
    initialize_locks_and_queues();
    char *buffer;
    int i, j;
    event_t event;
    long int events_we_handle;
    FifoQueueEntry e;
    segment *p;

    FifoQueue from_queue;                /* Queue for data from network layer */
    FifoQueue for_queue;    /* Queue for data for the network layer */

    from_queue = (FifoQueue) get_queue_NtoT();
    for_queue = (FifoQueue) get_queue_TtoN();

    // Setup some messages
    for (i = 0; i < MESSAGES; i++) {
        p = (segment *) malloc(sizeof(segment));
        buffer = (char *) malloc(sizeof(char) * MAX_PKT);
        sprintf(buffer, "D: %d", i);

        //strcpy(p->data, buffer);

        p->source = ThisStation;

        p->dest = 4;

        EnqueueFQ(NewFQE((void *) p), for_queue);

    }

    events_we_handle = DATA_FOR_TRANSPORT_LAYER;

    sleep(2);

    j = 0;

    //Signal network layer, that the packets are ready:
    for (int k = 0; k < MESSAGES; k++) {
        Signal(DATA_FROM_TRANSPORT_LAYER, NULL);
    }

    while(true){
        Wait(&event, events_we_handle);
        switch (event.type){

            case DATA_FOR_TRANSPORT_LAYER:
                Lock(network_layer_lock);

                e = DequeueFQ(from_queue);
                segment *p2;

                p2 = e->val;

                logLine(succes, "Received message: %s\n", p2->data);

                j++;
                Unlock(network_layer_lock);
                break;

        }
        if(j >= MESSAGES && EmptyFQ(from_queue)){

            Signal(DONE_SENDING, NULL);
            sleep(5);
            Stop();
        }
    }
}


//Recieves messages
void FakeTransportLayer2(){
    initialize_locks_and_queues();
    int j;
    event_t event;
    long int events_we_handle;
    FifoQueueEntry e;

    FifoQueue from_queue;                /* Queue for data from network layer */
    FifoQueue for_queue;    /* Queue for data for the network layer */

    events_we_handle = DATA_FOR_TRANSPORT_LAYER;

    sleep(2);

    j = 0;

    while(true){
        Wait(&event, events_we_handle);
        switch (event.type){
            case DATA_FOR_TRANSPORT_LAYER:
                Lock(network_layer_lock);


                from_queue = (FifoQueue) get_queue_NtoT();

                for_queue = (FifoQueue) get_queue_TtoN();

                printf("Er queue tom? %i\n", EmptyFQ(from_queue));
                e = DequeueFQ(from_queue);

                segment *p;

                p = e->val;


                //printf("Fik besked i transport laget med message: %s\n", p->data);
                logLine(succes, "Received message: %s\n", p->data);

                if (j < MESSAGES) {
                    //((char *) p->data)[0] = 'd';
                    p->source = ThisStation;

                    p->dest = 1;

                    printf("Queuer segment tilbage med source: %i, dest: %i\n", p->source, p->dest);
                    EnqueueFQ(NewFQE((void *) p), for_queue);
                    Signal(DATA_FROM_TRANSPORT_LAYER, NULL);
                }


                j++;

                Unlock(network_layer_lock);
                break;
        }
        if(j >= MESSAGES && EmptyFQ(for_queue)){

            Signal(DONE_SENDING, NULL);
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
    seq_nr ack_expected[NR_BUFS];              /* lower edge of sender's window */
    seq_nr next_frame_to_send;        /* upper edge of sender's window + 1 */
    seq_nr frame_expected;            /* lower edge of receiver's window */
    seq_nr too_far;                   /* upper edge of receiver's window + 1 */
    int i, dest;                            /* index into buffer pool */
    frame r;                          /* scratch variable */
    datagram out_buf[NR_BUFS];          /* buffers for the outbound stream */
    datagram in_buf[NR_BUFS];           /* buffers for the inbound stream */
    boolean arrived[NR_BUFS];         /* inbound bit map */
    seq_nr nbuffered;                 /* how many output buffers currently used */
    event_t event;
    long int events_we_handle;
    unsigned int timer_id;

    write_lock = malloc(sizeof(mlock_t));
    network_layer_lock = (mlock_t *) malloc(sizeof(mlock_t));

    Init_lock(write_lock);
    Init_lock(network_layer_lock);

    boolean *network_layer_enabled;
    network_layer_enabled = get_network_layer_enabled();


    next_frame_to_send = 0;             /* number of next outgoing frame */
    frame_expected = 0;                 /* frame number expected */
    too_far = NR_BUFS;                  /* receiver's upper window + 1 */
    nbuffered = 0;                      /* initially no packets are buffered */

    logLine(trace, "Starting selective repeat %d\n", ThisStation);

    for (i = 0; i < NR_BUFS; i++) {
        arrived[i] = false;
        ack_timer_id[i] = -1;
        no_nak[i] = false;
        ack_expected[i] = 0;                   /* next ack expected on the inbound stream */
        enable_network_layer(i, network_layer_enabled, network_layer_lock);             /* initialize */
        for (int j = 0; j < NR_BUFS; j++) {
            timer_ids[i][j] = -1;
        }
    }


    events_we_handle = frame_arrival | timeout | NETWORK_LAYER_READY;


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
            case NETWORK_LAYER_READY:        /* accept, save, and transmit a new frame */
                //Lock(write_lock);
                printf("NETWORK_LAYER_READY\n");
                logLine(trace, "Network layer delivers frame - lets send it\n");
                if (!EmptyFQ(from_network_layer_queue)) {
                    nbuffered = nbuffered + 1;        /* expand the window */

                    from_network_layer(&out_buf[next_frame_to_send % NR_BUFS], from_network_layer_queue,
                                       network_layer_lock); /* fetch new segment */

                    dest = out_buf[next_frame_to_send % NR_BUFS].to;
                    network_layer_enabled[dest] = false;

                    printf("1. send, dest: %i\n", dest);
                    send_frame(DATA, next_frame_to_send, frame_expected, out_buf, dest);        /* transmit the frame */
                    inc(next_frame_to_send);        /* advance upper window edge */
                    //Unlock(write_lock);
                }
                break;

            case frame_arrival:        /* a data or control frame has arrived */
                //Lock(write_lock);
                printf("FRAME_ARRIVAL\n");
                from_physical_layer(&r);        /* fetch incoming frame from physical layer */
                printf("    Frame from %i, to %i, globaldest: %i, globalsource: %i, message: %s, segment dest: %i, tpdu dest: %i\n", r.source, r.dest, r.info.globalDest, r.info.globalSource, r.info.data.data.payload, r.info.data.dest, r.info.data.data.dest);
                if (r.kind == DATA) {
                    printf("Got DATA\n");
                    /* An undamaged frame has arrived. */
                    if ((r.seq != frame_expected) && no_nak[r.source]) {
                        printf("2. send, dest: %i\n", r.source);
                        send_frame(NAK, 0, frame_expected, out_buf, r.source);
                    } else {
                        //printf("1. starting ack timer for: %i\n", r.source);
                        start_ack_timer(r.source);
                    }
                    if (between(frame_expected, r.seq, too_far) && (arrived[r.seq % NR_BUFS] == false)) {
                        /* Frames may be accepted in any order. */

                        arrived[r.seq % NR_BUFS] = true;        /* mark buffer as full */
                        in_buf[r.seq % NR_BUFS] = r.info;        /* insert data into buffer */
                        while (arrived[frame_expected % NR_BUFS]) {
                            /* Pass frames and advance window. */
                            to_network_layer(&in_buf[frame_expected % NR_BUFS], for_network_layer_queue, network_layer_lock);
                            no_nak[r.source] = true;
                            arrived[frame_expected % NR_BUFS] = false;
                            inc(frame_expected);        /* advance lower edge of receiver's window */
                            inc(too_far);        /* advance upper edge of receiver's window */
                            printf("2. starting ack timer for: %i\n", r.source);
                            start_ack_timer(r.source);        /* to see if (a separate ack is needed */
                        }
                    }
                }
                if ((r.kind == NAK) && between(ack_expected[r.source], (r.ack + 1) % (MAX_SEQ + 1), next_frame_to_send)) {
                    printf("3. send, dest: %i\n", r.source);
                    send_frame(DATA, (r.ack + 1) % (MAX_SEQ + 1), frame_expected, out_buf, r.source);
                }

                logLine(info, "Are we between so we can advance window? ack_expected=%d, r.ack=%d, next_frame_to_send=%d\n", ack_expected[r.source], r.ack, next_frame_to_send);
                while (between(ack_expected[r.source], r.ack, next_frame_to_send)) {
                    logLine(debug, "Advancing window %d\n", ack_expected[r.source]);
                    nbuffered = nbuffered - 1;                /* handle piggybacked ack */
                    //printf("3. stopping timer for %i\n", r.source);
                    stop_timer(ack_expected[r.source] % NR_BUFS, r.source);     /* frame arrived intact */
                    inc(ack_expected[r.source]);                        /* advance lower edge of sender's window */
                }
                //Unlock(write_lock);
                break;

            case timeout: /* Ack timeout or regular timeout*/

                printf("TIMEOUT\n");
                // Check if it is the ack_timer
                timer_id = event.timer_id;
                logLine(trace, "Timeout with id: %d - acktimer_id is %d\n", timer_id, ack_timer_id);
                logLine(info, "Message from timer: '%s'\n", (char *) event.msg);


                int test;
                test = -1;
                printf("finding ID\n");
                for (int j = 0; j < NR_BUFS; j++) {
                    //printf("%i\n", ack_timer_id[j]);
                    if (timer_id == ack_timer_id[j]){
                        test = j;
                        printf("found j: %i\n", test);
                        break;
                    } else {
                        for (int k = 0; k < NR_BUFS; k++) {
                            //printf(" %i\n", timer_ids[j][k]);
                            if (timer_id == timer_ids[j][k]){
                                test = k;
                                printf("found k: %i\n", test);
                                break;
                            }
                        }
                    }
                }

                printf("Timer ID: %i, ack_timer_id: %i, r.dest: %i, test: %i, ack_timer_id (test): %i, r.info.globalsource: %i\n", timer_id, ack_timer_id[r.dest], r.dest, test, ack_timer_id[test], r.info.globalSource);

                if(test != -1) {
                    if (timer_id == ack_timer_id[test]) { // Ack timer timer out
                        logLine(debug, "This was an ack-timer timeout. Sending explicit ack.\n");
                        free(event.msg);
                        ack_timer_id[test] = -1; // It is no longer running
                        printf("4. send, %i, test: %i\n", r.info.globalSource, test);
                        send_frame(ACK, 0, frame_expected, out_buf, test);        /* ack timer expired; send ack */
                    } else {
                        int timed_out_seq_nr = atoi((char *) event.msg);
                        printf("Event msg: %s\n", (char *) event.msg);
                        logLine(debug, "Timeout for frame - need to resend frame %d\n", timed_out_seq_nr);
                        printf("5. send, %i, test: %i\n", r.info.globalSource, test);
                        send_frame(DATA, timed_out_seq_nr, frame_expected, out_buf, test);
                    }
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
            datagram_to_string(&(s->info), temp);
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
    r->source = source;
    r->dest = dest;
    printf("length %i\n", length);
    printf("tpdu size: %i, frame size: %i\n", sizeof(tpdu), sizeof(frame));
    return 0;
}


void to_physical_layer(frame *r, int dest) {

    printf("physical layer, gd: %i\n", r->info.globalDest);
    ToSubnet(ThisStation, dest, (char *) r, sizeof(frame));
    //printf("After ToSubnet\n");
    print_frame(r, "sending");
}


void start_timer(seq_nr k, int station) {
    char *msg;
    msg = (char *) malloc(100 * sizeof(char));
    sprintf(msg, "%d", k); // Save seq_nr in message

    timer_ids[k % NR_BUFS][station] = SetTimer(FRAME_TIMER_TIMEOUT_MILLIS, (void *) msg);
    printf("starting timer for station: %i, with value: %i. k: %i, k mod NR_BUFS: %i, msg: %s\n", station, timer_ids[k % NR_BUFS][station], k, k % NR_BUFS, msg);

    logLine(trace, "start_timer for seq_nr=%d timer_ids=[%d, %d, %d, %d] %s\n", k, timer_ids[0][station], timer_ids[1][station], timer_ids[2][station], timer_ids[3][station], msg);

}


void stop_timer(seq_nr k, int station) {
    int timer_id;
    char *msg;
    timer_id = timer_ids[k][station];
    printf("Stopping timer for station: %i, seq_nr: %i, timer_id: %i\n", station, k, timer_id);
    logLine(trace, "stop_timer for seq_nr %d med id=%d\n", k, timer_id);

    if (StopTimer(timer_id, (void *) &msg)) {
        logLine(trace, "timer %d stoppet. msg: %s \n", timer_id, msg);
        printf("        Stoppede timer :)\n");
        free(msg);
    } else {
        printf("        Kunne ikke stoppe timer :(\n");
        logLine(trace, "timer %d kunne ikke stoppes. Maaske er den timet ud?timer_ids=[%d, %d, %d, %d] \n", timer_id, timer_ids[0][station], timer_ids[1][station], timer_ids[2][station], timer_ids[3][station]);
    }
}

void start_ack_timer(int station) {
    if (ack_timer_id[station] == -1) {
        logLine(trace, "Starting ack-timer\n");
        char *msg;
        msg = (char *) malloc(100 * sizeof(char));
        strcpy(msg, "Ack-timer");
        ack_timer_id[station] = SetTimer(ACT_TIMER_TIMEOUT_MILLIS, (void *) msg);
        //printf("started ack timer for station: %i, with value: %i\n", station, ack_timer_id[station]);
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

int get_ThisStation(){
    return ThisStation;
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


    //Transport Layers for our hosts
    //ACTIVATE(1, FakeTransportLayer1);
    //ACTIVATE(4, FakeTransportLayer2);



    //Selective Repeat for everyone
    ACTIVATE(1, selective_repeat);
    ACTIVATE(2, selective_repeat);
    ACTIVATE(3, selective_repeat);
    ACTIVATE(4, selective_repeat);

    //Network Layers for everyone
    ACTIVATE(1, FakeNetworkLayer);
    ACTIVATE(2, FakeNetworkLayer);
    ACTIVATE(3, FakeNetworkLayer);
    ACTIVATE(4, FakeNetworkLayer);

    //Start transport layer
    ACTIVATE(1, transport_layer_loop);
    ACTIVATE(2, transport_layer_loop);

    //Start application layer
    ACTIVATE(1, Station1_application_layer);
    ACTIVATE(2, Station2_application_layer);





    //Aner ikke hvad jeg laver:
    //ACTIVATE(2, initialize_locks_and_queues);
    //ACTIVATE(3, initialize_locks_and_queues);


    /* simuleringen starter */
    Start();
    exit(0);
}
