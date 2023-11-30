#!/bin/bash

domain=(adr eadr)
binary=(clobber undo pmdk volt)
repeats=10
# arg="10 100000 1 10 0"
nthreads=(2 4 8 16)

# mkdir -p agr_tpcc
# (cd agr_tpcc && ../../../../script/compile_and_execute.sh TPCC 39_fence)

for nthread in "${nthreads[@]}"; do
	for i in $(seq $repeats); do
		for d in "${domain[@]}"; do
			for b in "${binary[@]}"; do
				sudo rm /mnt/ramdisk/*
				# N_WAREHOUSE N_ITEMS N_THREADS DURATION NOPS
				# If DURATION is 0, NOPS is used, otherwise run for DURATION and ignore NOPS.
				if [[ $d == "eadr" ]]; then
					sudo PMEM_NO_FLUSH=1 ./tpcc."$d"."$b" 10 100000 "$nthread" 10 0
				elif [[ $d == "adr" ]]; then
					sudo PMEM_NO_FLUSH=0 ./tpcc."$d"."$b" 10 100000 "$nthread" 10 0
				fi
			done
		done

		# sudo ./agr_tpcc/TPCC_39_fence $arg

		yj-noti "[$0] $i/$repeats"
	done
done

# sudo rm /mnt/ramdisk/*
# sudo LD_LIBRARY_PATH=/usr/local/lib/pmdk_debug PMEM_NO_FLUSH=0 PMEMOBJ_LOG_LEVEL=40  ./tpcc.adr.pmdk $arg 2>pmemobj.flush.log
# sudo rm /mnt/ramdisk/*
# sudo LD_LIBRARY_PATH=/usr/local/lib/pmdk_debug PMEM_NO_FLUSH=1 PMEMOBJ_LOG_LEVEL=40  ./tpcc.eadr.pmdk $arg 2>pmemobj.noflush.log
