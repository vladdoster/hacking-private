# This file should be sourced.
# It tests performance of dequoting, (Q) vs eval "s=$quoted"

INPUT='ice as"program" pick"$ZPFX/bin/prll_(qer|bfr)" src"prll.sh" make"install PREFIX=$ZPFX"'
INPUT="${(q)INPUT}"

# The same output
eval "str=$INPUT"
print -r -- "$str"
print -r -- "${(Q)INPUT}"

Qflag() {
    local OUTPUT
    repeat 50000; do
        OUTPUT="${(Q)INPUT}"
    done
}

evl() {
    local OUTPUT
    repeat 50000; do
        eval "OUTPUT=$INPUT"
    done
}

typeset -F2 SECONDS=0
Qflag
print "(Q)-flag result: $SECONDS"

SECONDS=0
evl
print "eval result: $SECONDS"
