#!/bin/bash

./rpcclient add,0,0,x

for i in {1..50}; do
  ./rpcclient add,x,$i,x add,x,$i,x add,x,$i,x add,x,$i,x &
done
wait

./rpcclient add,x,0

./rpcclient -t dump,dumpfile.txt

./rpcclient add,x,0,z
./rpcclient setv,x,y
./rpcclient setv,y,z

./rpcclient add,1,1
./rpcclient add,1,z
./rpcclient add,z,1
./rpcclient add,z,z
./rpcclient add,1,1,a
./rpcclient add,1,z,a
./rpcclient add,z,1,a
./rpcclient add,z,z,a
./rpcclient addr,1,1
./rpcclient addr,1,x
./rpcclient addr,x,1
./rpcclient addr,x,x
./rpcclient addr,1,1,a
./rpcclient addr,1,x,a
./rpcclient addr,x,1,a
./rpcclient addr,x,x,a

