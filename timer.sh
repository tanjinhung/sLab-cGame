#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<EOF >&2
Usage: $0 [-n N] program [args...]
  -n N    number of runs (default 10)
EOF
  exit 1
}

# default runs
N=10

# if first arg is -n, grab the count
if [[ $# -ge 2 && $1 == -n ]]; then
  N=$2
  shift 2
fi

# must have at least a program to run
if [[ $# -lt 1 ]]; then
  usage
fi

sum_ns=0

for i in $(seq 1 $N); do
  t0=$(date +%s%N)    # nanosecond timestamp
  "$@"                # run your program with all remaining args
  t1=$(date +%s%N)

  dt_ns=$((t1 - t0))
  dt_ms=$((dt_ns / 1000000))
  rem_ns=$((dt_ns % 1000000))

  printf "Run %2d: %d.%06d ms\n" "$i" "$dt_ms" "$rem_ns"
  sum_ns=$((sum_ns + dt_ns))
done

# compute and print average
avg_ns=$((sum_ns / N))
avg_ms=$((avg_ns / 1000000))
avg_rem=$((avg_ns % 1000000))

printf "Average over %d runs: %d.%06d ms\n" \
       "$N" "$avg_ms" "$avg_rem"
