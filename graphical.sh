#!/bin/bash

path=$(dirname $0)

${path}/xremu \
	-rom ${path}/bin/boot.bin \
	-nvram ${path}/bin/nvram \
	"$@"