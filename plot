set terminal wxt size 1300, 900

set xlabel 'cas'
set ylabel 'dokumenty a reditel'

p "projekt.dat"  using 1:4 w l title 'reditel v praci', \
  "projekt.dat" using 1:2 w l title 'sekretarka', \
  "projekt.dat" using 1:3 w l title 'reditel', \
  "projekt.dat" using 1:6 w l title 'dny'

#  "projekt.dat" using 1:5 w l title 'technicky namestek', \

replot
