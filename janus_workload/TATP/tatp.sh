#!/bin/bash

domain=(adr eadr)
binary=(clobber undo)
repeats=1

for i in $(seq $repeats); do
	for d in ${domain[@]}; do
		for b in ${binary[@]}; do
			# NSUBSCRIBER NOPS NTHREADS DURATION
			sudo ./tatp.$d.$b 1000000 0 1 10
		done
	done
done