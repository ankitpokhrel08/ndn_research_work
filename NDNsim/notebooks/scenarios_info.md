# NDN Simulation Scenarios - Complete Reference

## Overview

Nine ndnSIM C++ simulation files covering **3 topologies × 3 traffic scenarios** = 9 combinations.
Each simulation runs for **600 seconds**. Attack scenarios introduce malicious traffic at **t = 300 s**,
giving a balanced 50/50 normal vs. attack split per run.

| File | Topology | Scenario | Attacker Node(s) | Attack Type |
|------|----------|----------|-----------------|-------------|
| `tree-normal.cpp` | Tree | Normal | - | None |
| `tree-cp.cpp` | Tree | Cache Poisoning | c1, c6 | CP |
| `tree-ifa.cpp` | Tree | Interest Flooding | c1 | IFA |
| `dfn-normal.cpp` | DFN | Normal | - | None |
| `dfn-cp.cpp` | DFN | Cache Poisoning | c1, c6 | CP |
| `dfn-ifa.cpp` | DFN | Interest Flooding | c1 | IFA |
| `dumbbell-normal.cpp` | Dumbbell | Normal | - | None |
| `dumbbell-cp.cpp` | Dumbbell | Cache Poisoning | c1, c3 | CP |
| `dumbbell-ifa.cpp` | Dumbbell | Interest Flooding | c1 | IFA |

---

## Global Simulation Parameters

| Parameter | Value |
|-----------|-------|
| Simulation duration | 600 s |
| Attack start time | 300 s (attack scenarios only) |
| CS (Content Store) size | 100 entries per node |
| Link data rate | 10 Mbps (all links) |
| Standard link delay | 10 ms |
| DFN cross-link delay | 15 ms |
| Producer payload size | 1024 bytes |
| Producer prefix | `/ndn` |
| Legitimate consumer frequency | 10 interests/sec per consumer |
| Content catalog size | 100 distinct names |
| Zipf-Mandelbrot parameters | q = 0.0, s = 0.7 |

---

## Topologies

### 1. Tree Topology

Used by: `tree-normal.cpp`, `tree-cp.cpp`, `tree-ifa.cpp`

**Node count:** 12 (2 producers + 4 routers + 6 consumers)

**Link map (all 10 ms, 10 Mbps):**

```
       p1    p2
        \   /
         r1
        /    \
      r2       r3
     / \       / \
   c1   c2   c3   c4
     \           /
      +--> r4 <--+      (r2→r4 and r3→r4 create a diamond)
          /  \
        c5    c6
```

| Link | Delay | Rate |
|------|-------|------|
| p1 - r1 | 10 ms | 10 Mbps |
| p2 - r1 | 10 ms | 10 Mbps |
| r1 - r2 | 10 ms | 10 Mbps |
| r1 - r3 | 10 ms | 10 Mbps |
| r2 - r4 | 10 ms | 10 Mbps |
| r3 - r4 | 10 ms | 10 Mbps |
| r2 - c1 | 10 ms | 10 Mbps |
| r2 - c2 | 10 ms | 10 Mbps |
| r3 - c3 | 10 ms | 10 Mbps |
| r3 - c4 | 10 ms | 10 Mbps |
| r4 - c5 | 10 ms | 10 Mbps |
| r4 - c6 | 10 ms | 10 Mbps |

**Routing:** r1 is the root (both producers attach here). r2 and r3 are mid-tier. r4 aggregates at the bottom.
`GlobalRoutingHelper` with `SetDefaultRoutes(true)` gives deterministic, loop-free forwarding.

---

### 2. DFN Topology

Used by: `dfn-normal.cpp`, `dfn-cp.cpp`, `dfn-ifa.cpp`

**Node count:** 12 (identical to Tree - same nodes, more links)

**Difference from Tree:** Two additional cross-links at 15 ms turn the diamond into a denser mesh
resembling a simplified version of the DFN (Deutsches Forschungsnetz / German Research Network).

**Extra links:**

| Link | Delay | Rate |
|------|-------|------|
| r1 - r4 | 15 ms | 10 Mbps |
| r2 - r3 | 15 ms | 10 Mbps |

**Full link map:**

```
       p1    p2
        \   /
         r1 ─────────── r4   (15 ms shortcut)
        /    \          /  \
      r2 ──── r3      c5    c6
(15ms) |       |
     / | \   / | \
   c1  c2  c3  c4
        (r2→r4 and r3→r4 also exist at 10 ms)
```

The extra links give routers alternative paths, making PIT / CS behaviour more complex than the pure tree.
Traffic can take multiple routes, spreading forwarding state across more nodes.

---

### 3. Dumbbell Topology

Used by: `dumbbell-normal.cpp`, `dumbbell-cp.cpp`, `dumbbell-ifa.cpp`

**Node count:** 10 (3 consumers + 2 left routers + 1 bottleneck + 2 right routers + 2 producers)

**Structure:**

```
c1 ─┐
c2 ─┤─ r1 ─ r2 ─ bn ─ r3 ─── p1
c3 ─┘              └── r4 ─── p2
```

| Link | Delay | Rate | Role |
|------|-------|------|------|
| c1 - r1 | 10 ms | 10 Mbps | Consumer access |
| c2 - r1 | 10 ms | 10 Mbps | Consumer access |
| c3 - r1 | 10 ms | 10 Mbps | Consumer access |
| r1 - r2 | 10 ms | 10 Mbps | Left aggregation |
| r2 - bn | 10 ms | 10 Mbps | **Bottleneck entry** |
| bn - r3 | 10 ms | 10 Mbps | **Bottleneck exit** |
| r3 - r4 | 10 ms | 10 Mbps | Right aggregation |
| r3 - p1 | 10 ms | 10 Mbps | Producer access |
| r4 - p2 | 10 ms | 10 Mbps | Producer access |

**Key property:** All traffic (legitimate + attack) must pass through the single `bn` node. This makes the
bottleneck the most stressed point during attacks - PIT saturation and cache pollution concentrate there.

---

## Normal Traffic (all topologies)

Normal consumers use **Zipf-Mandelbrot** request distribution (realistic popularity skew):

| Attribute | Value |
|-----------|-------|
| App | `ConsumerZipfMandelbrot` |
| Prefix | `/ndn` |
| Frequency | 10 interests/s per consumer |
| NumberOfContents | 100 |
| q (shift) | 0.0 |
| s (exponent) | 0.7 |

> **Exception:** In IFA scenarios the legitimate consumers use `ConsumerCbr` at 10 Hz (flat distribution),
> not Zipf-Mandelbrot. This is intentional - it keeps the baseline interest rate fixed so the effect of
> PIT flooding is cleanly measurable.

---

## Attack Types

### Cache Poisoning (CP)

**Goal:** Evict popular cached content by flooding the Content Store with unique, rarely-requested names.
Because the CS uses LRU eviction and holds only 100 entries, injecting 100 distinct junk names per second
displaces legitimate content, forcing cache misses and increasing producer load.

**Mechanism:**
- Attackers use `ConsumerCbr` to issue interests for unique sub-prefixes of `/ndn/junk/`
- Because producers serve the parent prefix `/ndn`, they **do respond** - the junk content
  gets cached legitimately, poisoning the store
- 50 unique names per attacker node × 2 attackers = **100 distinct junk names total**
- Each name requested at **2 Hz** → total attack rate = **200 interests/sec**

**Attack prefix pattern:**

| Attacker | Prefix range | Frequency |
|----------|-------------|-----------|
| c1 | `/ndn/junk/a0` … `/ndn/junk/a49` | 2 Hz each |
| c6 (or c3 in dumbbell) | `/ndn/junk/b0` … `/ndn/junk/b49` | 2 Hz each |

**Start time:** t = 300 s in all CP scenarios.

**Effect on topology:**
- **Tree / DFN:** Two geographically separated attackers (c1 on r2 side, c6 on r4 side) pollute
  different parts of the CS simultaneously - r2, r3, and r4 caches all affected.
- **Dumbbell:** c1 and c3 both sit behind r1; all junk traffic crosses the bottleneck, polluting
  bn and r3 caches - the nodes that serve c2's legitimate hits.

---

### Interest Flooding Attack (IFA)

**Goal:** Exhaust router PIT (Pending Interest Table) capacity by sending interests that never receive
a Data response, so PIT entries accumulate until timeout, blocking legitimate interest forwarding.

**Mechanism:**
- Attacker (c1) floods interests toward `/evil` prefix
- A **BlackholeProducer** (`blackhole-producer.hpp`) is installed on p1 and p2 for `/evil` - it
  receives interests but **never sends Data back**
- PIT entries across the path r1→r4→p1/p2 fill up and hold until their LifeTime expires
- Legitimate `/ndn` interests compete with flooded `/evil` entries for PIT slots and forwarding bandwidth

> **Dependency:** IFA simulations require the custom `blackhole-producer.hpp` header. This is not part
> of the standard ndnSIM distribution and must be present in the include path at compile time.

**Attack streams (all from c1, starting at t = 300 s):**

| App instance | Prefix | Frequency | LifeTime | PIT pressure |
|--------------|--------|-----------|----------|-------------|
| atk1 | `/evil/fake/video` | 40 Hz | 2 s | High (long-lived entries) |
| atk2 | `/evil/random-attack` | 30 Hz | 1500 ms | Medium-high |
| atk3 | `/evil/fake/image` | 20 Hz | 1 s | Medium |
| atk4 | `/evil/random-attack-2` | 10 Hz | 4 s | Low-rate, very long-lived |

**Total fake interest rate:** 40 + 30 + 20 + 10 = **100 interests/sec** from c1 alone.

**Effect on topology:**
- **Tree / DFN:** Attack traffic from c1 (on r2 side) propagates upstream through r1 all the way
  to p1/p2, saturating PIT on every router along the path.
- **Dumbbell:** All 100 fake interests/sec must cross r1→r2→bn→r3, competing with legitimate traffic
  from c2 and c3 at the bottleneck - the single most critical choke point.

---

## Scenario-by-Scenario Breakdown

### `tree-normal.cpp`
- **Topology:** Tree (12 nodes)
- **Scenario:** Baseline normal operation
- **Consumers:** c1-c6, all Zipf-Mandelbrot, 10 Hz
- **Attackers:** None
- **Duration:** 600 s, all labeled `normal`
- **Purpose:** Establishes the normal traffic fingerprint for the tree topology

---

### `tree-cp.cpp`
- **Topology:** Tree (12 nodes)
- **Scenario:** Cache Poisoning
- **Legitimate consumers:** c1-c6, Zipf-Mandelbrot, 10 Hz (run entire simulation)
- **Attackers:** c1 (50 CBR apps for `/ndn/junk/a*`) and c6 (50 CBR apps for `/ndn/junk/b*`)
- **Attack start:** t = 300 s
- **Total attack interests:** 200/sec
- **c1 position:** Behind r2 → pollutes r2 and r1 caches
- **c6 position:** Behind r4 → pollutes r4 cache
- **Ground truth labels:** `normal` (0-299 s), `cp` (300-599 s)

---

### `tree-ifa.cpp`
- **Topology:** Tree (12 nodes)
- **Scenario:** Interest Flooding Attack
- **Legitimate consumers:** c1-c6, ConsumerCbr, 10 Hz (run entire simulation)
- **Attacker:** c1 only (4 concurrent attack apps, total 100 Hz)
- **Attack start:** t = 300 s
- **c1 position:** Behind r2 → fake interests flood upstream through r1 toward p1/p2
- **Requires:** `blackhole-producer.hpp`
- **Ground truth labels:** `normal` (0-299 s), `ifa` (300-599 s)

---

### `dfn-normal.cpp`
- **Topology:** DFN (12 nodes, 14 links)
- **Scenario:** Baseline normal operation
- **Consumers:** c1-c6, Zipf-Mandelbrot, 10 Hz
- **Attackers:** None
- **Duration:** 600 s, all labeled `normal`
- **Purpose:** Normal fingerprint for the DFN topology; note extra routing paths produce different
  CS hit patterns than tree-normal despite identical application parameters

---

### `dfn-cp.cpp`
- **Topology:** DFN (12 nodes, 14 links)
- **Scenario:** Cache Poisoning
- **Legitimate consumers:** c1-c6, Zipf-Mandelbrot, 10 Hz
- **Attackers:** c1 (50 CBR for `/ndn/junk/a*`) and c6 (50 CBR for `/ndn/junk/b*`)
- **Attack start:** t = 300 s
- **Total attack interests:** 200/sec
- **DFN-specific effect:** Extra links (r1-r4, r2-r3) mean junk content can propagate into
  additional caches not present in the tree scenario, worsening pollution spread
- **Ground truth labels:** `normal` (0-299 s), `cp` (300-599 s)

---

### `dfn-ifa.cpp`
- **Topology:** DFN (12 nodes, 14 links)
- **Scenario:** Interest Flooding Attack
- **Legitimate consumers:** c1-c6, ConsumerCbr, 10 Hz
- **Attacker:** c1 only (4 attack apps, 100 Hz total)
- **Attack start:** t = 300 s
- **Requires:** `blackhole-producer.hpp`
- **DFN-specific effect:** Multiple paths for `/evil` traffic → PIT entries appear on more router
  nodes than in the tree case
- **Ground truth labels:** `normal` (0-299 s), `ifa` (300-599 s)

---

### `dumbbell-normal.cpp`
- **Topology:** Dumbbell (10 nodes)
- **Scenario:** Baseline normal operation
- **Consumers:** c1-c3, Zipf-Mandelbrot, 10 Hz (3 consumers vs. 6 in tree/DFN)
- **Attackers:** None
- **Duration:** 600 s, all labeled `normal`
- **Purpose:** Normal fingerprint for dumbbell; bottleneck node stats are the key distinguishing feature

---

### `dumbbell-cp.cpp`
- **Topology:** Dumbbell (10 nodes)
- **Scenario:** Cache Poisoning
- **Legitimate consumers:** c1-c3, Zipf-Mandelbrot, 10 Hz
- **Attackers:** c1 (50 CBR for `/ndn/junk/a*`) and c3 (50 CBR for `/ndn/junk/b*`)
- **Attack start:** t = 300 s
- **Total attack interests:** 200/sec
- **Both attackers behind r1** → all junk traffic crosses the bottleneck
- **Critical impact:** bn and r3 caches get polluted first, directly degrading c2's hit rate
  (c2 is the only non-attacker consumer)
- **Ground truth labels:** `normal` (0-299 s), `cp` (300-599 s)

---

### `dumbbell-ifa.cpp`
- **Topology:** Dumbbell (10 nodes)
- **Scenario:** Interest Flooding Attack
- **Legitimate consumers:** c1-c3, ConsumerCbr, 10 Hz
- **Attacker:** c1 only (4 attack apps, 100 Hz total)
- **Attack start:** t = 300 s
- **Requires:** `blackhole-producer.hpp`
- **Critical impact:** All 100 fake interests/sec from c1 must traverse r2→bn→r3 alongside legitimate
  traffic from c2 and c3, maximally stressing the single bottleneck link
- **Ground truth labels:** `normal` (0-299 s), `ifa` (300-599 s)

---

## Ground Truth Labels

All scenarios write a `ground-truth.csv` with per-second labels:

| Scenario type | t = 0 - 299 s | t = 300 - 599 s |
|--------------|----------------|-----------------|
| Normal | `normal` | `normal` |
| Cache Poisoning | `normal` | `cp` |
| Interest Flooding | `normal` | `ifa` |

The CSV format is:
```
time,label
0,normal
1,normal
...
300,cp       ← or ifa, depending on scenario
...
599,cp
```

This gives **300 normal + 300 attack** time-steps per attack scenario (perfectly balanced classes).

---

## Output Files per Scenario

Each of the 9 simulations writes 4 output files into `results/`:

| File suffix | Tracer | Granularity | Key columns |
|-------------|--------|-------------|-------------|
| `*-rate-trace.txt` | `L3RateTracer` | 1 s intervals | Node, FaceId, InInterests, OutInterests, InData, OutData, InNacks, OutNacks |
| `*-cs-trace.txt` | `CsTracer` | 1 s intervals | Node, CacheHits, CacheMisses |
| `*-app-delays.txt` | `AppDelayTracer` | Per-packet | SeqNo, delay, retx count, hop count |
| `*-ground-truth.csv` | (custom) | 1 s intervals | time, label |

**Full output file list:**

```
results/
├── tree-normal-rate-trace.txt       tree-normal-cs-trace.txt
├── tree-normal-app-delays.txt       tree-normal-ground-truth.csv
├── tree-cp-rate-trace.txt           tree-cp-cs-trace.txt
├── tree-cp-app-delays.txt           tree-cp-ground-truth.csv
├── tree-ifa-rate-trace.txt          tree-ifa-cs-trace.txt
├── tree-ifa-app-delays.txt          tree-ifa-ground-truth.csv
├── dfn-normal-rate-trace.txt        dfn-normal-cs-trace.txt
├── dfn-normal-app-delays.txt        dfn-normal-ground-truth.csv
├── dfn-cp-rate-trace.txt            dfn-cp-cs-trace.txt
├── dfn-cp-app-delays.txt            dfn-cp-ground-truth.csv
├── dfn-ifa-rate-trace.txt           dfn-ifa-cs-trace.txt
├── dfn-ifa-app-delays.txt           dfn-ifa-ground-truth.csv
├── dumbbell-normal-rate-trace.txt   dumbbell-normal-cs-trace.txt
├── dumbbell-normal-app-delays.txt   dumbbell-normal-ground-truth.csv
├── dumbbell-cp-rate-trace.txt       dumbbell-cp-cs-trace.txt
├── dumbbell-cp-app-delays.txt       dumbbell-cp-ground-truth.csv
├── dumbbell-ifa-rate-trace.txt      dumbbell-ifa-cs-trace.txt
├── dumbbell-ifa-app-delays.txt      dumbbell-ifa-ground-truth.csv
```

---

## Dependencies and Compilation Notes

| Dependency | Used by | Notes |
|------------|---------|-------|
| `ns3/core-module.h` | All | Standard ns-3 |
| `ns3/network-module.h` | All | Standard ns-3 |
| `ns3/point-to-point-module.h` | All | Standard ns-3 |
| `ns3/ndnSIM-module.h` | All | ndnSIM required |
| `blackhole-producer.hpp` | All IFA files | **Custom header - must be provided** |

`blackhole-producer.hpp` implements `ns3::ndn::BlackholeProducer`: an NDN application that
registers a prefix and accepts incoming interests but never sends a Data response back.
This simulates an unresponsive (or slow/dead) producer to create persistent PIT entries.

---

## Key Design Decisions

1. **CS size = 100 matches content catalog = 100.** The cache can theoretically hold every item.
   CP attack works precisely because 100 junk names fill it completely, evicting all legitimate content.

2. **CP attackers use `/ndn/junk/*` (sub-prefix of `/ndn`).** Producers serve the entire `/ndn`
   namespace, so junk requests receive real Data responses and get legitimately cached - this is
   what makes cache pollution effective vs. simply dropping unknown requests.

3. **IFA attackers use `/evil/*` (separate namespace).** The route to `/evil` is announced via
   `grHelper.AddOrigins`, so interests are forwarded all the way to the BlackholeProducer. If `/evil`
   had no route, interests would be NACKed immediately at the first router and PIT pressure would
   be minimal.

4. **50/50 class balance** (300 s normal + 300 s attack) is by design for clean ML binary
   classification without needing to handle class imbalance.

5. **Dumbbell has 3 consumers; Tree/DFN have 6.** This was chosen to keep total legitimate traffic
   comparable across topologies (3 × 10 Hz = 30 Hz vs 6 × 10 Hz = 60 Hz). Analysts should be
   aware of this difference when comparing per-node metrics across topology types.

6. **IFA legitimate consumers use `ConsumerCbr`, not Zipf-Mandelbrot.** Flat rate makes the
   pre/post attack comparison cleaner. CP scenarios keep Zipf-Mandelbrot because cache hit patterns
   (the target metric for CP) depend on popularity distribution.
