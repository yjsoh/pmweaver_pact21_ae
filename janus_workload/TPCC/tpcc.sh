#!/bin/bash

domain=(adr eadr)
binary=(clobber undo)
repeats=10
arg="10 1000000 1 10 0"

# mkdir -p agr_tpcc
# (cd agr_tpcc && ../../../../script/compile_and_execute.sh TPCC 39_fence)

for i in $(seq $repeats); do
	for d in "${domain[@]}"; do
		for b in "${binary[@]}"; do
			# N_WAREHOUSE N_ITEMS N_THREADS DURATION NOPS
			# If DURATION is 0, NOPS is used, otherwise run for DURATION and ignore NOPS.
			sudo ./tpcc."$d"."$b" $arg
		done
	done

	sudo ./agr_tpcc/TPCC_39_fence $arg

	yj-noti "$i/$repeats"
done