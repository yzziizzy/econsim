#!/bin/bash


gcc \
	-o econsim \
	-ggdb -O0 \
	-lncurses -ltinfo -lm \
	-DSTI_C3DLAS_NO_CONFLICT \
	-Wall -Werror=all \
	-Wno-unused-function \
	-Wno-unused-label \
	-Wno-switch \
	-Wno-unused-but-set-variable \
	-Wno-unused-variable \
	-Wno-discarded-qualifiers \
	-Werror=int-conversion -Werror=implicit-function-declaration \
	-Werror=incompatible-pointer-types \
	sti/sti.c c_json/json.c \
	main.c econ.c 
	

