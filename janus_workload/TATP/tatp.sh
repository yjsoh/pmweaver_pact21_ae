#!/bin/bash

domain=(adr eadr)
binary=(clobber undo pmdk)
repeats=10

# mkdir -p agr_tatp
# (cd agr_tatp && ../../../../script/compile_and_execute.sh TATP 39_fence)

for i in $(seq $repeats); do
	for d in "${domain[@]}"; do
		for b in "${binary[@]}"; do
			# NSUBSCRIBER NOPS NTHREADS DURATION
			if [[ $d == "eadr" ]]; then
				sudo PMEM_NO_FLUSH=1 ./tatp.$d.$b 1000000 0 1 10
			elif [[ $d == "adr" ]]; then
				sudo PMEM_NO_FLUSH=0 ./tatp.$d.$b 1000000 0 1 10
			fi
		done
	done

	# sudo ./agr_tatp/TATP_39_fence 1000000 0 1 10
	yj-noti "$i/$repeats"
done
