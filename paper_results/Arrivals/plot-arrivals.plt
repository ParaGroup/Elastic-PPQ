# Script to generate a multiplot with the arrivals with various distributions and time-scales
mpl_top    = 0.4 # inch  outer top margin, title goes here
mpl_bot    = 0.7 # inch  outer bottom margin, x label goes here
mpl_left   = 0.9 # inch  outer left margin, y label goes here
mpl_right  = 0.1 # inch  outer right margin, y2 label goes here
mpl_height = 1.2 # inch  height of individual plots
mpl_width  = 2.0 # inch  width of individual plots
mpl_dx     = 0.35 # inch  inter-plot horizontal spacing
mpl_dy     = 0.45 # inch  inter-plot vertical spacing
mpl_ny     = 3   # number of rows
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
set output 'arrivals.eps'
set offsets
set autoscale fix
set size 1,1
set nokey

# Start plotting
set multiplot

# Grid style
set grid back lw 1 lt 0 lc rgb '#808080'

# Line styles
LINEWIDTH = 'lw 4.5'
set style line 1 @LINEWIDTH lt 2 lc rgb "black"
set style line 2 @LINEWIDTH lt 1  lc rgb "red"
set style fill solid 0.6

# Define x-axis settings for all subplots
set xrange[0:800]
#set format x ''
set xtics 200 font "Helvetica,12" offset 0,0.5,0
set mxtics 5

# Subplot 1-3
# Set horizontal margins for first column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for third row (top)
set tmargin at screen top(3)
set bmargin at screen bot(3)
set title 'Poisson traffic' font "Helvetica,16" offset 0,-0.5,0
set xlabel "Time unit = 100 ms" font "Helvetica,14" offset 0,1.5,0
set ylabel "Arrivals" font "Helvetica,14" offset 4.5,0,0
set yrange[0:3200]
set ytics ("0" 0, "0.8K" 800, "1.6K" 1600, "2.4K" 2400, "3.2K" 3200) font "Helvetica,12" offset 0.6,0,0
plot "arrivals_poisson_100ms.txt" with boxes ls 1 notitle

# Subplot 2-3
# Set horizontal margins for second column
set lmargin at screen left(2)
set rmargin at screen right(2)
# Set horizontal margins for third row (top)
set tmargin at screen top(3)
set bmargin at screen bot(3)
set title 'Poisson traffic' font "Helvetica,16" offset 0,-0.5,0
set xlabel "Time unit = 1 sec" font "Helvetica,14" offset 0,1.5,0
set ylabel "Arrivals" font "Helvetica,14" offset 3.7,0,0
set yrange[0:32000]
set ytics ("0" 0, "8K" 8000, "16K" 16000, "24K" 24000, "32K" 32000) font "Helvetica,12" offset 0.6,0,0
plot "arrivals_poisson_1s.txt" with boxes ls 1 notitle, "< head -80 arrivals_poisson_1s.txt" using 1:2 with boxes ls 2 notitle

# Subplot 1-2
# Set horizontal margins for first column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for second row (middle)
set tmargin at screen top(2)
set bmargin at screen bot(2)
set title 'Markov Modulated traffic' font "Helvetica,16" offset 0,-0.5,0
set xlabel "Time unit = 100 ms" font "Helvetica,14" offset 0,1.5,0
set ylabel "Arrivals" font "Helvetica,14" offset 4.5,0,0
set yrange[0:3200]
set ytics ("0" 0, "0.8K" 800, "1.6K" 1600, "2.4K" 2400, "3.2K" 3200) font "Helvetica,12" offset 0.6,0,0
plot "arrivals_map_100ms.txt" with boxes ls 1 notitle

# Subplot 2-2
# Set horizontal margins for second column
set lmargin at screen left(2)
set rmargin at screen right(2)
# Set horizontal margins for second row (middle)
set tmargin at screen top(2)
set bmargin at screen bot(2)
set title 'Markov Modulated traffic' font "Helvetica,16" offset 0,-0.5,0
set xlabel "Time unit = 1 sec" font "Helvetica,14" offset 0,1.5,0
set ylabel "Arrivals" font "Helvetica,14" offset 3.7,0,0
set yrange[0:32000]
set ytics ("0" 0, "8K" 8000, "16K" 16000, "24K" 24000, "32K" 32000) font "Helvetica,12" offset 0.6,0,0
plot "arrivals_map_1s.txt" with boxes ls 1 notitle, "< head -80 arrivals_map_1s.txt" using 1:2 with boxes ls 2 notitle

# Subplot 1-1
# Set horizontal margins for first column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for first row (bottom)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'Self-similar traffic' font "Helvetica,16" offset 0,-0.5,0
set xlabel "Time unit = 100 ms" font "Helvetica,14" offset 0,1.5,0
set ylabel "Arrivals" font "Helvetica,14" offset 4.5,0,0
set yrange[0:3200]
set ytics ("0" 0, "0.8K" 800, "1.6K" 1600, "2.4K" 2400, "3.2K" 3200) font "Helvetica,12" offset 0.6,0,0
plot "arrivals_pareto_100ms.txt" with boxes ls 1 notitle

# Subplot 2-1
# Set horizontal margins for second column
set lmargin at screen left(2)
set rmargin at screen right(2)
# Set horizontal margins for first row (bottom)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'Self-similar traffic' font "Helvetica,16" offset 0,-0.5,0
set xlabel "Time unit = 1 sec" font "Helvetica,14" offset 0,1.5,0
set ylabel "Arrivals" font "Helvetica,14" offset 3.7,0,0
set yrange[0:32000]
set ytics ("0" 0, "8K" 8000, "16K" 16000, "24K" 24000, "32K" 32000) font "Helvetica,12" offset 0.6,0,0
plot "arrivals_pareto_1s.txt" with boxes ls 1 notitle, "< head -80 arrivals_pareto_1s.txt" using 1:2 with boxes ls 2 notitle
unset multiplot

system("sh crop.sh")