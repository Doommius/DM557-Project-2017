//
// Created by jervelund on 11/21/17.
//
#include <test1.h>
#include <caca_conio.h>
#include <cstdlib>
#include "transport-layer.h"
#include "network_layer.h"
#include "rdt.h"
#include "subnet.h"
#include "subnetsupport.h"


/* En macro for at lette overførslen af korrekt navn til Activate */
#define ACTIVATE(n, f) Activate(n, f, #f)

char *StationName;         /* Globalvariabel til at overføre programnavn      */
int ThisStation;           /* Globalvariabel der identificerer denne station. */

LogBuf mylog;                /* logbufferen                                     */
log_type LogStyle;         /* Hvilken slags log skal systemet føre            */


void Station1_application_layer(void)
{
    connect(2, 20, 20);
    sleep(5);
    Stop();
}

void Station2_application_layer(void)
{
    listen(20);
}

void Station3_application_layer(void)
{
}

void Station4_application_layer(void)
{
}


void main(int argc, char *argv[]){

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
    ACTIVATE(1, FakeTransportLayer1);
    ACTIVATE(4, FakeTransportLayer2);


    //Network Layers for everyone
    ACTIVATE(1, FakeNetworkLayer);
    ACTIVATE(2, FakeNetworkLayer);
    ACTIVATE(3, FakeNetworkLayer);
    ACTIVATE(4, FakeNetworkLayer);

    //Aner ikke hvad jeg laver:
    //ACTIVATE(2, initialize_locks_and_queues);
    //ACTIVATE(3, initialize_locks_and_queues);

    //Selective Repeat for everyone
    ACTIVATE(1, selective_repeat);
    ACTIVATE(2, selective_repeat);
    ACTIVATE(3, selective_repeat);
    ACTIVATE(4, selective_repeat);

    ACTIVATE(1, Station1_application_layer);
    ACTIVATE(2, Station2_application_layer);
    ACTIVATE(3, Station3_application_layer);
    ACTIVATE(4, Station4_application_layer);


    /* simuleringen starter */
    Start();
    exit(0);

};
