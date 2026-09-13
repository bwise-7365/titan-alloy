Copyright Ben Paul Wise. All Rights Reserved.

# Task 06: Panzergruppe Guderian rules digest and rules XML (milestone M7, first half)

status: assigned
worker: W5 (sonnet)          started: (not yet)
resume: (worker updates this line after every step)
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

Copyright Ben Paul Wise. All Rights Reserved.
