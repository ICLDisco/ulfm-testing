#!/bin/sh

stddev () { 
  awk '
    BEGIN{m=1e9;M=0e0}
    {x[NR]=$6; if($6<m){m=$6}; if($6>M){M=$6}; s+=$6}
    END{a=s/NR; 
        for (i in x){ss += (x[i]-a)^2} sd = sqrt(ss/NR); 
        print "SIZE= '$1'\tAVG= "a"\tSD= "sd"\tMIN= "m"\tMAX= "M"\tNR= "NR}'
}

threshold () {
  awk '
    {if($6<'$1') print $0}
  '

}

rm N R P*
#for size in $(seq $((64*1024)) $((32*1024)) $((320*1024))); do
for b in $(seq 3 23); do 
  size=$(dc -e "2 $b ^p");
  #tcut=`grep "N($size)" all  | cut -d'#' -f 3 | bimodal.pl | tee bimodal.$size.out | grep 'threshold is at' | cut -d' ' -f 4`
  tcut=1e9
  grep all -e "N($size)" | tr -d [] | threshold $tcut | stddev $size >>N
  grep all -e "R($size)" | tr -d [] | threshold $tcut | stddev $size >>R
  for run in $(seq 0 9); do
    grep all -e "P($size)" | grep -e "\[\\s*$run\]" | tr -d [] | threshold $tcut | stddev $size >>P$run
  done
done
exit  
