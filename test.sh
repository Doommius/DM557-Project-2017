#!bin/bash
pkill mynetwork
pkill network
make clean
make all
./network -pmynetwork -n4 -e0