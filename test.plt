set title "Throughput Curve"
set terminal png
set datafile separator ','
set output "test.png"
plot "throughput-1.txt" using 1:2 w lp title "Node 0", "throughput-2.txt" using 1:2 w lp title "Node 1", "throughput-3.txt" using 1:2 w lp title "Node 2", "throughput-4.txt" using 1:2 w lp title "Node 3"


#set terminal png size 1080,480  
#set output "test.png"  
#plot "throughput-1.txt" using 1:2 "%lf,%lf" with linespoints title "Flow 1", "throughput-2.txt" using 1:2 "%lf,%lf" with linespoints title "Flow 2", "throughput-3.txt" using 1:2 "%lf,%lf" with linespoints title "Flow 3" 
