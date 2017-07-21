# Script to generate a multiplot of the PLQ evaluation in the artificial case
mpl_top    = 0.4 # inch  outer top margin, title goes here
mpl_bot    = 0.2 # inch  outer bottom margin, x label goes here
mpl_left   = 0.9 # inch  outer left margin, y label goes here
mpl_right  = 0.1 # inch  outer right margin, y2 label goes here
mpl_height = 0.8 # inch  height of individual plots
mpl_width  = 2.0 # inch  width of individual plots
mpl_dx     = 0.1 # inch  inter-plot horizontal spacing
mpl_dy     = 0 # inch  inter-plot vertical spacing
mpl_ny     = 3   # number of rows
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
set output 'artificial.eps'
set offsets
set autoscale fix
set size ratio 0.35

# Start plotting
set multiplot

# Grid style
set grid back lw 1 lt 0 lc rgb '#808080'

# Key style
set key horizontal spacing 0.8 samplen 1 width -2 font "Helvetica,12"
set key vertical maxrows 1

# Line styles
LINEWIDTH1 = 'lw 4.5'
LINEWIDTH2 = 'lw 3'
set style line 1 @LINEWIDTH1 lt 1 lc rgb "black"
set style line 2 @LINEWIDTH1 lt 1 lc rgb "blue"
set style line 3 lw 0 lt 1 lc rgb "#FF5F20"
set style line 4 lw 0 lt 1 lc rgb "#5047FF"
set style line 5 lw 2 lt 1 lc rgb "red"
set style line 6 lw 0 lt 1 lc rgb "#4C8C42"
set style fill solid 0.8 border rgb "black"

# Define x-axis settings for all subplots
set xrange[0:180]
set xtics (0, 30, 60, 90, 120, 150, 180) font "Helvetica,10" offset 0,0.7,0
set mxtics 3

# Subplot 1-3
# Set horizontal margins for first column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for third row (top)
set tmargin at screen top(3)
set bmargin at screen bot(3)
set title 'Two-level adaptation: artificial case' font "Helvetica,12" offset 0,-0.5,0
set yrange[0:100000]
set ylabel "Tuples/sec" font "Helvetica,11" offset 4.8,0,0
set ytics ("0" 0, "20K" 20000, "40K" 40000, "60K" 60000, "80K" 80000, "100K" 100000) nomirror font "Helvetica,10" offset 0.6,0,0
set y2range[2:12]
set y2label "Splitting and no. threads" font "Helvetica,11" offset -4.5,0,0
set y2tics 2 nomirror font "Helvetica,10" offset -1,0,0
plot "arrivals_artificial.txt" with lines ls 1 title "arrivals", "controller.log" using ($0*2.5):3 with boxes ls 3 axes x1y2 title "splitting", "controller.log" using ($0*2.5):5 with lines ls 2 axes x1y2 title "thread level"

# Subplot 1-2
# Set horizontal margins for second column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for third row (top)
set tmargin at screen top(2)
set bmargin at screen bot(2)
set notitle
set yrange[0:2.5]
set ylabel "Utiliz. factor" font "Helvetica,11" offset 4.8,0,0
set ytics 0.5 nomirror font "Helvetica,10" offset 0.6,0,0
set y2label ""
unset y2tics
plot "controller.log" using ($0*2.5):2 with boxes ls 4 title "utiliz. factor", 0.9 with lines ls 5 title "setpoint (0.9)"

# Subplot 1-1
# Set horizontal margins for second column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set notitle
set yrange[0:25]
set ylabel "Panes/sec" font "Helvetica,11" offset 3.8,0,0
set ytics 5 nomirror font "Helvetica,10" offset 0.6,0,0
unset y2tics
set xlabel "Time (sec)" font "Helvetica, 11" offset 0,1.8,0
plot "client_plq.log" using 0:2 with boxes ls 6 title "throughput", 10 with lines ls 5 title "ideal (10)"
unset multiplot

system("sh crop.sh")