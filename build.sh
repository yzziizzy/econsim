#!/bin/bash


gcc \
	-o econsim \
	-ggdb -O0 \
	-lncurses \
	-Werror=int-conversion -Werror=implicit-function-declaration \
	-Werror=incompatible-pointer-types \
	main.c ds.c econ.c mempool.c

