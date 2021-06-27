#!/bin/bash

path=$(dirname $0)

${path}/limnemu \
	-rom ${path}/bin/boot.bin \
	-nvram ${path}/bin/nvram \
	"$@"