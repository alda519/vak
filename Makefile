CPPFLAGS=-lsimlib

.PHONY: run plot replot

projekt: projekt.cpp

ims-vak: ims-vak.cpp

vak: vak.cpp

run: projekt
	./projekt > projekt.dat

plot: projekt
	gnuplot -persist plot

replot: projekt run plot
