//
// Created by morten on 12/8/17.
//

#include <string.h>
#include "application_layer.h"

int stringLen;

void Station1_application_layer(void) {
    char *msg;
    int dest;
    dest = 4;
    msg = "Hello World, i am a program";
    stringLen = strlen(msg) + 1;
    sleep(2);
    connect(dest, 20, 20);
    sleep(2);
    printf("----------Sending----------\n");
    send(2, msg, stringLen);
    sleep(5);
    printf("----------Disconnecting----------\n");
    disconnect(dest);
    sleep(5);
    Stop();
}

void Station2_application_layer(void) {
    sleep(2);
    //listen(20);
    char *msg;
    int dest;
    dest = 1;
    msg = malloc(stringLen);
    if(receive(dest, msg, stringLen)){
        printf("Got message: %s\n", msg);
    }
}

void Station3_application_layer(void) {
}

void Station4_application_layer(void) {
    sleep(2);
    //listen(20);
    char *msg;
    int dest;
    dest = 1;
    msg = malloc(stringLen);
    if(receive(dest, msg, stringLen)){
        printf("Got message: %s\n", msg);
    }
}