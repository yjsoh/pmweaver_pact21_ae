#!/bin/bash

domain=(eadr)
binary=(volt)
repeats=10
# args="1000000 0 1 10"
nthreads=(2 4 8 16)
# mkdir -p agr_tatp
# (cd agr_tatp && ../../../../script/compile_and_execute.sh TATP 39_fence)

for nthread in "${nthreads[@]}"; do
	for i in $(seq $repeats); do
		for d in "${domain[@]}"; do
			for b in "${binary[@]}"; do
				sudo rm -f /mnt/ramdisk/*
				# NSUBSCRIBER NOPS NTHREADS DURATION
				if [[ $d == "eadr" ]]; then
					sudo PMEM_NO_FLUSH=1 ./tatp."$d"."$b" 1000000 0 "$nthread" 10
				elif [[ $d == "adr" ]]; then
					sudo PMEM_NO_FLUSH=0 ./tatp."$d"."$b" 1000000 0 "$nthread" 10
				fi
			done
		done

		yj-noti "[$0] $i/$repeats $(wc -l tatp.csv)"
	done
done

yj-noti "[$0] Done"
