# Script to generate a multiplot of the WLQ analysis (idle time of the WLQ workers)
mpl_top    = 0.4 # inch  outer top margin, title goes here
mpl_bot    = 0.7 # inch  outer bottom margin, x label goes here
mpl_left   = 0.9 # inch  outer left margin, y label goes here
mpl_right  = 0.1 # inch  outer right margin, y2 label goes here
mpl_height = 0.9 # inch  height of individual plots
mpl_width  = 1.7 # inch  width of individual plots
mpl_dx     = 0.25 # inch  inter-plot horizontal spacing
mpl_dy     = 0.15 # inch  inter-plot vertical spacing
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
set output 'idle.eps'
set offsets
set autoscale fix
set size ratio 0.45

# Start plotting
set multiplot

# Grid style
set grid back lw 1 lt 0 lc rgb '#808080'

# Key style
set key horizontal spacing 1 samplen 1 width -2 font "Helvetica,12"
set key vertical maxcolumns 1

# Line styles
LINEWIDTH = 'lw 1'
set style line 1 @LINEWIDTH lt 1 lc rgb "white"
set style line 2 @LINEWIDTH lt 1 lc rgb "black"

# Plot style
set style data histogram
set style histogram rowstacked gap 20
set style fill solid border 0
everytot(col) = (int(column(col))%20 ==0)?stringcolumn(1):""

# Define x-axis settings for all subplots
set xtics font "Helvetica,10" offset 0,0.7,0
set xlabel "Time (sec)" font "Helvetica,11" offset 0,1.8,0

# Subplot 1-1
# Set horizontal margins for first column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'Idle time (fb-default)' font "Helvetica,12" offset 0,-0.5,0
set ylabel "Idle time (%)" font "Helvetica,11" offset 5.5,0,0
set yrange[0:100]
set ytics 20 nomirror font "Helvetica,10" offset 0.7,0,0
plot "wlq_old/controller.log" using ($8*100):xticlabels(everytot(1)) notitle, "" using ($11*100):xticlabels(everytot(1)) ls 1 notitle

# Subplot 2-1
# Set horizontal margins for second column
set lmargin at screen left(2)
set rmargin at screen right(2)
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'Idle time (fb-opt)' font "Helvetica,12" offset 0,-0.5,0
set ylabel "Idle time (%)" font "Helvetica,11" offset 5.5,0,0
set yrange[0:100]
set ytics 20 nomirror font "Helvetica,10" offset 0.7,0,0
plot "wlq_new/controller.log" using ($8*100):xticlabels(everytot(1)) notitle, "" using ($11*100):xticlabels(everytot(1)) ls 1 notitle
unset multiplot

system("sh crop.sh")