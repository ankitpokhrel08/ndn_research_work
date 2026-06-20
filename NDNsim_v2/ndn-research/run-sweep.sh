#!/bin/bash
# run-sweep.sh — Detection-floor sweep
# Usage: bash run-sweep.sh [topology]
# e.g.:  bash run-sweep.sh tree       (just tree)
#        bash run-sweep.sh             (all three)

set -e

TOPOS="${1:-tree dfn dumbbell}"
ATTACKS="ifa cp"
RATES="100 50 30 20 15 12 10"
SEEDS="1 2 3 4 5"

cd /workspace/ndn-research

echo "=== Building ==="
./waf

total=0
done_count=0

# Count total runs
for topo in $TOPOS; do
  for seed in $SEEDS; do
    total=$((total + 1))  # normal
  done
  for attack in $ATTACKS; do
    for rate in $RATES; do
      for seed in $SEEDS; do
        total=$((total + 1))
      done
    done
  done
done

echo "=== Total runs: $total ==="

# Normal baselines (5 seeds each)
for topo in $TOPOS; do
  for seed in $SEEDS; do
    outbase="results/${topo}-normal-run${seed}"
    if [ -s "${outbase}-rate-trace.txt" ]; then
      echo "[SKIP] ${topo}-normal run${seed} (already exists)"
    else
      echo "[RUN]  ${topo}-normal run${seed}"
      ./build/${topo}-normal --rngRun=$seed
    fi
    done_count=$((done_count + 1))
    echo "       Progress: ${done_count}/${total}"
  done
done

# Attack sweeps
for topo in $TOPOS; do
  for attack in $ATTACKS; do
    for rate in $RATES; do
      for seed in $SEEDS; do
        outbase="results/${topo}-${attack}-r${rate}-run${seed}"
        if [ -s "${outbase}-rate-trace.txt" ]; then
          echo "[SKIP] ${topo}-${attack} r=${rate} run${seed} (already exists)"
        else
          echo "[RUN]  ${topo}-${attack} r=${rate} run${seed}"
          ./build/${topo}-${attack} --attackRate=$rate --rngRun=$seed
        fi
        done_count=$((done_count + 1))
        echo "       Progress: ${done_count}/${total}"
      done
    done
  done
done

echo ""
echo "=== Sweep complete ==="
echo ""

# Sanity checks
echo "--- Sanity check: CsTracer not all-zero ---"
for topo in $TOPOS; do
  f="results/${topo}-normal-run1-cs-trace.txt"
  if [ -f "$f" ]; then
    nonzero=$(awk -F'\t' '$3=="CacheHits" && $4>0' "$f" | head -1)
    if [ -n "$nonzero" ]; then
      echo "  [OK] $topo normal has nonzero CacheHits"
    else
      echo "  [WARN] $topo normal CacheHits all zero!"
    fi
  fi
done

echo ""
echo "--- Sanity check: two seeds differ ---"
for topo in $TOPOS; do
  f1="results/${topo}-ifa-r100-run1-rate-trace.txt"
  f2="results/${topo}-ifa-r100-run2-rate-trace.txt"
  if [ -f "$f1" ] && [ -f "$f2" ]; then
    if diff -q "$f1" "$f2" > /dev/null 2>&1; then
      echo "  [WARN] $topo IFA r100 run1 and run2 are IDENTICAL — seed fix may not have taken"
    else
      echo "  [OK] $topo IFA r100 run1 and run2 differ"
    fi
  fi
done

echo ""
echo "--- File count ---"
echo "Rate traces: $(ls results/*-rate-trace.txt 2>/dev/null | wc -l)"
echo "Ground truths: $(ls results/*-ground-truth.csv 2>/dev/null | wc -l)"