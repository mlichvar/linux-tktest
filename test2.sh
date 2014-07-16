#!/bin/bash

for nohz in 1 0; do
  if [ $nohz -eq 1 ]; then
    nohz_options="-u 500"
    printf "%-20s" "nohz on"
  else
    nohz_options="-H -R -u 1"
    printf "%-20s" "nohz off"
  fi

  printf "%13s%35s\n" "[1, samples/2]" "[samples/2 + 1, samples]"
  printf "%-10s%10s%10s%10s%10s%10s%10s\n" samples freq dev max freq dev max

  for samples in 10 30 100 300 1000 3000 10000 30000 100000; do
    for sample_options in "-n $[$samples/2]" "-i $[$samples / 2] -n $samples"; do
      for freq_offset in 1000{0,1,2,3,4,5,6,7,8,9}{0,1,2,3,4,5,6,7,8,9}; do
	./tk_test -o $freq_offset $nohz_options $sample_options
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
    done | (read a b c; read d e f; printf "%-10d%10.5f%10.1f%10.1f%10.5f%10.1f%10.1f\n" $samples $c $a $b $f $d $e)
  done
  echo
done
