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
        transport_layer.c
        application_layer.c
        Wait.c network_layer.h events.h events.c network_layer.c "debugging folder/main.cpp")

set(SOURCE_FILES_mynetwork rdt.c rdt.h debug.c debug.h fifoqueue.h fifoqueue.c subnetsupport.h subnet.h priqueue.c priqueue.h Signal.c subnet.c Wait.c Stop.c Activate.c ClearEvent.c FromSubnet.c ToSubnet.c Start.c signalhandler.c shell.c flow.c errortimer.c timer.c
        receiver.c control.c re_delay_frame.c transfer_frame.c transmit_error.c delay_frame.c subnetsupport.c network_layer.c network_layer.h events.c events.h transport_layer.c transport_layer.h application_layer.c application_layer.h)

set(SOURCE_FILES_network network.c)

add_executable(dm557_projekt_2017 ${SOURCE_FILES})

add_executable(network ${SOURCE_FILES_network})

add_executable(mynetwork ${SOURCE_FILES_mynetwork})

TARGET_LINK_LIBRARIES( mynetwork
        pthread
        )

TARGET_LINK_LIBRARIES( network
        pthread
        )