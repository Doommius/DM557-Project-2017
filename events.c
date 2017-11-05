//
// Created by morten on 11/4/17.
//

#include "events.h"

void check_event(long int event, char* event_name){
    switch (event) {
        case FRAME_ARRIVAL:
            event_name = "frame_arrival";
            break;
        case TIME_OUT:
            event_name = "timeout";
            break;
        case NETWORK_LAYER_ALLOWED_TO_SEND:
            event_name = "network_layer_allowed_to_send";
            break;
        case NETWORK_LAYER_READY:
            event_name = "network_layer_ready";
            break;
        case DATA_FOR_NETWORK_LAYER:
            event_name = "data_for_network_layer";
            break;
        case DATA_FOR_TRANSPORT_LAYER:
            event_name = "data_for_transport_layer";
            break;
        case DATA_FROM_TRANSPORT_LAYER:
            event_name = "data_from_transport_layer";
            break;
        case DATA_FOR_LINK_LAYER:
            event_name = "data_for_link_layer";
            break;
        case DATA_FROM_LINK_LAYER:
            event_name = "data_from_link_layer";
            break;
        default:
            event_name = "unknown";
            break;
    }
}