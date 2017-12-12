//
// Created by morten on 11/4/17.
//

#ifndef EVENTS_H
#define EVENTS_H

//#no care about hex
#define FRAME_ARRIVAL 1
#define TIME_OUT 2

#define NETWORK_LAYER_ALLOWED_TO_SEND 4     //Network layer allowed to send
#define NETWORK_LAYER_READY 8               //Network layer ready to send
#define DATA_FOR_NETWORK_LAYER 16           //Data for network layer from link layer

#define DATA_FOR_TRANSPORT_LAYER 32         //Data for transport layer from network layer
#define DATA_FROM_TRANSPORT_LAYER 64        //Data from transport layer to network layer

#define DATA_FOR_LINK_LAYER 128             //Data for link layer from network layer
#define DONE_SENDING 256                    //Signal Network layer that we are done, so shutdown

#define DATA_FOR_APPLICATION_LAYER 512      //Data for application layer from transport layer
#define DATA_FROM_APPLICATION_LAYER 1024    //Data from application layer to transport layer

#define CONNECTION_REPLY 2048               //Connection request reply


void check_event(long int event, char* event_name);

#endif //EVENTS_H
