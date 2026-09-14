Copyright Ben Paul Wise. All Rights Reserved.

# Task 06: Panzergruppe Guderian rules digest and rules XML (milestone M7, first half)

status: review
worker: W5 (sonnet)          started: 2026-09-14
resume: read CLAUDE.md, PLAN.md, TRC digest/xml/hexrules.xsd/README, PGG map+counters XML (full,
  via grep since large: 9 major cities Rzhev/Vitebsk/Gzhatsk/Vyazma/Smolensk/Orsha/Mogilev/Roslavl/
  Kaluga + 4 towns; rail+road link networks; no region-layers/panels/spaces in PGG sheet; counters:
  Soviet army HQs "(n)*10" family=unit kind hq-shaped but tagged family=unit name=army-hq-like id
  pattern "s1-<leader>-<n>th-army-hq-N-10"; Soviet units untried U-N back faces; German
  divisions(4 steps: 9-7/4-7/2-7/1-7)+regiments(2 steps, echelon=regiment, UR corps code like 3E/2A/
  9G); SS "4(DE)" "3(DR)" "2(GR)" regiment codes; markers: cross(front balkenkreuz/back DIS),
  DIS(front DIS/back Out-of-Supply or RAIL CUT), boom(front cloud/back Out-of-Supply), Lw + sair
  (support family, front aircraft silhouette/back Air Interdiction -- German and Soviet air-interdiction
  units); read PGG rule-summary PDF (2pp) -- captured SoP, untried units, stacking, movement, rail
  (Soviet only), overrun, disruption, air interdiction, combat, retreats, advance, Soviet leaders,
  reinforcements, victory (occupation-based, VP bands). NEXT: read the normative rulebook PDF and CRT
  PDF, then amendments/errata, then write the digest .md
inputs:
  C:/Library/War-Games/Panzergruppe Guderian/Panzergruppe_Guderian_rule_summary.pdf   (read first: shape)
  C:/Library/War-Games/Panzergruppe Guderian/PGG rules counters map v1.pdf              (normative text)
  C:/Library/War-Games/Panzergruppe Guderian/PGG_CRT.pdf                                (the table)
  C:/Library/War-Games/Panzergruppe Guderian/PGG-amendments.pdf,
    Panzergruppe_Guderian_Clarifications_Errata_and_Addenda.pdf                         (each clause -> a rule, ref "errata:")
  hexgames/game_rules/the-russian-campaign.md                (the digest FORMAT to follow, section for section)
  hexgames/game_rules/xml/the-russian-campaign.xml, hexrules.xsd, game_rules/xml/README.md (the language)
  hexgames/map_graphics/xml/panzergruppe-guderian.xml       (terrain, line and link ids the rules must name)
  hexgames/unit_graphics/xml/panzergruppe-guderian.xml      (counter families, value lines "2-7", "U-6", "(3)*10")
  hexgames/packages/xml/trc.package.xml (manifest example), hexpackage.xsd
  hexgames/tools/validate-xml.py, CLAUDE.md
outputs:
  hexgames/game_rules/panzergruppe-guderian.md      (digest, same section order as the TRC digest)
  hexgames/game_rules/xml/panzergruppe-guderian.xml (validates; same style as the TRC instance)
  hexgames/packages/xml/pgg.package.xml             (sides by counter style: soviet/soviet-back/german/german-back/ss/plain/redmarker;
                                                     terrain/hexside/network/space bindings; unit bindings by regex)
  hexgames/game_records/xml/pgg-test.xml            (a small test scenario on real hex ids of the PGG sheet)
acceptance:
  python tools/validate-xml.py game_rules/xml packages/xml game_records/xml -> all valid
  hexgames_cli --validate packages/xml/pgg.package.xml -> no problems (build tree cmake-build-debug)
  every typed element PGG needs exists in hexrules.xsd; if one is missing (candidate: unit-type/@reveal
    for untried Soviet units), do NOT edit the XSD -- express it as a <rule> and list the proposal here
  banner-check green (first and last line of the .md)
log:
- 2026-09-13 created by the coordinator (not yet launched)
- 2026-09-14 relaunched as W5 (3rd restart) after connection drops caused by reading the PDF
  directly. Coordinator pre-extracted the rulebook to scratchpad S (see relaunch prompt); this run
  never opens a PDF. Read S/text/page-01..08.txt (full rulebook 1.0-16.2), S/crt.txt (clean CRT/TEC/
  supply summary), S/amendments.txt (Alanthwaite fan variant, ref "amendments:"), S/errata.txt
  (official SPI corrections, ref "errata:"). Confirmed against map_graphics/xml/panzergruppe-guderian.xml
  and unit_graphics/xml/panzergruppe-guderian.xml: grid flat/offset-odd id-format {col:02}{row:02}
  (4-digit hex ids, e.g. 2117=Smolensk); hex-terrain fills only clear/woods/swamp/lake (cities are a
  glyph overlay, symbol="city-major" x9 = the Major Cities of the VP schedule, symbol="town" x4
  (Velizh 1509, Bely 3005, Mstislavl 1524, Krichev 1427) = the Minor Cities); edge line values river/
  rail/road only (river = hexside terrain, rail/road = link kind, no separate "border" hexsides in
  PGG); link kind="rail"/"road" (432 links, both networks drawn centre-to-centre, rail mutable via
  errata 6.37 rail-cut). Counter ids: Soviet army HQs s1-<leader>-<n>th-army-hq-N-10; Soviet infantry
  s1-<id>-inf-<atk>-<def>-<mov>; German infantry regiments s2-<id>-inf-2-7 (2-step, echelon regiment);
  German motorized regiments s2-<id>-mot-9-7; independent regts s2-gd-mot-4-10, s2-lehr-mot-3-10;
  German army-corps-coded infantry divisions s2-<rgt>-<div>-inf-2-10 (id encodes regiment/division,
  e.g. 3/3, 12/4); armour s2-<rgt>-<div>-arm-4-10; markers family=marker: cross (balkenkreuz/DIS),
  dis (DIS), boom (cloud/OOS); support family: lw (Lw air, German air interdiction), sair (Soviet air
  interdiction). Package/scenario formats confirmed from trc.package.xml, hexpackage.xsd,
  game_records/xml/trc-test.xml, hexsave.xsd -- style follows exactly.
  NEXT: write game_rules/panzergruppe-guderian.md (digest, TRC section order), then
  game_rules/xml/panzergruppe-guderian.xml, validating after; then pgg.package.xml, then
  game_records/xml/pgg-test.xml, validating each.
- 2026-09-14 (cont.) game_rules/panzergruppe-guderian.md written (5 sections, TRC order),
  banner first/last line confirmed manually (tools/banner-check.py's ROOTS list does not include
  game_rules/, so it never scans this file either way -- same as the other three digests).
  game_rules/xml/panzergruppe-guderian.xml written and validates
  (python tools/validate-xml.py game_rules/xml -> all 4 files OK). Fixed on the way: xs:ID/attribute
  rules actually enforced by the XSD (network/space/unit-type/list have no @ref; phase has no
  @units) -- folded those citations into prose instead; escaped "<=0" as "&lt;=0" in a list @code.
  NEXT: write packages/xml/pgg.package.xml (model on trc.package.xml), validate with
  python tools/validate-xml.py packages/xml; then game_records/xml/pgg-test.xml, validate with
  python tools/validate-xml.py game_records/xml; then hexgames_cli --validate if the build tree
  has it; then fill in the Notes/proposals section below and set status: review.
- 2026-09-14 (cont., final) packages/xml/pgg.package.xml written and validates; ran
  cmake-build-debug/tools/hexgames_cli.exe --validate packages/xml/pgg.package.xml (pre-existing
  build tree, not rebuilt) -> "0 problems" once game_records/xml/pgg-test.xml existed (its first
  run correctly reported only the then-missing scenario file, confirming every side/terrain/
  hexside/network/space/unit binding in the package resolves and every counter in
  unit_graphics/xml/panzergruppe-guderian.xml matches exactly one <unit> binding). Wrote
  game_records/xml/pgg-test.xml (7 units, real hex ids), validates. Full acceptance sweep:
  `python tools/validate-xml.py game_rules/xml packages/xml game_records/xml` -> 11 files, all
  valid; `python tools/banner-check.py .` -> 276 files, 0 failures. All four outputs delivered.
  status: review. See "## Notes" below for schema proposals and open questions.

## Brief for the worker (paste as the agent prompt, prefixed by the standard W-role preamble)
You are worker W5. Read this task file first and stay within its inputs. Produce the Panzergruppe
Guderian rules digest and rules XML the way the other three games were done: digest first (Standard
Written English, plain words, rule references), then the XML within the existing hexrules schema.
PGG specifics to capture: flat-topped map `{col:02}{row:02}`; German divisions/regiments with
breakdown and regroup; Soviet untried units ("U" strength revealed on first combat, face-up on
reveal); army HQs "(n)*10"; overrun during movement with a die; disruption (DIS) markers; out of
supply; rail cut and air interdiction markers; road and rail networks with mutable rail; corps
attachment codes as a list; the CRT as a table with defender-rounded odds; sequence of play as the
rules number it; victory by cities/VP schedule. Scope every prose <rule> with @phase/@units/@sides
where the text allows. Validate after every file. Update the resume line after every step. Finish
with status: review and a list of any schema additions you wish you had (proposals only).

## Notes

**Outputs delivered**, all validating:
- `game_rules/panzergruppe-guderian.md` — digest, same five-section shape as the TRC digest
  (Game Overview, Standard Elements, Unique Exceptions, Map and Coordinate Notes, Library
  Implications). Copyright banner is the literal first and last line, checked by hand: it is
  outside `banner-check.py`'s `ROOTS` list (which omits `game_rules/`, same as for the other three
  digests, so the script does not scan any of them).
- `game_rules/xml/panzergruppe-guderian.xml` — validates against `hexrules.xsd`.
- `packages/xml/pgg.package.xml` — validates against `hexpackage.xsd`, and
  `hexgames_cli --validate` (pre-existing `cmake-build-debug` tree, not rebuilt) reports 0
  problems: every side/terrain/hexside/network/space/unit binding resolves, and every
  `family="unit"`/`family="support"` counter in the counters document matches exactly one `<unit>`
  binding.
- `game_records/xml/pgg-test.xml` — a 7-unit scenario on real hex ids (Soviet 16th/19th Army
  set-up hexes 2216/1414, German supply-road hex 0120, Vitebsk 0513), validates.

**Schema proposals (not applied — `hexrules.xsd` was not edited)**

1. `unit-type/@reveal` (enumeration, e.g. `none` | `on-combat`). PGG's Untried mechanic is more
   specific than the existing `hidden` boolean: a unit is not merely "placed face down," it
   irreversibly flips at the *instant* of its own first combat or Overrun (12.2), and that flip is
   itself a rule-bearing event — a unit revealing at zero Combat Strength is removed outright, or
   (for the two 0-1-6 exceptions) becomes defend-only and forces a Leader-style retreat if it was
   attacking (12.3). I worked around this with `hidden="true"` plus three `<rule>` elements
   (`untried-placement`, `untried-reveal-timing`, `untried-zoc`, `untried-dead-pile`); a typed
   `@reveal` attribute would let an engine dispatch the timing mechanically rather than by
   convention, and would generalise to any other hidden-unit game the family later adds.

2. A way to mark a CRT cell's result as carrying a **conditional side effect**. PGG's `crt.txt`
   marks certain Defender results with a trailing `*` meaning "this Defender loss/retreat also
   Disrupts the unit, but only when the combat was an Overrun" (6.61) — a per-cell annotation that
   is neither a `modifier` (it is not a strength/DRM shift) nor expressible in the plain-text
   `table`/`cell` content without inventing an ad hoc convention. I encoded it as a `<rule
   id="overrun-disruption">` cross-referencing the `*`-suffixed codes in prose. A `list`/`item`
   (the `crt-results` list already carries the code glossary) could instead carry an optional
   `causes` `IDREFS` attribute pointing at a marker/status `unit-type`, conditioned on a `context`
   attribute (e.g. `context="overrun"`) — general enough to cover similar "outcome X also does Y
   under condition Z" patterns in other games' CRTs.

3. `Segment/@length` has no way to say "the bound is a value printed on a *specific* unit, not a
   game-wide constant." Soviet supply's Line of Communications is bounded by the coordinating
   Leader's own printed Radius rating (10.1, 11.22), which varies per Leader counter. I left
   `length` off that `segment` and described the bound in the segment's prose instead, which
   validates but is not machine-checkable the way a numeric `length` is. An `Extent` enumeration
   value such as `per-unit` (parallel to the existing `authored`, "the set is printed on the map,
   not computed") would make this pattern checkable elsewhere too.

**Open questions for Ben (not blocking — these are about the counter-id-to-rules-type binding in
`pgg.package.xml`, not about the rules digest or the rules XML)**

- The German counter sheet's id shape does not cleanly separate Panzer-regiment (Panzer Grenadier)
  components of a Panzer Division from the own regiments of a standalone Motorized Infantry
  Division: both use a `<regiment>-<division>-inf-<n>-10` / `-mot-<n>-10` id shape. I bound *all*
  such regiment-coded `inf`/`mot` ids (movement 10) to a single `german-motorized` unit-type
  alongside the true Motorized Infantry Division's regiments, which is defensible for combat/
  movement purposes (both classes are 2-step, `zoc="full"`, per rule 9.63's own grouping) but
  loses the Panzer-vs-Motorized division *identity* at the type level; which division a regiment
  belongs to (needed for the Divisional Integration check, 7.3) has to come from the save file's
  `unit/@attached` grouping, not the rules `unit-type`. `hexgames_cli --validate` only checks that
  every counter matches exactly one binding, not that the binding is the semantically "right" one,
  so this passed cleanly without confirming the grouping is correct.
- The counter sheet's German Infantry Division counters (18 divisions, matching the 16.2
  reinforcement list exactly) carry only two printed values, `9-7` and `2-7` (one `unit-type` id
  per division, two faces), not the rulebook's stated four-step progression `9-7/4-7/2-7/1-7`
  (9.63). The digest keeps the rulebook's stated four steps (`steps="4"` on `german-infantry`
  in the rules XML) since that is what the source text says; whether this specific remake counter
  mix implements the missing two intermediate steps by some other mechanism (a generic reduction
  marker?) was not established from the pages supplied, and is worth a look before anyone builds a
  step-reduction table keyed to specific printed counters.
- `packages/xml/pgg.package.xml`'s `<unit type="soviet-armour" match="...">` pattern uses a
  negative lookahead, `(?!gd-mot|lehr-mot)`, to keep the two independent German regiments (bound
  explicitly to `german-motorized`) from also matching the broad Soviet-armour pattern. This
  validated and `hexgames_cli` accepted it, so the engine's regex engine supports lookahead, but
  it is the one pattern in the file that depends on that support rather than on a plain anchored
  match; flagging it in case a future non-PCRE regex backend is ever substituted.

Copyright Ben Paul Wise. All Rights Reserved.
