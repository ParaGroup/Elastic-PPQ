# Script to plot the experimental results of the external load experiments
mpl_top    = 0.4 # inch  outer top margin, title goes here
mpl_bot    = 0.7 # inch  outer bottom margin, x label goes here
mpl_left   = 0.9 # inch  outer left margin, y label goes here
mpl_right  = 0.1 # inch  outer right margin, y2 label goes here
mpl_height = 1.2 # inch  height of individual plots
mpl_width  = 1.5 # inch  width of individual plots
mpl_width2  = 2.4 # inch  width of individual plots
mpl_dx     = 0.25 # inch  inter-plot horizontal spacing
mpl_dy     = 0.15 # inch  inter-plot vertical spacing
mpl_ny     = 1   # number of rows
mpl_nx     = 3   # number of columns

# Calculate full dimensions
xsize = mpl_left+mpl_right+(mpl_width*2+mpl_width2)+(mpl_nx-1)*mpl_dx
ysize = mpl_top+mpl_bot+(mpl_ny*mpl_height)+(mpl_ny-1)*mpl_dy

# Placement functions
# Rows are numbered from bottom to top
bot(n) = (mpl_bot+(n-1)*mpl_height+(n-1)*mpl_dy)/ysize
top(n)  = 1-((mpl_top+(mpl_ny-n)*(mpl_height+mpl_dy))/ysize)
# Columns are numbered from left to right
left1 = (mpl_left)/xsize
left2 = (mpl_left+mpl_width+0.75*mpl_dx)/xsize
left3 = (mpl_left+mpl_width+mpl_width2+2*mpl_dx)/xsize
right1 = 1-((mpl_right+(mpl_width+mpl_dx)+(mpl_width2+mpl_dx))/xsize)
right2 = 1-((mpl_right+(mpl_width+mpl_dx))/xsize)
right3 = 1-((mpl_right)/xsize)

# Initialization
set terminal postscript eps enhanced color dl 2.0 size xsize,ysize "Helvetica" 28
set encoding iso_8859_1
set tics scale 1.5
set output 'interference.eps'
set offsets
set autoscale fix
set size ratio 0.9

# Start plotting
set multiplot

# Grid style
set grid back lw 1 lt 0 lc rgb '#808080'

# Key style
set key horizontal spacing 1 samplen 1 width -2 font "Helvetica,12"
set key vertical maxcolumns 1

# Line styles
LINEWIDTH = 'lw 2.5'
set style line 1 @LINEWIDTH lt 1 lc rgb "blue" pointtype 3 pointsize 0.8
set style line 2 @LINEWIDTH lt 1 lc rgb "red" pointtype 7 pointsize 0.8

# Subplot 1-1
# Set horizontal margins for first column
set lmargin at screen left1
set rmargin at screen right1
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
# Plot type
set style data histogram
set style histogram cluster gap 1.8
set style fill solid 0.8 border 0
set boxwidth 0.8 relative
# Plotting
set title "Average total thread level" font "Helvetica,12" offset 0,-0.5,0
set xlabel "External load level" font "Helvetica,11" offset 0,1.6,0
set xtics font "Helvetica,10" offset 0,0.7,0
set ylabel "Thread level (PLQ+WLQ)" font "Helvetica,11" offset 4.2,0,0
set yrange [0:50]
set ytics 10 font "Helvetica,10" offset 0.7,0,0
plot "cores.txt" using 2:xtic(1) with histogram ls 1 notitle

# Subplot 2-1
set size ratio 0.5
# Set horizontal margins for first column
set lmargin at screen left2
set rmargin at screen right2
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
# Plot type
set style data linespoints
# Plotting
set title "Total thread level over the execution" font "Helvetica,12" offset 0,-0.5,0
set xlabel "Time (sec)" font "Helvetica,11" offset 0,1.6,0
set xrange [0:350]
set ylabel "Thread level (PLQ+WLQ)" font "Helvetica,11" offset 4.2,0,0
set yrange [0:80]
set ytics 20 font "Helvetica,10" offset 0.7,0,0
plot "results_game1/skyline_controller_0.1.log" using ($1*2.5):($5+$6) with linespoints ls 1 title "external load=0.1", "results_game1/skyline_controller_0.6.log" using ($1*2.5):($5+$6) with linespoints ls 2 title "external load=0.6"

# Subplot 3-1
set size ratio 0.9
# Set horizontal margins for first column
set lmargin at screen left3
set rmargin at screen right3
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
# Plot type
set style data histogram
set style histogram cluster gap 1.8
set style fill solid 0.8 border 0
set boxwidth 0.8 relative
# Plotting
set title "Throughput under external load" font "Helvetica,12" offset 0,-0.5,0
set xlabel "External load level" font "Helvetica,11" offset 0,1.6,0
set ylabel "Throughput (\%)" font "Helvetica,11" offset 5.2,0,0
set yrange [0:125]
set ytics (0, 25, 50, 75, 100) font "Helvetica,10" offset 0.7,0,0
set y2range[0:4]
set y2label "Throughput CV" font "Helvetica,11" offset -5.2,0,0
set y2tics (0, 0.8, 1.6, 2.4, 3.2) nomirror font "Helvetica,10" offset -1,0,0
unset xrange
plot "bw.txt" using (($2*100)/5):xtic(1) with histogram ls 1 title "throughput", "" using 3:xtic(1) with histogram ls 2 fs pattern 2 axes x1y2 title "coeff. variation"

system("sh crop.sh")