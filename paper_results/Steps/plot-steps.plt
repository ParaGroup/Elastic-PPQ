# Script to generate a multiplot of the experiments with various steps
mpl_top    = 0.4 # inch  outer top margin, title goes here
mpl_bot    = 0.7 # inch  outer bottom margin, x label goes here
mpl_left   = 0.9 # inch  outer left margin, y label goes here
mpl_right  = 0.1 # inch  outer right margin, y2 label goes here
mpl_height = 1.2 # inch  height of individual plots
mpl_width  = 1.5 # inch  width of individual plots
mpl_dx     = 0.1 # inch  inter-plot horizontal spacing
mpl_dy     = 0.45 # inch  inter-plot vertical spacing
mpl_ny     = 1   # number of rows
mpl_nx     = 2   # number of columns

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
set output 'steps.eps'
set offsets
set autoscale fix
set size ratio 0.9

# Start plotting
set multiplot

# Grid style
set grid back lw 1 lt 0 lc rgb '#808080'

# Key style
set key horizontal spacing 1 samplen 1 width -2 font "Helvetica,11"
set key vertical maxcolumns 1

# Line styles
LINEWIDTH = 'lw 2.5'
set style line 1 @LINEWIDTH lt 1 lc rgb "blue" pointtype 5 pointsize 1
set style line 2 @LINEWIDTH lt 1 lc rgb "#E2601F" pointtype 7 pointsize 1
set style line 3 @LINEWIDTH lt 1 lc rgb "black" pointtype 3 pointsize 1
set style line 4 @LINEWIDTH lt 1 lc rgb "#BC20D4" pointtype 13 pointsize 1
set style line 5 @LINEWIDTH lt 1 lc rgb "red"

# Subplot 1-1
# Set horizontal margins for first column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
# Define x-axis setting
set xrange[0:600]
set xtics (100, 250, 500) font "Helvetica,9" offset 0,0.7,0
set xlabel "PID interval (ms)" font "Helvetica,10" offset 0,1.8,0
set mxtics 1
set title 'Varying the PID interval' font "Helvetica,11" offset 0,-0.5,0
set ylabel "PID accuracy (%)" font "Helvetica,10" offset 4.5,0,0
set yrange[-5:15]
set ytics (0, 5, 10, 15) nomirror font "Helvetica,9" offset 0.7,0,0
plot "pid-errors.txt" using 1:(100-(($2*100)/0.9)) with linespoints ls 1 title "Tstep=1sec", "" using 1:(100-(($3*100)/0.9)) with linespoints ls 2 title "Tstep=2.5sec", "" using 1:(100-(($4*100)/0.9)) with linespoints ls 3 title "Tstep=5sec", "" using 1:(100-(($5*100)/0.9)) with linespoints ls 4 title "Tstep=10sec", 0 with lines ls 5 notitle

# Subplot 2-1
# Set horizontal margins for first column
set lmargin at screen left(2)
set rmargin at screen right(2)
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
# Redefine plot type
set style data histogram
set style histogram cluster gap 1.8
set style fill solid 0.8 border 0
set boxwidth 0.8 relative
unset xrange
set xlabel "Elasticity step" font "Helvetica,10" offset 0,1.8,0
set title 'Varying the elasticity step' font "Helvetica,11" offset 0,-0.5,0
set ylabel "Throughput CV" font "Helvetica,10" offset 5.3,0,0
set yrange[0:2.2]
set ytics 0.4 nomirror font "Helvetica,9" offset 0.7,0,0
plot "bw-cv.txt" using 2:xtic(1) with histogram ls 1 notitle

unset multiplot

system("sh crop.sh")