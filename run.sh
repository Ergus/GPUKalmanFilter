#!/bin/bash

resudir="Results"
resufile="res_"$(date +"%d_%m_%Y_%H:%M").out

#Check for Results dir
if ! [ -d resudir ]; then
    echo "Creating Results Folder: "$resudir
    mkdir $resudir
    fi

echo "Outputs goes to: "$resudir/$resufile

for i in {1..200..10}; do
    printf "Filtering file for: %d events\n" $i 
    awk -v env=$i '$1=="Event:"{cont++}{if(cont<=env)print $0;else exit 0}' in.dat > /tmp/out.dat
    for a in *.x;do
        echo "Running " $a
        ./$a /tmp/out.dat >> $resudir/$resufile 2>&1
        done
    echo "-----------------------------" >> $resudir/$resufile
    done

echo "Outputs went to: "$resudir/$resufile
