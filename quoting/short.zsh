# This file should be sourced.
# It tests performance of quoting, (q) vs printf '%q '.

INPUT='ice as"program" pick"$ZPFX/bin/prll_(qer|bfr)" src"prll.sh" make"install PREFIX=$ZPFX"'

# The same output
printf '%q ' "$INPUT"
print
print -r -- "${(q)INPUT}"

qflag() {
    local OUTPUT
    repeat 10000; do
        OUTPUT="${(q)INPUT}"
    done
}

prtf() {
    local OUTPUT
    repeat 10000; do
        printf -v OUTPUT '%q ' "$INPUT"
    done
}

typeset -F3 SECONDS=0
qflag
print "(q)-flag result: $SECONDS"

SECONDS=0
prtf
print "printf result: $SECONDS"
