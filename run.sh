#!/bin/bash

resudir="Results"
resufile=$(hostname)"_"$(date +"%d_%m_%Y_%H:%M").res

#Check for Results dir
if ! [ -d resudir ]; then
    echo "Creating Results Folder: "$resudir
    mkdir $resudir
    fi

echo "Outputs goes to: "$resudir/$resufile

#Set the number of threads as 85% of total cores gives best performance
printf -v OMP_NUM_THREADS %.0f $(echo $(grep processor /proc/cpuinfo | wc -l)*"0.85"| bc)
export OMP_NUM_THREADS
echo Using $OMP_NUM_THREADS threads for the OpenMP Code

for i in {1..20} {30..300..10}; do
    printf "Filtering file for: %d events\n" $i 
    awk -v env=$i '$1=="Event:"{cont++}{if(cont<=env)print $0;else exit 0}' in.dat > /tmp/out.dat
    for a in *.x;do
        echo "Running " $a
        ./$a /tmp/out.dat >> $resudir/$resufile 2>&1
        done
    echo "-----------------------------" >> $resudir/$resufile
    done

echo "Outputs went to: "$resudir/$resufile
