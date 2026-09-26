Copyright Ben Paul Wise. All Rights Reserved.

# The Russian Campaign (5th edition) map: reading notes (by eye), 2026-09-25

Result: `map_graphics/xml/the-russian-campaign.xml`. It validates and renders. The grid, palette,
terrain and line styles, country borders, panels and non-place labels were kept. Terrain, water,
rivers, rails, prohibited hexsides, military districts and the positions of 13 places were re-read.
Method: `doc/map-reading-by-eye.md`.

## Sources
- `The Russian Campaign 5th map.jpg`, 1224 x 1483: the sheet's pixel space. The existing grid fits it
  exactly (checked at both corners).
- `TRC map north reduced.png` and `TRC map south reduced.png`, 8160 x 6120 photographs with folds.
  They were registered to the sheet by cross-correlating about 180 patches each: an affine map plus an
  interpolated local residual, median error about 3 sheet px. They were used to settle every question
  that needed detail: coasts, cities, rails, districts, prohibited hexsides.

## How each layer was read
- Terrain: the colour fractions in each hex on the scan (flat colour; the photos' lighting is uneven).
  The dominant class is used when it covers 30% or more of the hex. The 135 coastal hexes were read
  one by one on the photos. The sea/land rule is the printed one: a hex ringed by the light land
  outline is land, otherwise sea. Hexes under the printed panels over the sea are sea.
- Rivers: thin blue features in the photos (the blue mask minus its lakes and seas), scored per hex,
  written as one hex chain per named river and checked on photo overlays. They are river links, drawn
  centre to centre; `TrcFacts::readRivers` reads the river network (rule 14.1.1).
- Rails: traced by eye on full-resolution photo crops, with thin dark lines highlighted. Waypoints are
  joined by the shortest lattice path that hugs the drawn line. 68 links, all steps adjacent.
- Military districts (green dashed): scored by the dash colour along each hexside, short gaps bridged
  along hexsides, false hits in woods, mountains and on the San removed by eye. 75 hexsides.
- Prohibited hexsides (white dots): 16, read by eye.

## Corrections to the previous sheet
- Places: 13 were in the wrong hex, several of them on water. Warsaw P21 -> L26, Helsinki C14 -> B14,
  Sevastopol KK23 -> JJ23, Astrakhan QQ5 -> PP5, Gorki U2 -> U1, Posen I25 -> H29, Breslau O28 -> K30,
  Kaunas K21 -> J21, Königsberg G23 -> H23, Krasnodar OO14 -> NN16, and the oil fields Ploesti
  AA30 -> AA29, Maikop OO12 -> PP13, Grozny QQ6 -> QQ7 (the derricks; the names are printed nearby).
- Blocked hexsides: all 32 old ones lay along panel edges; replaced by the 16 printed ones.
- Rails: the old links followed rivers in the south; all 68 re-read.
- Sea: panel-covered sea hexes (A19-A23, B19, B21, B24, KK28 ...) are now sea. Riga, Helsinki and
  Sevastopol are now on land, which removes the "cities on water" data gap in `TrcFacts`.

## Doubts for Ben
1. Lakes: Saimaa (A9, A10) and the Stettin lagoon (D29) are now land. Only a small part of each hex is
   water.
2. Rivers are now centre-to-centre `<link kind="river">` chains (28), as printed; the "Lielupe" of the
   first reading was a misread and is gone. Confluences are shared hexes (the sheet is implicit). Every
   link has `ends`; a river rising on the map ends in `source` (approved 2026-09-25).
3. Rails, uncertain steps: Konigsberg - J24 ends in the woods (unexplained end); Orsha - Smolensk
   (O16 P15 P14) is inferred; Bryansk - south joins the Kiev - Kursk line at W15; T9 - Saratov and the
   X8 junction; Kharkov - Stalino; Dnepropetrovsk - Stalino; Astrakhan - south-west ends at QQ6.
4. Districts: the Leningrad district line (A6-C7) and the short E10:nw piece are recorded. The sheet
   does not print a full partition, so no district regions are defined.
5. The Kerch Strait edge (KK20:e) was kept as it was.

Copyright Ben Paul Wise. All Rights Reserved.
