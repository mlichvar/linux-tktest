#!/bin/bash

for nohz in "" "-H -R -u 1"; do
  for sample_options in "-n 10" "-n 100" "-i 5000 -n 10000"; do
    for freq_offset in 1000{0,1,2,3,4,5,6,7,8,9}{0,1,2,3,4,5,6,7,8,9}; do
      ./tk_test -o $freq_offset $nohz $sample_options
    done | awk '
      BEGIN {
	n = 0; sum1 = 0; sum2 = 0; sum3 = 0
      } /samples/ {
	sum1 += $8;
	sum2 += $10;
	diff = ($12 - 100 - n / 1000);
	sum3 += diff * diff;
	n++;
      } END {
	printf "%.1f %.1f %.5f\n", sum1 / n, sum2 / n, sqrt(sum3 / n);
      }'
  done | (read a b c; read d e f; read g h i; echo -e "$c\t\t$f\t\t$g\t\t$h")
done
