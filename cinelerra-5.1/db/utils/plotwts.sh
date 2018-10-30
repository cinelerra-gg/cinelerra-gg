#!/bin/bash
dir=/tmp/data
n1=`expr "00000$1" : '.*\(.....\)'`
n2=`expr "00000$2" : '.*\(.....\)'`
cmd="plot \"$dir/w$n1.txt\" with lines, \"$dir/w$n2.txt\" with lines"
gnuplot -e "set terminal png; $cmd" | xv - &
