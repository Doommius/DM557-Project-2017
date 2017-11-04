//
// Created by morten on 11/4/17.
//

#ifndef EVENTS_H
#define EVENTS_H

#define FRAME_ARRIVAL 1
#define TIME_OUT 2
#define NETWORK_LAYER_ALLOWED_TO_SEND 4
#define NETWORK_LAYER_READY 8
#define DATA_FOR_NETWORK_LAYER 16
#define TRANSPORT_LAYER_READY 32


void check_event(long int event, char* event_name);

#endif //EVENTS_H
