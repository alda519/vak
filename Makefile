CPPFLAGS=-lsimlib

.PHONY: run plot replot

projekt: projekt.cpp


zip:
	tar czf 07_xcupak04_xdujic01.tar.gz experimenty projekt.cpp Makefile dokumentace/dokumentace.pdf plot


run: projekt
	./projekt > projekt.dat

plot: projekt
	gnuplot -persist plot

replot: projekt run plot
