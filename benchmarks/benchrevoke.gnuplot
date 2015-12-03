#load "style.gnp"

set grid front
set logscale x 8
set logscale y 10

dred="#DC143C"
orange="#FF8C00"
violet="#DC5ADC"

set xtics ("1" 1, "4" 4,"16" 16, "64" 64, "256" 256, "1K" 1024, "4K" 4096, "16K" 16384, "64K" 65536, "256K" 262144, "1M" 1048576, "4M" 4194304, "16M" 16777216, "64M" 67108864, "256M" 268435456, "1G" 1073741824, "2G" 2147483648.0, "4G" 4294967296.0) rotate by 0 offset 0
#set format x "%.0s%c"

set output "revoke.pdf"
set terminal pdf enhanced color font "helvetica-Bold,12" size 10cm,10cm linewidth 2
set multiplot
set origin 0, 0
set size 1, 1
set title "Revoke Time and Perturbation in AllReduce (np=128, IB20G)"
set key top left Left reverse

#set xrange [64*1024:400*1024]
set xrange [16:8*1024*1024]
set yrange [*:*]
#set ytics 1
set xlabel "MESSAGE SIZE (Bytes)"
set ylabel "TIME (ms)" offset 3

set style fill transparent solid 0.5 noborder

plot \
 "N" u 2:($4*1e3) w lp t "Fault Free AllReduce"  lt 2 lc 8 pt 6, \
 "N" u 2:(($8)*1e3):(($4+$6)*1e3) w filledcu t "Fault Free [Min:Avg+Standard Dev.]" lc 8, \
 "R" u 2:($4*1e3) w lp t "Revoked AllReduce"  lt 1 lc rgb orange pt 10, \
 "P0" u 2:($4*1e3) w lp t "1^{st} post-revoke AllReduce"  lt 1 lc rgb violet pt 2, \
 "P0" u 2:(($8)*1e3):(($4+$6)*1e3) w filledcu t "1^{st} post-revoke [Min:Avg+Standard Dev]" lc rgb violet, \
 "P1" u 2:($4*1e3) w l t "2^{nd} post-revoke AllReduce"  lt 1 lc rgb "royalblue", \
 "P2" u 2:($4*1e3) w l t "3^{rd} post-revoke AllReduce"  lt 1 lc rgb "sea-green" 

unset multiplot 
set output "revoke-darter.pdf"
set title "Revoke Time and Perturbation in AllReduce (np=4096, Darter, Ugni)"
set xrange[32:16*1024]
plot \
 "darter/N" u 2:($4*1e3) w lp t "Fault Free AllReduce"  lt 2 lc 8 pt 6, \
 "darter/N" u 2:(($8)*1e3):(($4+$6)*1e3) w filledcu t "Fault Free [Min:Avg+Standard Dev.]" lc 8, \
 "darter/R" u 2:($4*1e3) w lp t "Revoked AllReduce"  lt 1 lc rgb orange pt 10, \
 "darter/P0" u 2:($4*1e3) w lp t "1^{st} post-revoke AllReduce"  lt 1 lc rgb violet pt 2, \
 "darter/P0" u 2:(($8)*1e3):(($4+$6)*1e3) w filledcu t "1^{st} post-revoke [Min:Avg+Standard Dev]" lc rgb violet, \
 "darter/P1" u 2:($4*1e3) w l t "2^{nd} post-revoke AllReduce"  lt 1 lc rgb "royalblue", \
 "darter/P2" u 2:($4*1e3) w l t "3^{rd} post-revoke AllReduce"  lt 1 lc rgb "sea-green" 


 set output "revoke-darter6k.pdf"
 set title "Revoke Time and Perturbation in AllReduce (np=6000)"
 set ylabel "TIME (us)" offset 3
 set xrange[32:8*1024*1024]
 set mytics 10
 set format y "%g"
 unit=1e6
 plot \
  "darter6k/6kar/N" u 2:($4*unit) w lp t "Fault Free AllReduce"  lt 2 lc 8 pt 6, \
  "darter6k/6kar/N" u 2:(($8*unit)):(($4+$6)*unit) w filledcu t "Fault Free [Min:Avg+Standard Dev.]" lc 8, \
  "darter6k/6kar/R" u 2:($4*unit) w lp t "Revoked AllReduce"  lt 1 lc rgb orange pt 10, \
  "darter6k/6kar/R" u 2:(($8*unit)):(($4+$6)*unit) w filledcu t "Revoked AllReduce [Min:Avg+stddev]"  lt 1 lc rgb orange, \
  "darter6k/6kar/P0" u 2:($4*unit) w lp t "1^{st} post-revoke AllReduce"  lt 1 lc rgb violet pt 2, \
  "darter6k/6kar/P0" u 2:(($8*unit)):(($4+$6)*unit) w filledcu t "1^{st} post-revoke [Min:Avg+Standard Dev]" lc rgb violet, \
  "darter6k/6kar/P1" u 2:($4*unit) w l t "2^{nd} post-revoke AllReduce"  lt 1 lc rgb "royalblue", \
  "darter6k/6kar/P2" u 2:($4*unit) w l t "3^{rd} post-revoke AllReduce"  lt 1 lc rgb "sea-green", \
  "darter6k/IMBAR6k" u 1:($5) w l dashtype 2 lc "gray" t "Cray AllReduce"

#  "darter6k/6kar/P9" u 2:($4*1e3) w l t "9th post-revoke AllReduce"  lt 1 lc rgb "green" 

set ylabel offset 1
set yrange [*:180]

set output "revoke-sar.pdf"
set title "Revoke Time and Perturbation in AllReduce (4Bytes)"
set xlabel "#PROCESSES"
set xrange [1500:6000]
set xtics 1000
set format x "%.0s%c"
unset logscale x
unset logscale y
plot \
 "darter6k/sar/REVOKE.scalar.o435516.gz.N8" u 1:($2*unit) w lp t "Fault Free AllReduce" lt 2 lc 8 pt 6 \
,"darter6k/sar/REVOKE.scalar.o435516.gz.R8" u 1:($2*unit) w lp t "Revoked AllReduce" lt 1 lc rgb orange pt 10 \
,"darter6k/sar/REVOKE.scalar.o435516.gz.P8-0" u 1:($2*unit) w lp t "1^{st} post-revoke AllReduce"  lt 1 lc rgb violet pt 2 \
,"darter6k/sar/REVOKE.scalar.o435516.gz.P8-1" u 1:($2*unit) w l t "2^{nd} post-revoke AllReduce"  lt 1 lc rgb "royalblue" \
,"darter6k/sar/REVOKE.scalar.o435516.gz.P8-2" u 1:($2*unit) w l t "3^{rd} post-revoke AllReduce"  lt 1 lc rgb "sea-green"


set output "revoke-sbr.pdf"
set title "Revoke Time and Perturbation in Barrier"
set xlabel "#PROCESSES"
set xrange [1500:6000]
set xtics 1000
set format x "%.0s%c"
unset logscale x
unset logscale y
plot \
 "darter6k/sbr/REVOKE.scalb.o435515.gz.N0" u 1:($2*unit) w lp t "Fault Free Barrier" lt 2 lc 8 pt 6 \
,"darter6k/sbr/REVOKE.scalb.o435515.gz.R0" u 1:($2*unit) w lp t "Revoked Barrier" lt 1 lc rgb orange pt 10 \
,"darter6k/sbr/REVOKE.scalb.o435515.gz.P0-0" u 1:($2*unit) w lp t "1^{st} post-revoke Barrier"  lt 1 lc rgb violet pt 2 \
,"darter6k/sbr/REVOKE.scalb.o435515.gz.P0-1" u 1:($2*unit) w l t "2^{nd} post-revoke Barrier"  lt 1 lc rgb "royalblue" \
,"darter6k/sbr/REVOKE.scalb.o435515.gz.P0-6" u 1:($2*unit) w l t "5^{th} post-revoke Barrier"  lt 1 lc rgb "sea-green"


#6000 0.000115156 0.00052309 0.000135147 1.74246e-05 0
brffavg(x)=0.000135147
brffstd(x)=0.000135147+1.74246e-05
brffmin(x)=0.000115156

set output "revoke-rrbr.pdf"
set title "Revoke Time and Perturbation in Barrier (np=6000)"
set xlabel "Revoke Initiator Rank"
set xrange [0:6000]
set xtics 1000
set format x "%.0s%c"
unset logscale x
unset logscale y
plot \
 brffmin(x)*unit w l t "Fault Free Barrier" lt 2 lc 8 dt 2\
,"darter6k/6kbr/REVOKE.6kbrf.o437644.gz.R0" u 6:($2*unit) w lp t "Revoked Barrier" lt 1 lc rgb orange pt 10 \
,"darter6k/6kbr/REVOKE.6kbrf.o437644.gz.P0-0" u 6:($2*unit) w lp t "1^{st} post-revoke Barrier"  lt 1 lc rgb violet pt 2 \
,"darter6k/6kbr/REVOKE.6kbrf.o437644.gz.P0-1" u 6:($2*unit) w l t "2^{nd} post-revoke Barrier"  lt 1 lc rgb "royalblue" \
,"darter6k/6kbr/REVOKE.6kbrf.o437644.gz.P0-6" u 6:($2*unit) w l t "5^{th} post-revoke Barrier"  lt 1 lc rgb "sea-green"

#,brffstd(x)*unit w filledcu y1=0.000115156*unit t "Fault Free [Min:Avg+Standard Dev.]" lc 8 \


#,"darter6k/sar/REVOKE.scalar.o435516.gz.N8" u 1:(($2*unit)):(($4+$5)*unit) w filledcu t "Fault Free [Min:Avg+Standard Dev.]" lc 8 \

#    EOF
