# Script to generate a multiplot of the splitting behavior (Case with MAP I=1K)
mpl_top    = 0.4 # inch  outer top margin, title goes here
mpl_bot    = 0.7 # inch  outer bottom margin, x label goes here
mpl_left   = 0.9 # inch  outer left margin, y label goes here
mpl_right  = 0.1 # inch  outer right margin, y2 label goes here
mpl_height = 1.2 # inch  height of individual plots
mpl_width  = 1.7 # inch  width of individual plots
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
set terminal pdfcairo enhanced size xsize,ysize
set encoding iso_8859_1
set tics scale 1.5
set output 'split.pdf'
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
LINEWIDTH = 'lw 1.5'
set style line 1 @LINEWIDTH lt 1 lc rgb "blue" pointtype 5 pointsize 0.4
set style line 2 @LINEWIDTH lt 1 lc rgb "#E2601F" pointtype 7 pointsize 0.4
set style line 3 @LINEWIDTH lt 1 lc rgb "black" pointtype 3 pointsize 0.4
set style line 4 @LINEWIDTH lt 1 lc rgb "red"
set style fill transparent solid 0.25 noborder

# Define x-axis settings for all subplots
set xrange[1:12.5]
set xtics 1 font "Helvetica,9" offset 0,0.7,0
set xlabel "Thread level and splitting" font "Helvetica,10" offset 0,1.3,0
set mxtics 1

# Subplot 1-1
# Set horizontal margins for first column
set lmargin at screen left(1)
set rmargin at screen right(1)
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set title 'Even splitting case' font "Helvetica,11" offset 0,-0.5,0
set ylabel "Filtering ratio (%)" font "Helvetica,10" offset 3.2,0,0
set yrange[0:80]
set ytics 20 nomirror font "Helvetica,9" offset 0.7,0,0
set y2range[0.2:16.2]
set logscale y2
set y2label "Utiliz. factor-logscale" font "Helvetica,10" offset -5.2,0,0
set y2tics (0.2, 0.6, 1.8, 5.4, 16.2) nomirror font "Helvetica,9" offset -1,0,0
plot "data_tb_i1K.txt" using 1:((100*$2)/8000) with filledcurves x1 ls 3 notitle, "" using 1:((100*$2)/8000) with linespoints ls 3 title "filtering ratio", "" using 1:3 with filledcurves x1 ls 1 axes x1y2 notitle, "" using 1:3 with linespoints ls 1 axes x1y2 title "utiliz. factor", 0.9 with lines ls 4 axes x1y2 title "setpoint (0.9)"

# Subplot 2-1
# Set horizontal margins for second column
set lmargin at screen left(2)
set rmargin at screen right(2)
# Set horizontal margins for third row (top)
set tmargin at screen top(1)
set bmargin at screen bot(1)
set xlabel "Thread level" font "Helvetica,10" offset 0,1.3,0
set title 'PID splitting case' font "Helvetica,11" offset 0,-0.5,0
set ylabel "Splitting factor" font "Helvetica,10" offset 3.8,0,0
set yrange[0:10]
set ytics 2.5 nomirror font "Helvetica,9" offset 0.7,0,0
set y2range[0.2:16.2]
set logscale y2
set y2label "Utiliz. factor-logscale" font "Helvetica,10" offset -5.2,0,0
set y2tics (0.2, 0.6, 1.8, 5.4, 16.2) nomirror font "Helvetica,9" offset -1,0,0
plot "data_aspid_i1K.txt" using 1:3 with filledcurves x1 ls 2 notitle, "" using 1:3 with linespoints ls 2 title "splitting", "" using 1:4 with filledcurves x1 ls 1 axes x1y2 notitle, "" using 1:4 with linespoints ls 1 axes x1y2 title "utiliz. factor", 0.9 with lines ls 4 axes x1y2 title "setpoint (0.9)"
unset multiplot

system("pdfcrop split.pdf split.pdf")