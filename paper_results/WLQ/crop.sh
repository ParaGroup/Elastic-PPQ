for f in *.eps; do ps2pdf $f; done
rm *.eps
for f in *.pdf; do pdfcrop $f $f; done
cd ..
