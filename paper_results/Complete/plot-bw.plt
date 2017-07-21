# Script to generate a multiplot of the bandwidth of the Game1 dataset
mpl_top    = 0.4 # inch  outer top margin, title goes here
mpl_bot    = 0.7 # inch  outer bottom margin, x label goes here
mpl_left   = 0.9 # inch  outer left margin, y label goes here
mpl_right  = 0.1 # inch  outer right margin, y2 label goes here
mpl_height = 1.2 # inch  height of individual plots
mpl_width  = 1.7 # inch  width of individual plots
mpl_dx     = 0.1 # inch  inter-plot horizontal spacing
mpl_dy     = 0.45 # inch  inter-plot vertical spacing
mpl_ny     = 1   # number of rows
mpl_nx     = 1   # number of columns

# Calculate full dimensions
xsize = mpl_left+mpl_right+(mpl_width*mpl_nx)+(mpl_nx-1)*mpl_dx
ysize = mpl_top+mpl_bot+(mpl_ny*mpl_height)+(mpl_ny-1)*mpl_dy

# Placement functions
# Rows are numbered from bottom to top
bot(n) = (mpl_bot+(n-1)*mpl_height+(n-1)*mpl_dy)/ysize
top(n)  = 1-((mpl_top+(mpl_ny-n)*(mpl_height+mpl_dy))/ysize)
# Columns are numbered from left to right
left(n) = (mpl_left+(n-1)*mpl_width+(n-1)*mpl_dx)/xsize
right(n)  = 1-((mpl_right+(mpl_nx-n)*(mpl_width+mpl_dx))/xsize)

# Initialization
set terminal postscript eps enhanced color dl 2.0 size xsize,ysize "Helvetica" 28
set encoding iso_8859_1
set tics scale 1.5
set output 'real-bw.eps'
set offsets
set autoscale fix
set size ratio 0.3

# Start plotting
set multiplot

# Grid style
set grid back lw 1 lt 0 lc rgb '#808080'

# Key style
set key horizontal spacing -0.2 samplen 1 width -4 font "Helvetica,10"
set key vertical maxrows 1

# Line styles
LINEWIDTH = 'lw 1.5'
set style line 1 @LINEWIDTH lt 1 lc rgb "#4C8C42" pointtype 2 pointsize 0.6
set style line 2 @LINEWIDTH lt 1 lc rgb "black" pointtype 12 pointsize 0.6
set style line 3 @LINEWIDTH lt 1 lc rgb "red"

# Define x-axis settings for all subplots
set xlabel "Time (sec)" font "Helvetica,9" offset 0,1.8,0
set xrange [0:350]
set xtics 50 font "Helvetica,8" offset 0,0.7,0 nomirror
set mxtics 2

# Subplot 1-1
# Set horizontal margins for first column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'Throughput of the system' font "Helvetica,10" offset 0,-0.5,0
set ylabel "Windows/sec" font "Helvetica,9" offset 4.5,0,0
set yrange[0:40]
set ytics 8 nomirror font "Helvetica,8" offset 0.7,0,0
plot "results_real_r0.7/skyline_client_Game1.log" using 1:2 with linespoints ls 1 title "adaptive", "results_max/client_Game1_max6-38.log" using 1:2 with linespoints ls 2 title "static", 5 with lines ls 3 title "setpoint (5)"
unset multiplot

system("sh crop.sh")