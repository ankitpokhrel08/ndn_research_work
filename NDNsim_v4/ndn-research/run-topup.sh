#!/bin/bash
# run-topup.sh — CP floor gap-fill + IFA low-rate probe
# No source changes. Only --attackRate and --rngRun vary.
# Usage: bash run-topup.sh          # main 120 runs
#        bash run-topup.sh backfill # also do r50/r100 seeds 2-5 (+24 runs)

cd /workspace/ndn-research

echo "=== Building (no-op if already built) ==="
./waf || { echo "Build failed"; exit 1; }

TOPOS="tree dfn dumbbell"
SEEDS="1 2 3 4 5"

# Count total
total=0
for topo in $TOPOS; do
  for rate in 35 40 45; do for seed in $SEEDS; do total=$((total+1)); done; done
  for rate in 1 2 4 6 8; do for seed in $SEEDS; do total=$((total+1)); done; done
done
done_count=0

echo "=== Main top-up: $total runs ==="

# CP — fill the 30->50 gap (the headline floor)
for topo in $TOPOS; do
  for rate in 35 40 45; do
    for seed in $SEEDS; do
      out="results/${topo}-cp-r${rate}-run${seed}"
      if [ -s "${out}-rate-trace.txt" ]; then
        echo "[SKIP] ${topo}-cp r=${rate} run${seed}"
      else
        echo "[RUN]  ${topo}-cp r=${rate} run${seed}"
        ./build/${topo}-cp --attackRate=$rate --rngRun=$seed
      fi
      done_count=$((done_count+1)); echo "       Progress: ${done_count}/${total}"
    done
  done
done

# IFA — probe below r10 to find its floor
for topo in $TOPOS; do
  for rate in 1 2 4 6 8; do
    for seed in $SEEDS; do
      out="results/${topo}-ifa-r${rate}-run${seed}"
      if [ -s "${out}-rate-trace.txt" ]; then
        echo "[SKIP] ${topo}-ifa r=${rate} run${seed}"
      else
        echo "[RUN]  ${topo}-ifa r=${rate} run${seed}"
        ./build/${topo}-ifa --attackRate=$rate --rngRun=$seed
      fi
      done_count=$((done_count+1)); echo "       Progress: ${done_count}/${total}"
    done
  done
done

# Optional backfill: r50/r100 CP, seeds 2-5 (run1 already exists)
if [ "$1" == "backfill" ]; then
  echo "=== Backfill: CP r50/r100 seeds 2-5 ==="
  for topo in $TOPOS; do
    for rate in 50 100; do
      for seed in 2 3 4 5; do
        out="results/${topo}-cp-r${rate}-run${seed}"
        if [ -s "${out}-rate-trace.txt" ]; then
          echo "[SKIP] ${topo}-cp r=${rate} run${seed}"
        else
          echo "[RUN]  ${topo}-cp r=${rate} run${seed}"
          ./build/${topo}-cp --attackRate=$rate --rngRun=$seed
        fi
      done
    done
  done
fi

echo ""
echo "=== Top-up complete ==="

# Sanity checks
echo ""
echo "--- 1) New file count (expect 120 main) ---"
ls results/{tree,dfn,dumbbell}-cp-r{35,40,45}-run*-rate-trace.txt \
   results/{tree,dfn,dumbbell}-ifa-r{1,2,4,6,8}-run*-rate-trace.txt 2>/dev/null | wc -l

echo ""
echo "--- 2) Seeds differ ---"
diff -q results/tree-cp-r40-run1-rate-trace.txt results/tree-cp-r40-run2-rate-trace.txt >/dev/null 2>&1 \
  && echo "[WARN] CP identical!" || echo "[OK] CP seeds differ"
diff -q results/tree-ifa-r4-run1-rate-trace.txt results/tree-ifa-r4-run2-rate-trace.txt >/dev/null 2>&1 \
  && echo "[WARN] IFA identical!" || echo "[OK] IFA seeds differ"

echo ""
echo "--- 3) Rate scales (ifa-r8 > ifa-r2 attacker OutInterests) ---"
for f in cp-r40 ifa-r2 ifa-r8; do
  v=$(awk -F'\t' '$1>400 && $2==6 && $5=="OutInterests"{s+=$6;n++} END{if(n)print s/n}' \
        results/tree-${f}-run1-rate-trace.txt)
  echo "  tree-${f} node6 OutInterests/s = $v"
done

echo ""
echo "--- 4) Background alive at t=400 ---"
awk -F'\t' '$1==400 && $2>=7 && $2<=11 && $5=="OutInterests"{print "  node",$2,"=",$6}' \
  results/tree-normal-run1-rate-trace.txt