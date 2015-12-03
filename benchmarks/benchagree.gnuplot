#!/opt/local/bin/gnuplot -persist
#
#    
#    	G N U P L O T
#    	Version 4.6 patchlevel 6    last modified September 2014
#    	Build System: Darwin x86_64
#    
#    	Copyright (C) 1986-1993, 1998, 2004, 2007-2014
#    	Thomas Williams, Colin Kelley and many others
#    
#    	gnuplot home:     http://www.gnuplot.info
#    	faq, bugs, etc:   type "help FAQ"
#    	immediate help:   type "help"  (plot window: hit 'h')
set terminal pdfcairo enhanced linewidth 2.0 font "Helvetica-Bold,14"
# set terminal aqua 0 title "Figure 0" size 846,594 font "Times-Roman,14" noenhanced solid
set output "era.pdf"
unset clip points
set clip one
unset clip two
set bar 1.000000 front
set border 31 front linetype -1 linewidth 1.000
set boxwidth
set style fill  empty border
set style rectangle back fc  lt -3 fillstyle   solid 1.00 border lt -1
set style circle radius graph 0.02, first 0, 0 
set style ellipse size graph 0.05, 0.03, first 0 angle 0 units xy
set dummy x,y
#set format x "% g"
set format x "%.0s%c"
set format y "% g"
set format x2 "% g"
set format y2 "% g"
set format z "% g"
set format cb "% g"
set format r "% g"
set angles radians
set grid nopolar
set grid noxtics nomxtics ytics nomytics noztics nomztics \
 nox2tics nomx2tics noy2tics nomy2tics nocbtics nomcbtics
set grid layerdefault   linetype 0 linewidth 1.000,  linetype 0 linewidth 1.000
set raxis
set key title ""
set key inside left top vertical Left reverse enhanced autotitles nobox
set key noinvert samplen 4 spacing 1 width 0 height 0 
set key maxcolumns 0 maxrows 0
set key noopaque
unset label
unset arrow
set style increment default
unset style line
unset style arrow
set style histogram clustered gap 2 title  offset character 0, 0, 0
unset logscale
set offsets 0, 0, 0, 0
set pointsize 1
set pointintervalbox 1
set encoding default
unset polar
unset parametric
unset decimalsign
set view 60, 30, 1, 1
set samples 100, 100
set isosamples 10, 10
set surface
unset contour
set clabel '%8.3g'
set mapping cartesian
set datafile separator whitespace
unset hidden3d
set cntrparam order 4
set cntrparam linear
set cntrparam levels auto 5
set cntrparam points 5
set size ratio 0 1,1
set origin 0,0
set style data points
set style function lines
set xzeroaxis linetype -2 linewidth 1.000
set yzeroaxis linetype -2 linewidth 1.000
set zzeroaxis linetype -2 linewidth 1.000
set x2zeroaxis linetype -2 linewidth 1.000
set y2zeroaxis linetype -2 linewidth 1.000
set ticslevel 0.5
set mxtics default
set mytics default
set mztics default
set mx2tics default
set my2tics default
set mcbtics default
set xtics border in scale 1,0.5 mirror norotate  offset character 0, 0, 0 autojustify
set xtics autofreq  norangelimit
set ytics border in scale 1,0.5 mirror norotate  offset character 0, 0, 0 autojustify
set ytics autofreq  norangelimit
set ztics border in scale 1,0.5 nomirror norotate  offset character 0, 0, 0 autojustify
set ztics autofreq  norangelimit
set nox2tics
set noy2tics
set cbtics border in scale 1,0.5 mirror norotate  offset character 0, 0, 0 autojustify
set cbtics autofreq  norangelimit
set rtics axis in scale 1,0.5 nomirror norotate  offset character 0, 0, 0 autojustify
set rtics autofreq  norangelimit
set title "" 
set title  offset character 0, 0, 0 font "" norotate
set timestamp bottom 
set timestamp "" 
set timestamp  offset character 0, 0, 0 font "" norotate
set rrange [ * : * ] noreverse nowriteback
set trange [ * : * ] noreverse nowriteback
set urange [ * : * ] noreverse nowriteback
set vrange [ * : * ] noreverse nowriteback
set xlabel "#processes" 
set xlabel  offset character 0, 0.5, 0 font "" textcolor lt -1 norotate
set x2label "" 
set x2label  offset character 0, 0, 0 font "" textcolor lt -1 norotate
set xrange [ 720.000 : 6000.00 ] noreverse nowriteback
set x2range [ * : * ] noreverse nowriteback
set ylabel "us" 
set ylabel  offset character 2, 0, 0 font "" textcolor lt -1 rotate by -270
set y2label "" 
set y2label  offset character 0, 0, 0 font "" textcolor lt -1 rotate by -270
set yrange [ 1.00000 : * ] noreverse nowriteback
set y2range [ * : * ] noreverse nowriteback
set zlabel "" 
set zlabel  offset character 0, 0, 0 font "" textcolor lt -1 norotate
set zrange [ * : * ] noreverse nowriteback
set cblabel "" 
set cblabel  offset character 0, 0, 0 font "" textcolor lt -1 rotate by -270
set cbrange [ * : * ] noreverse nowriteback
set zero 1e-08
set lmargin  -1
set bmargin  -1
set rmargin  -1
set tmargin  -1
set locale "en_US.UTF-8"
set pm3d explicit at s
set pm3d scansautomatic
set pm3d interpolate 1,1 flush begin noftriangles nohidden3d corners2color mean
set palette positive nops_allcF maxcolors 0 gamma 1.5 color model RGB 
set palette rgbformulae 7, 5, 15
set colorbox default
set colorbox vertical origin screen 0.9, 0.2, 0 size screen 0.05, 0.6, 0 front bdefault
set style boxplot candles range  1.50 outliers pt 7 separation 1 labels auto unsorted
set loadpath 
set fontpath 
set psdir
set fit noerrorvariables noprescale
set style fill transparent solid 0.5 noborder
GNUTERM = "aqua"
#set logscale y 10
set style line 1 lc rgb "gray70" ps 0.4 pt 5
set style line 2 lc rgb "gray50" ps 0.5 pt 13
set style line 3 lc rgb "orange" ps 0.5 pt 9
set style line 4 lc rgb "light-red" ps 0.5 pt 11
set style line 5 lc rgb "medium-blue" ps 0.5 pt 7
set style line 6 lc rgb "purple" ps 0.5 pt 22
set title "ERA Topologies (Cray XC30)" offset 0,-0.5
plot \
 "ULFM.o420099.era+4.gz.imb" u 1:3:4 w filledcu ls 1 t "" \
,"IMB.o420839.gz.imb" u 1:3:4 w filledcu ls 2 t "" \
,"erasm+1.2" u 1:(($2-$3)*1e6):(($2+$3)*1e6) w filledcu ls 3 t "" \
,"ULFM.o419764.gz.after" u 1:(($2-$3)*1e6):(($2+$3)*1e6) w filledcu ls 5 t "" \
,"ULFM.o419310.gz.after" u 1:(($2-$3)*1e6):(($2+$3)*1e6) w filledcu ls 4 t "" \
,"ULFM.o419764.gz.after" u 1:($2*1e6) w lp ls 5 t "ERA Agreement(flat binary tree)" \
,"erasm+1.2" u 1:($2*1e6) w lp ls 3 t "ERA Agreement(hierarchical bin/star tree)" \
,"ULFM.o419310.gz.after" u 1:($2*1e6) w lp ls 4 t "ERA Agreement(hierarchical bin/bin tree)" \
,"ULFM.o420099.era+4.gz.imb" u 1:2 w lp ls 1 t "Open MPI Allreduce(4Bytes)" \
,"IMB.o420839.gz.imb" u 1:2 w lp ls 2 t "Cray Allreduce(4Bytes)" \
#,"ULFM.o420099.era+4.gz.after" u 1:($2*1e6) w lp lc 4 ps 0.5 t "ERA Agreement(local-tree w/bin-bin)" \
#,"ULFM.era-4.o420898.gz.after" u 1:(($2-$3)*1e6):(($2+$3)*1e6) w filledcu t "" fc 1 \
#,"ULFM.era-4.o420898.gz.after" u 1:($2*1e6) w lp lc 2 ps 0.5 t "ERA Agreement(flat-tree w/binary)" \


set output "erasmall.pdf"
set xrange [16:6100]
set yrange [1:500]
#set logscale y 2
set logscale x 2
set size square
set title "Log2phases vs ERA Scalability (Cray XC30)" offset 0,-0.5

imb(x) = a*log(x)/log(2)
fit imb(x) "<cat IMB.o420839.gz.imb  IMB.o424474.gz.imb | sort -n" u 1:2 via a
#cat ULFM.o420147.era-4.gz.imb ULFM.o420099.era+4.gz.imb #OMPI IMB
ulfm(x) = c*log(x)/log(2)
fit ulfm(x) "<cat ULFM.o420146.era+4.gz.after ULFM.o419310.gz.after | sort -n" u 1:($2*1e6) via c

plot \
 "<cat ULFM.o420168.l2p.gz.after l2p | sort -n" u 1:($2*1e6) w lp ls 6 t "Log2phases" \
,"<cat ULFM.o420146.era+4.gz.after ULFM.o419310.gz.after | sort -n" u 1:($2*1e6) w lp ls 4 t "ERA(bin-bin tree)" \
,ulfm(x) t "6.2 log_2(x)" ls 3 lw 0.5 \
,"<cat IMB.o420839.gz.imb  IMB.o424474.gz.imb | sort -n" u 1:2 w lp ls 2 t "Cray Allreduce(4Bytes)" \
,imb(x) t "2.2 log_2(x)" ls 1 lw 0.5 \


# "<cat ULFM.o420146.era+4.gz.after ULFM.o419310.gz.after | sort -n" u 1:(($2-$3)*1e6):(($2+$3)*1e6) w filledcu fc 4 t "" \
#,"<cat ULFM.o420168.l2p.gz.after l2p | sort -n" u 1:(($2-$3)*1e6):(($2+$3)*1e6) w filledcu fc 5 t "" \
# "<cat ULFM.o420147.era-4.gz.imb ULFM.o419764.gz.imb | sort -n" u 1:3:4 w filledcu t "" fc 1\
#,"<cat ULFM.o420147.era-4.gz.after ULFM.o419764.gz.after | sort -n" u 1:(($2-$3)*1e6):(($2+$3)*1e6) w filledcu t "" fc 2 \
#,"<cat ULFM.o420147.era-4.gz.after ULFM.o419764.gz.after | sort -n" u 1:($2*1e6) w lp lc 2 ps 0.5 t "ERA Agreement(flat-tree w/binary)" \



#,"erasm-1.2" u 1:($2*1e6):($3*1e6) w errorlines t "ERA Agreement(flat-tree w/binary)" \
#,"ULFM.o417567.gz.after" u 1:($2*1e6):($3*1e6) w errorlines t "ERA Agreement(sm nolocal)" \
#,"erasm-1" u 1:($2*1e6):($3*1e6) w errorlines t "ERA Agreement(sm nolocal)" \
#,"erasm+1" u 1:($2*1e6):($3*1e6) w errorlines t "ERA Agreement(sm local)" \
#,"l2p" u 1:($2*1e6):($3*1e6) w errorlines t "Log2P Agreement" \
#,"all" u 1:($2*1e6):($3*1e6) w errorlines t "ERA Agreement(vader nolocal)" lc 7 \
#,"old" u 1:($2*1e6):($3*1e6) w errorlines t "Jan 20" \
#,"locality" u 1:($2*1e6):($3*1e6) w errorlines t "locality (buggy version)"
#,"imb128s" u 1:6:4:5 w lp t "allreduce(128bytes)" \

#    EOF
