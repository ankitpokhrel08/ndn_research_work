# paper_writing/

Draft manuscript for *Discover Networks* (Springer, open access, ISSN 3004-9792):
"Detecting Network Anomalies in Named Data Networking Through Isolation
Forest-Based Behavioral Analysis."

## Layout

- `main.tex` — top-level document; `\input`s the ten files in `sections/` in
  order (abstract, introduction, related-work, method, emulation, simulation,
  mechanism, discussion, conclusion, declarations).
- `sections/` — one `.tex` file per section.
- `figures/` — figures referenced via `\includegraphics`; `\graphicspath` is
  set to this directory in `main.tex`.
- `references.bib` — BibTeX database (added in a later task).
- `check.sh` — grep-based structural verifier that does not require a LaTeX
  engine. Run it after every change:

  ```bash
  bash paper_writing/check.sh
  ```

  It checks for leftover placeholder tokens (`TODO`, `TBD`, `FIXME`, `XXX`,
  `Lorem`, `???` — except the intentional `TODO-TEMPLATE` marker described
  below), that every `\ref`/`\eqref`/`\autoref` has a matching `\label`,
  that every `\cite` key exists in `references.bib`, and that every
  `\includegraphics` file exists under `figures/`.

## Template status

The official Springer Nature `sn-jnl` class could not be downloaded in this
environment (the documented template URL returned 404 and no `sn-jnl`
package could be located on CTAN or GitHub). `main.tex` currently uses a
fallback preamble: standard `article` class, 11pt, with `natbib` configured
for numbered citations (`unsrtnat` style) plus `graphicx`, `booktabs`,
`amsmath`, `subcaption`, `hyperref`, and `geometry`. The fallback is marked
with a `% TODO-TEMPLATE:` comment at the top of `main.tex`; `check.sh`
deliberately ignores that token. Before submission, swap in the real
`sn-jnl.cls` (and its bibliography style, e.g. `sn-mathphys-num.bst`) and
remove the fallback packages that duplicate its functionality.

## Compiling

### Overleaf

Upload the `paper_writing/` directory (or zip its contents) as a new
Overleaf project. Overleaf ships a full TeX Live install, so no local setup
is required.

### Local (tectonic)

[Tectonic](https://tectonic-typesetting.github.io/) is a self-contained
LaTeX engine that fetches packages on demand — no separate TeX Live install.

```bash
brew install tectonic   # macOS
cd paper_writing
tectonic main.tex
```

If tectonic is not installed, `check.sh` remains the verification gate for
structural correctness; a full compile can be deferred to Overleaf or a
later task once tectonic (or another LaTeX distribution) is available.
