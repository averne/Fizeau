#!/bin/bash
set -e

docker run --rm --name devkitpro-fizeau -v .:/mnt/ devkitpro/devkita64:20260215 sh -c "git config --global --add safe.directory /mnt && cd /mnt/ && make -C common -j\$(nproc) && make -j\$(nproc) dist"
