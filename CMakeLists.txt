cmake_minimum_required(VERSION 3.6)
project(dm557_projekt_2017)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        include/fifoqueue.h
        include/priqueue.h
        include/subnet.h
        include/subnetsupport.h
        Activate.c
        ClearEvent.c
        control.c
        debug.c
        debug.h
        delay_frame.c
        errortimer.c
        fifoqueue.c
        fifoqueue.h
        flow.c
        FromSubnet.c
        funk1.c
        maketest.c
        nettest.c
        network.c
        priqueue.c
        priqueue.h
        rdt.c
        rdt.h
        re_delay_frame.c
        receiver.c
        shell.c
        Signal.c
        signalhandler.c
        signaltest.c
        skel.c
        Start.c
        Stop.c
        subnet.c
        subnet.h
        subnet_internal.h
        subnetsupport.c
        subnetsupport.h
        timer.c
        timertest.c
        ToSubnet.c
        transfer_frame.c
        transmit_error.c
        Wait.c)

set(SOURCE_FILES_mynetwork rdt.c rdt.h debug.c debug.h fifoqueue.h fifoqueue.c subnetsupport.c subnetsupport.h subnet.h priqueue.c priqueue.h)



add_executable(dm557_projekt_2017 ${SOURCE_FILES})

add_executable(mynetwork ${SOURCE_FILES_mynetwork})