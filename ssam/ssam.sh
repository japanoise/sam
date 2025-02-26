#!/bin/sh
arg0=$(basename "$0")

usage()
{
    printf "usage: %s [-n] [-e script] [-f sfile] [file ...]\n" "$1"
}

flagn=0
flage=""
flagf=""

while getopts ne:f:h OPT; do
    case "$OPT" in
        n)
            flagn=1
            ;;

        e)
            if [ -z "$OPTARG" ]; then
                usage "$arg0"
                exit 1
            fi
            flage="$OPTARG"
            ;;
        f)
            if [ -z "$OPTARG" ]; then
                usage "$arg0"
                exit 1
            fi
            flagf="$OPTARG"
            ;;
        h)
            usage "$arg0"
            exit 0
            ;;
        *)
            usage "$arg0"
            exit 1
            ;;
    esac
done
shift "$((OPTIND - 1))"

if [ -z "$TMPDIR" ]; then
    TMPDIR="/tmp"
fi
tmp="$TMPDIR/ssam.tmp.$USER.$$"

# Variable is referenced in non-expanded string, it's OK that it's not
# set by the parent script here.
# shellcheck disable=SC2154
trap 'result=$?; rm -f "$tmp"; exit $result' INT EXIT
cat "$@" >"$tmp"

{
    # select entire file
    echo ',{'
    echo k
    echo '}'
    echo 0k

    # run scripts, print
    [ -n "$flagf" ] && cat "$flagf"
    [ -n "$flage" ] && echo "$flage"
    [ "$flagn" -eq 0 ] && echo ','
} | sam -d "$tmp" 2>/dev/null

exit $?
