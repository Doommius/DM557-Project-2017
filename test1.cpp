//
// Created by jervelund on 11/21/17.
//
#include <test1.h>
#include <caca_conio.h>
#include "transport-layer.h"
#include "subnet.h"

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
