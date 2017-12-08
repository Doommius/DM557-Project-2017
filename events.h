//
// Created by morten on 11/4/17.
//

#ifndef EVENTS_H
#define EVENTS_H

//#no care about hex
#define FRAME_ARRIVAL 1
#define TIME_OUT 2
#define NETWORK_LAYER_ALLOWED_TO_SEND 4
#define NETWORK_LAYER_READY 8
#define DATA_FOR_NETWORK_LAYER 16
#define DATA_FOR_TRANSPORT_LAYER 32
#define DATA_FROM_TRANSPORT_LAYER 64
#define DATA_FOR_LINK_LAYER 128
#define DATA_FROM_LINK_LAYER 256
#define DONE_SENDING 512
#define DATA_FOR_APPLICATION_LAYER 1024
#define DATA_FROM_APPLICATION_LAYER 2048


void check_event(long int event, char* event_name);

#endif //EVENTS_H
