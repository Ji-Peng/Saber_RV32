#!/bin/bash
while [[ "$1" != "" ]]
do
    case "$1"
    in
    --elf)      elf="$2";   shift 2;;
    --hex)      hex="$2";   shift 2;;
    --jlink)    jlink="$2"; shift 2;;
    --gdb)      gdb="$2";   shift 2;;
    *)          echo "$0 Unkown argument $1"; exit 1;;
    esac
done

if [ "$elf" == "" -a "$hex" == "" ]
then
    echo "$0: --elf or --hex is required" >&2
    exit 1
fi

if [ "$jlink" == "" -a "$gdb" == "" ]
then
    echo "$0: --jlink or --gdb is required" >&2
    exit 1
fi

export GDB_PORT=3333

if [ "$gdb" == "" ]
then

echo -e "loadfile $hex\nrnh\nexit" | $jlink -device FE310 -if JTAG -speed 4000 -jtagconf -1,-1 -autoconnect 1

else

if [ "$jlink" == "" ]; then echo "$0: --jlink is required" >&2; fi
if [ "$elf" == "" ]; then echo "$0: --elf is required" >&2; fi
$jlink -device FE310 -if JTAG -speed 4000 -port ${GDB_PORT} &

$gdb $elf -ex "target extended-remote localhost:${GDB_PORT}" -ex "load" -ex "set disassemble-next-line on" -ex "source gdb_script" -ex "set print elements 0"

kill %1

fi
