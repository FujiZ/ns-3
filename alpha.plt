set terminal png size 1280,480  
set output "alpha.png"  
#set yrange [0.0:0.03]
plot "alpha.0.dat" u 4:5 w lp title "Node 0", "alpha.1.dat" u 4:5 w lp title "Node 1", "alpha.2.dat" u 4:5 w lp title "Node 2", "alpha.5.dat" u 4:5 w lp title "Server"
#plot "throughput-1.txt" using 1:2 "%lf,%lf" with linespoints title "Flow 1", "throughput-2.txt" using 1:2 "%lf,%lf" with linespoints title "Flow 2", "throughput-3.txt" using 1:2 "%lf,%lf" with linespoints title "Flow 3" 
