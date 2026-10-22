# Military Unit Icon Library — Project Handoff

## Goal

Build software — ideally C++, but this is negotiable — that generates military-unit
icons for hex-and-counter paper wargaming, output as PNG, SVG, or similar. The
target visual style is military-manual / wargame-counter symbology (NATO APP-6 /
MIL-STD-2525 family), not photorealistic art.

**Priorities, in order:**

1. Fits the hex-and-counter wargaming use case.
2. Composable SVG pieces (frame shapes, branch/arm icons, size/echelon marks,
   etc.) that the software assembles itself would be fine, and possibly the
   optimal approach — this matters more than the library being native C++.
3. A pure C++ library is a nice-to-have, not a hard requirement.

## Constraint: approximate, don't duplicate

There is a representative set of existing counter sheets, as PNG images, that
serve as *use cases* — reference points to confirm the software can closely
approximate known, real-world examples. The goal is **not** to exactly
reproduce them. In particular, avoid copying:

- Exact fonts
- Exact corner treatments / frame artwork
- Any other specific stylistic execution

The bar is: convey the same information (affiliation, branch, unit size,
designation, etc.), not replicate the same pixels.

## Research so far — three candidate sources of composable pieces

1. **DISA / Esri `joint-military-symbology-xml` (JMSML)**
   Official SVG files for MIL-STD-2525D / APP-6D, one file per symbol
   component (frame, branch icon, status amplifier), named by their position
   in the Symbol Identification Code so they can be layered programmatically.
   Apache License 2.0. Repo archived (read-only) since September 2024, but
   files remain downloadable.
   https://github.com/Esri/joint-military-symbology-xml

2. **milsymbol (JavaScript, MIT)**
   Actively maintained, covers MIL-STD-2525 (C/D/E) and APP-6 (B/D/E),
   outputs SVG or Canvas. Builds symbols from path data defined in its own
   code rather than separate SVG files, so using it as a *piece source*
   means writing a one-time extraction script (e.g. via Node) to dump just
   the frames/icons/marks actually needed for wargame counters into
   standalone SVG files.
   https://github.com/spatialillusions/milsymbol

3. **LaTeX `wargame` package (CC-BY-SA-4.0)**
   Purpose-built for hex-and-counter wargames (maps, counters, counter
   sheets, VASSAL export), using TikZ and NATO App6 symbology. Renders to
   PDF, not separate SVG pieces, so it's a weaker fit for a
   read-pieces-and-compose-in-C++ workflow — would need PDF-to-SVG
   conversion of whole rendered counters rather than components.
   https://ctan.org/pkg/wargame

Not evaluated in depth: `mil-sym-java` (Apache 2.0, DoD-origin, renders to
SVG, but Java rather than C++ — would need JNI or an out-of-process call).

## Not yet done

An open C++ library search did not turn up a maintained, open-source, native
C++ option — only a commercial C++ SDK (Nobori Symbology) was found. Nothing
found so far is proven wrong, just not confirmed; the search was not
exhaustive.

## Next step

Upload the representative PNG counter sheets (in whatever session picks this
up) and have Claude analyze them to derive a specification — not a
reproduction. That should describe, per counter:

- What frame shape maps to which affiliation
- Which visual slot carries branch/unit-type icon
- How unit size/echelon is marked
- Where unit designation and other text sit, and what each field encodes
- What the color coding means

That specification, expressed abstractly (shapes, positions, meanings) rather
than as exact glyphs, is what the software should implement — using one or a
mix of the composable-piece sources above as raw material.

---

## Memory file (saved context, `/areas/military-unit-icons.md`)

This is the persistent memory note Claude keeps on this project. A future
Claude Code session will not have access to it automatically — paste it in
if you want that context carried over.

```
---
name: military-unit-icons
description: Looking for an open-source C++ library to generate military unit icons (PNG/SVG) in military-manual or hex-wargame style; read when helping with this tooling
sources: [chat]
aliases: [military unit icons, NATO unit symbols, wargame counters]
---

- [stated] wants an open-source C++ library for making military unit icons in PNG, SVG, or similar format
- [stated] target style: icons like those in military manuals or hex-based paper wargames
- [stated] wargaming use case is more important than the C++-native requirement; composable SVG pieces the software can assemble would be fine, possibly optimal
- [stated] has a representative set of existing counter sheets as PNG images (not yet uploaded to chat), to use as reference use-cases so the software can closely approximate known examples without exactly duplicating them (e.g. not copying exact fonts or corner treatments)
- [stated] plans to have Claude analyze those PNG counter sheets (in a future chat, with images uploaded) to derive the requirements/spec needed to approximate them
```
