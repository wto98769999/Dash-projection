set terminal png truecolor size 960,960
set output "bufferLevel.png"   
set autoscale    
set style data linespoints 
set xlabel "Time/s" 
set ylabel "Buffer/s"
set title "BufferStatus" 
set grid
plot [0:120] [0:10] './festive/1/sim4_cl0_bufferLog.txt' using 1:2  title "festive" lw 1.2 ps 1.2 , './tobasco/1/sim4_cl0_bufferLog.txt' using 1:2  title "tobasco" lw 1.2 ps 1.2 ,'./tomato/1/sim4_cl0_bufferLog.txt' using 1:2  title "tomato" lw 1.2 ps 1.2 ,'./tomato2/1/sim4_cl0_bufferLog.txt' using 1:2  title "tomato2" lw 1.2 ps 1.2 

set terminal png truecolor size 960,960   
set output "bitrateLevel.png"  
set autoscale   
set style data linespoints
set xlabel "Time/s" 
set ylabel "RepIndex"
set title "RepIndex" 
set grid
plot [0:120] [0:15] './festive/1/sim4_cl0_adaptationLog.txt' using 3:2  title "festive" lw 1.2 ps 1.2 , './tobasco/1/sim4_cl0_adaptationLog.txt' using 3:2  title "tobasco" lw 1.2 ps 1.2 ,'./tomato/1/sim4_cl0_adaptationLog.txt' using 3:2  title "tomato" lw 1.2 ps 1.2 ,'./tomato2/1/sim4_cl0_adaptationLog.txt' using 3:2  title "tomato2" lw 1.2 ps 1.2 

set terminal png truecolor size 960,960   
set output "BandwidthEstimate.png"  
set autoscale   
set style data linespoints
set xlabel "Time/s" 
set ylabel "BandwidthEstimate/bps"
set title "BandwidthEstimate" 
set grid
plot [0:120] [0:160000000] './festive/1/sim4_cl0_adaptationLog.txt' using 3:4  title "festive" lw 1.2 ps 1.2 , './tobasco/1/sim4_cl0_adaptationLog.txt' using 3:4  title "tobasco" lw 1.2 ps 1.2 ,'./tomato/1/sim4_cl0_adaptationLog.txt' using 3:4  title "tomato" lw 1.2 ps 1.2 ,'./tomato2/1/sim4_cl0_adaptationLog.txt' using 3:4  title "tomato2" lw 1.2 ps 1.2 

set terminal png truecolor size 960,960   
set output "InstantBitrate.png"  
set autoscale   
set style data linespoints 
set xlabel "Time/s" 
set ylabel "InstantBitrate/bps"
set title "InstantBitrate" 
set grid
plot [0:120] [0:160000000] './festive/1/sim4_cl0_downloadLog.txt' using 4:8  title "festive" lw 1.2 ps 1.2 , './tobasco/1/sim4_cl0_downloadLog.txt' using 4:8  title "tobasco" lw 1.2 ps 1.2 ,'./tomato/1/sim4_cl0_downloadLog.txt' using 4:8  title "tomato" lw 1.2 ps 1.2 , './tomato2/1/sim4_cl0_downloadLog.txt' using 4:8  title "tomato2" lw 1.2 ps 1.2 

set terminal png truecolor size 960,960  
set output "PauseTime.png"  
set autoscale   
set style data impulses 
set xlabel "Time/s" 
set ylabel "PauseTime/ms"
set title "PauseTime" 
set grid
plot [0:120] [0:4000] './festive/1/sim4_cl0_adaptationLog.txt' using 3:5  title "festive" lw 4, './tobasco/1/sim4_cl0_adaptationLog.txt' using 3:5  title "tobasco" lw 4, './tomato/1/sim4_cl0_adaptationLog.txt' using 3:5  title "tomato" lw 4,'./tomato2/1/sim4_cl0_adaptationLog.txt' using 3:5  title "tomato2" lw 4