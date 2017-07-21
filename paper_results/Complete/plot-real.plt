# Script to plot the experimental results of the real dataset Game1
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
set output 'real-exp.eps'
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
LINEWIDTH = 'lw 1.5'
set style line 1 @LINEWIDTH lt 1 lc rgb "blue" pointtype 5 pointsize 0.5

# Define x-axis settings for all subplots
set xlabel "Time (sec)" font "Helvetica,11" offset 0,1.8,0
set xrange [0:350]
set xtics 50 font "Helvetica,10" offset 0,0.7,0
set mxtics 2

# Subplot 1-1
# Set horizontal margins for first column
set lmargin at screen left1
set rmargin at screen right1
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'Arrival rate' font "Helvetica,12" offset 0,-0.5,0
set ylabel "Tuples/sec" font "Helvetica,11" offset 5.5,0,0
set yrange [8000:26000]
set ytics ("8K" 8000, "12.5K" 12500, "17K" 17000, "21.5K" 21500, "26K" 26000) font "Helvetica,10" offset 0.7,0,0
plot "rates/arrivals_Game1_2x.txt" using 1:2 with linespoints ls 1 notitle

# Line styles
LINEWIDTH = 'lw 1.5'
set style line 1 @LINEWIDTH lt 1 lc rgb "blue"
set style line 2 @LINEWIDTH lt 1 lc rgb "#E2601F" pointtype 7 pointsize 0.5
set style fill solid
set boxwidth 0.7

# Subplot 2-1
set size ratio 0.5
# Set horizontal margins for first column
set lmargin at screen left2
set rmargin at screen right2
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'PLQ workers and splitting factor' font "Helvetica,12" offset 0,-0.5,0
set ylabel "Thread level and splitting" font "Helvetica,11" offset 3.5,0,0
set yrange [0:8]
set ytics 2 font "Helvetica,10" offset 0.7,0,0
val(x,y) = x > 56 && x < 70 ? 2*y : 1.5*y
plot "results_real_r0.8/skyline_controller_Game1.log" using ($1*2.5):($5) with boxes ls 1 title "thread level", "" using ($1*2.5):(val($1, $3) > $5 ? $5 : val($1, $3)) with boxes ls 2 title "splitting"

# Line styles
LINEWIDTH = 'lw 2.5'
set style line 1 @LINEWIDTH lt 1 lc rgb "red" pointtype 5 pointsize 0.5

# Subplot 3-1
set size ratio 0.9
# Set horizontal margins for first column
set lmargin at screen left3
set rmargin at screen right3
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'WLQ workers' font "Helvetica,12" offset 0,-0.5,0
set ylabel "Thread level" font "Helvetica,11" offset 4.5,0,0
set yrange [0:35]
set ytics 7 font "Helvetica,10" offset 0.7,0,0
plot "results_real_r0.9/skyline_controller_Game1.log" using ($1*2.5):6 with linespoints ls 1 notitle
unset multiplot

system("sh crop.sh")