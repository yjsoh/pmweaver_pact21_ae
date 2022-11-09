#!/bin/bash

domain=(adr eadr)
binary=(clobber undo pmdk)
repeats=10
args="1000000 0 1 10"
# mkdir -p agr_tatp
# (cd agr_tatp && ../../../../script/compile_and_execute.sh TATP 39_fence)

for i in $(seq $repeats); do
	for d in "${domain[@]}"; do
		for b in "${binary[@]}"; do
			sudo rm -f /mnt/ramdisk/*
			# NSUBSCRIBER NOPS NTHREADS DURATION
			if [[ $d == "eadr" ]]; then
				sudo PMEM_NO_FLUSH=1 ./tatp.$d.$b $args
			elif [[ $d == "adr" ]]; then
				sudo PMEM_NO_FLUSH=0 ./tatp.$d.$b $args
			fi
		done
	done

	yj-noti "[$0] $i/$repeats $(wc -l tatp.csv)"
done

yj-noti "[$0] Done"
