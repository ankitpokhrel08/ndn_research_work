#!/bin/bash
# paper_writing/check.sh — structural verification (no LaTeX engine needed)
cd "$(dirname "$0")"
fail=0
# 1. Placeholder scan
if grep -RInE 'TODO|TBD|FIXME|XXX|\bLorem\b|\?\?\?' sections/ main.tex 2>/dev/null | grep -v 'TODO-TEMPLATE'; then
  echo "FAIL: placeholder tokens found"; fail=1
fi
# 2. Every \ref/\eqref/\autoref target has a matching \label
for r in $(grep -RhoE '\\(ref|autoref|eqref)\{[^}]+\}' sections/ | sed -E 's/.*\{([^}]+)\}/\1/' | sort -u); do
  grep -RqF "\\label{$r}" sections/ || { echo "FAIL: undefined ref: $r"; fail=1; }
done
# 3. Every \cite key exists in references.bib
for c in $(grep -RhoE '\\cite[a-z]*\{[^}]+\}' sections/ | sed -E 's/\\cite[a-z]*\{//; s/\}//' | tr ',' '\n' | sort -u); do
  grep -qE "^@[a-zA-Z]+\{$c," references.bib 2>/dev/null || { echo "FAIL: undefined cite: $c"; fail=1; }
done
# 4. Every \includegraphics file exists in figures/
for f in $(grep -RhoE '\\includegraphics(\[[^]]*\])?\{[^}]+\}' sections/ | sed -E 's/.*\{([^}]+)\}/\1/' | sort -u); do
  ls figures/"$f"* >/dev/null 2>&1 || { echo "FAIL: missing figure: $f"; fail=1; }
done
[ $fail -eq 0 ] && echo "OK: structural check passed"
exit $fail
