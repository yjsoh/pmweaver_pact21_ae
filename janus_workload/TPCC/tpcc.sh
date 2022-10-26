#!/bin/bash

domain=(adr eadr)
binary=(clobber undo)
repeats=1

for i in $(seq $repeats); do
	for d in ${domain[@]}; do
		for b in ${binary[@]}; do
			# N_WAREHOUSE N_ITEMS N_THREADS DURATION NOPS
			# If DURATION is 0, NOPS is used, otherwise run for DURATION and ignore NOPS.
			sudo ./tpcc.$d.$b 10 1000000 1 10 0
		done
	done
done