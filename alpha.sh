#!/bin/bash
rm -rf alpla.0.dat
rm -rf alpla.1.dat
rm -rf alpla.2.dat
rm -rf alpla.5.dat
cat $1 | grep "node 0" > alpha.0.dat
cat $1 | grep "node 1" > alpha.1.dat
cat $1 | grep "node 2" > alpha.2.dat
cat $1 | grep "node 5" > alpha.5.dat

gnuplot alpha.plt
