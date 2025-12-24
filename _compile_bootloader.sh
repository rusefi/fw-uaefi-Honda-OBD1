#!/usr/bin/env bash


cd ext/rusefi/firmware/
bash bin/compile.sh ../../../meta-info.env bootloader

cd /../../..

source ext/rusefi/firmware/config/boards/common_script_read_meta_env.inc meta-info.env
