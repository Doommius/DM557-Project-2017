//
// Created by morten on 12/8/17.
//

#include "application_layer.h"

void Station1_application_layer(void) {
    sleep(1);
    connect(2, 20, 20);
    //send(2, "Hello world", 12);
    sleep(5);
    Stop();
}

void Station2_application_layer(void) {
    sleep(1);
    listen(20);
}

void Station3_application_layer(void) {
}

void Station4_application_layer(void) {
}