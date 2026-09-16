Copyright Ben Paul Wise. All Rights Reserved.

# examples/pgg -- the pilot's own working files

The `work/` folder is not kept in git (it holds tiles, crops and catalogues a run regenerates), so these
copies of the Panzergruppe Guderian pilot's files are kept here. Read them before writing your own for a
new map; the templates they were built from are in `../../README.md`.

| file | what it is | written in |
|---|---|---|
| `reader-prompt.txt` | the prompt the tile readers were given, filled in for PGG | stage 5b |
| `verify-prompt.txt` | the prompt the verifiers were given | stage 9 |
| `read-0117_0420.json` | worked catalogue record: Orsha, a dense rail junction | stage 5a |
| `read-2117_2420.json` | worked catalogue record: Smolensk, a two-hex city with vertex passes | stage 5a |
| `markers.json` | the edge-marker pass: entrance brackets, set-up lines, victory-point texts | stage 5d |

Both worked records carry a `notes` array giving the reasoning for every hard call. Those notes became the
numbered decision rules of the reader prompt; do the same on a new map, because the rules that matter are
the ones its own print forces.

What is specific to PGG, and must be re-derived rather than copied: the vocabulary (how this print draws
each feature, with its colours), the decision rules about lines along hexsides and through vertices, and
the list of printed furniture to ignore. What carries over: the shape of both prompts, the hard rules
(never open the source images, write each record as the tile is finished), and the output schemas.

Copyright Ben Paul Wise. All Rights Reserved.
