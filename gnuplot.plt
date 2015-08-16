#!/usr/bin/gnuplot

set term postscript eps enhanced color linewidth 2 dashlength 4
### Files
floop = "Results/lab14_06_08_2015_loop.res"
fbadgood = "Results/lab14_07_08_2015_bad_good.res"
fcpu = "Results/lab14_07_08_2015_cpu_loop.res"
fpointer = "Results/lab14_07_08_2015_pointer.res"
### Arrays
serials = "Bad Good Bad_Good"
titles1 = "AOS SOA SOA+conversion"
GPUs = "OpenCl1 OpenCl2 Cuda1 Cuda2"
### Times ########################
set grid
set key left top
set xlabel "Tracks"
set ylabel "Time (s)"

### Time Serial Code #############
### This graph is only the serial code times ################
set title "Time vs Number of Tracks from 1 to 300 events CPU code"
plotCPU = "<awk '$4==\"%s\"&&/Time/&&/CPU/{print $8,$12}' %s"

set output '|epstopdf --filter --outfile=Graphs/Time_Serial_Code.pdf'

plot for [i=1:words(serials)] sprintf(plotCPU,word(serials,i),fbadgood) t word(titles1,i) w linespoints pt 7 lw 0.5 ps 0.75

### GPU times#####################
### This is Cuda and OpenCL running on GPU with copy implementation only.
set title "Time vs Number of Tracks from 1 to 300 events GPU copy code"

plotGPU = "<awk '$4==\"%s\"&&/Time/&&/CPU/&&/Kernel/{print $8,$12}' %s"
set output '|epstopdf --filter --outfile=Graphs/Time_GPU_Code.pdf'

plot for [i=1:words(GPUs)] sprintf(plotGPU,word(GPUs,i),floop) t word(GPUs,i) w linespoints pt 7 lw 0.5 ps 0.75

### CPU times#####################
### This is the comparison of the times running the same CPU with serial code and OpenCL.
### This don't take into account the copy time only solution time.
set title "Time vs Number of Tracks from 1 to 300 events CPU code, compare OpenCl"
set output '|epstopdf --filter --outfile=Graphs/Time_CPU_Code.pdf'

plot for [i=1:words(serials)] sprintf(plotCPU,word(serials,i),fbadgood) t word(titles1,i) w linespoints pt 7 lw 0.5 ps 0.75,\
     for [i=1:2] sprintf(plotGPU,word(GPUs,i),fcpu) t word(GPUs,i) w linespoints pt 7 lw 0.5 ps 0.75

### Pointer times Cuda #################
### The comparison time betwen loop copy and pinter access to global memory
### Only for Cuda.
set output '|epstopdf --filter --outfile=Graphs/Time_pointer_Cuda.pdf'

plot for [i=3:4] sprintf(plotGPU,word(GPUs,i),floop) t "loop ".word(GPUs,i) w linespoints pt 7 lw 0.5 ps 0.75, \
     for [i=3:4] sprintf(plotGPU,word(GPUs,i),fpointer) t "pointer ".word(GPUs,i) w linespoints pt 7 lw 0.5 ps 0.75

### Pointer times OpenCl #################
### The comparison time betwen loop copy and pinter access to global memory
### Only for OpenCl.
set output '|epstopdf --filter --outfile=Graphs/Time_pointer_OpenCl.pdf'

plot for [i=1:2] sprintf(plotGPU,word(GPUs,i),floop) t "loop ".word(GPUs,i) w linespoints pt 7 lw 0.5 ps 0.75, \
     for [i=1:2] sprintf(plotGPU,word(GPUs,i),fpointer) t "pointer ".word(GPUs,i) w linespoints pt 7 lw 0.5 ps 0.75

#------------------------------------------------
### Speed Up Serial Code###########
### Speedup for the CPU serial codes ####
plotSU = "<awk '$4==\"%s\"||$4==\"Bad\"&&/Time/&&/CPU/{if($4==\"Bad\"){bad[bc++]=$12}else{good[gc++]=$12;x[xc++]=$8}}END{for(i=0;i<xc;i++){print x[i], bad[i]/good[i]}}' %s"

set output '|epstopdf --filter --outfile=Graphs/SU_Serial_Code.pdf'
set key right top
set title "SpeedUp vs Number of Tracks from 1 to 300 events serial code"
set xlabel "Tracks"
set ylabel "SpeedUp"

plot for [i=2:words(serials)] sprintf(plotSU,word(serials,i),fbadgood) t word(titles1,i) w linespoints pt 7 lw 0.5 ps 0.75

### Speed Up OpenCl Code###########
### Speedup for the CPU serial codes ####

plotSU2 = "<awk '($4==\"Bad\"&&/total/){bad[bc++]=$12};/CPU/&&($4==\"%s\"&&/Kernel/){good[gc++]=$12;x[xc++]=$8}END{for(i=0;i<xc;i++){print x[i], bad[i]/good[i]}}' %s"

set output '|epstopdf --filter --outfile=Graphs/SU_OpenCl_Code.pdf'
set key left top
set title "SpeedUp vs Number of Tracks from 1 to 300 events OpenCl on CPU"
set xlabel "Tracks"
set ylabel "SpeedUp"

plot for [i=1:2] sprintf(plotSU2,word(GPUs,i),fcpu) t word(GPUs,i) w linespoints pt 7 lw 0.5 ps 0.75