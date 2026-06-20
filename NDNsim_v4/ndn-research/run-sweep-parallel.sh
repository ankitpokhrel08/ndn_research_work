#!/bin/bash
# run-sweep-parallel.sh — Detection-floor sweep, parallelized
# Usage: bash run-sweep-parallel.sh [topology] [jobs]
#   bash run-sweep-parallel.sh              # all topologies, auto job count
#   bash run-sweep-parallel.sh tree         # tree only, auto jobs
#   bash run-sweep-parallel.sh tree 6       # tree only, 6 parallel jobs
#   bash run-sweep-parallel.sh "" 8         # all topologies, 8 jobs

TOPOS="${1:-tree dfn dumbbell}"
[ -z "$TOPOS" ] && TOPOS="tree dfn dumbbell"

# Default job count = number of cores (leave 1 free), min 2
NCORES=$(nproc 2>/dev/null || echo 4)
JOBS="${2:-$((NCORES > 1 ? NCORES - 1 : 1))}"

ATTACKS="ifa cp"
RATES="100 50 30 20 15 12 10"
SEEDS="1 2 3 4 5"

cd /workspace/ndn-research

echo "=== Building ==="
./waf || { echo "Build failed"; exit 1; }

echo "=== Parallel jobs: $JOBS | Topologies: $TOPOS ==="

# Build a job list (one command per line), then feed to xargs -P
JOBFILE=$(mktemp)

for topo in $TOPOS; do
  # normal baselines
  for seed in $SEEDS; do
    outbase="results/${topo}-normal-run${seed}"
    if [ ! -s "${outbase}-rate-trace.txt" ]; then
      echo "./build/${topo}-normal --rngRun=${seed}" >> "$JOBFILE"
    fi
  done
  # attack sweeps
  for attack in $ATTACKS; do
    for rate in $RATES; do
      for seed in $SEEDS; do
        outbase="results/${topo}-${attack}-r${rate}-run${seed}"
        if [ ! -s "${outbase}-rate-trace.txt" ]; then
          echo "./build/${topo}-${attack} --attackRate=${rate} --rngRun=${seed}" >> "$JOBFILE"
        fi
      done
    done
  done
done

REMAINING=$(wc -l < "$JOBFILE")
echo "=== Jobs to run (skipping existing): $REMAINING ==="

if [ "$REMAINING" -eq 0 ]; then
  echo "Nothing to do — all results already present."
  rm -f "$JOBFILE"
else
  # Run jobs in parallel. Each line is a full command.
  # --line-buffered-ish progress via a counter file.
  cat "$JOBFILE" | xargs -P "$JOBS" -I {} bash -c '
    cmd="{}"
    echo "[START] $cmd"
    eval "$cmd" > /dev/null 2>&1 && echo "[DONE ] $cmd" || echo "[FAIL ] $cmd"
  '
  rm -f "$JOBFILE"
fi

echo ""
echo "=== Sweep complete ==="
echo ""

# Sanity checks
echo "--- CsTracer not all-zero ---"
for topo in $TOPOS; do
  f="results/${topo}-normal-run1-cs-trace.txt"
  if [ -f "$f" ]; then
    nonzero=$(awk -F'\t' '$3=="CacheHits" && $4>0' "$f" | head -1)
    [ -n "$nonzero" ] && echo "  [OK] $topo normal nonzero CacheHits" || echo "  [WARN] $topo normal CacheHits all zero!"
  fi
done

echo ""
echo "--- Two seeds differ ---"
for topo in $TOPOS; do
  f1="results/${topo}-ifa-r100-run1-rate-trace.txt"
  f2="results/${topo}-ifa-r100-run2-rate-trace.txt"
  if [ -f "$f1" ] && [ -f "$f2" ]; then
    if diff -q "$f1" "$f2" > /dev/null 2>&1; then
      echo "  [WARN] $topo IFA r100 run1/run2 IDENTICAL"
    else
      echo "  [OK] $topo IFA r100 run1/run2 differ"
    fi
  fi
done

echo ""
echo "--- Legit traffic alive at t=400 (background not dead) ---"
for topo in $TOPOS; do
  f="results/${topo}-normal-run1-rate-trace.txt"
  if [ -f "$f" ]; then
    alive=$(awk -F'\t' '$1==400 && $5=="OutInterests" && $6>0' "$f" | head -1)
    [ -n "$alive" ] && echo "  [OK] $topo has live traffic at t=400" || echo "  [WARN] $topo traffic DEAD at t=400!"
  fi
done

echo ""
echo "--- File count ---"
echo "Rate traces:   $(ls results/*-rate-trace.txt 2>/dev/null | wc -l)"
echo "Ground truths: $(ls results/*-ground-truth.csv 2>/dev/null | wc -l)"