#!/bin/bash

twib sd push sysmodule/out/Fizeau.nsp /atmosphere/contents/0100000000000F12/exefs.nsp
if [ $? -ne 0 ]; then
    echo Failed to copy file
    exit
fi

twib reboot

while [ $(twib list-devices | wc -l) -gt 1 ]; do sleep 0.1; done

echo Waiting...
RET=1
while [ $RET -ne 0 ]; do
    PIPE=$(twib list-named-pipes 2>/dev/null)
    RET=$?
    sleep 0.1
done

echo Opening pipe $PIPE
twib open-named-pipe $PIPE
