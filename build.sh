#!/bin/bash


gcc \
	-o econsim \
	-ggdb -O0 \
	-Wint-conversion -Wimplicit-function-declaration \
	main.c ds.c econ.c mempool.c

