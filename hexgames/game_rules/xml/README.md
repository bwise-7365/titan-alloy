# hexrules — an XML rule language for hex wargames

`hexrules.xsd` defines the language. The three instance documents rebuild the
rule digests in `..\*.md`:

| File | Game | Validates |
|---|---|---|
| `d-day-at-tarawa.xml` | D-Day at Tarawa (solitaire, cards, no dice) | yes |
| `the-russian-campaign.xml` | The Russian Campaign, 5th ed. (two players, one die) | yes |
| `dai-senso.xml` | Axis Empires: Dai Senso! (three factions, card selection) | yes |

Validated with libxml2 2.11.9 via `lxml`, the same engine XML Copy Editor uses.
No namespace; each file points at the schema with `xsi:noNamespaceSchemaLocation`.

## Design in one paragraph

The digests show that most mechanisms come in two or three incompatible shapes
and many are one-offs, so the language does not try to formalise every clause.
It gives **typed elements to the data that recurs in the same shape** across
all three games, **two generic containers** for enumerations and charts, and
**one uniform escape hatch** — `<rule>` — for everything else. Every leaf is
"attributes for the machine, one sentence for the reader"; every container ends
with the same annex of `note`, `rule`, `list` and `table` elements in any order.

## Element map

```
game  @id @title @publisher @year @players @source @hex-scale @turn-scale
  sides            side* (@id @name @control=human|automaton @hostile-to)
  map              hex-ids (@pattern @example)
                   hex-terrain*, hexside-terrain*   -- same type: @move-cost @stop @stop-except
                                                       @enter-only @defence-multiplier @shift @blocks
                   network*      (@carries @mutable)
                   region-layer* (@partition @bounded-by @mutable) > region*
                   space*        (@kind=box|pool|track|display @side @returns)
  counters         unit-type*    (@side @kind @steps @zoc @stacking @hidden)
  sequence         phase*        (@side @turns @condition @optional) -- phases nest
  stacking         @units @steps @enforced @repair @exempt
  zoc*             @side @range @projected-by @blocked-by @negated-by-friendly
                   @stops-movement @zoc-to-zoc-forbidden @mandatory-attack @blocks
  movement         @budget=actions|hexes|points > mode* (@phase @network @randomizer)
  supply           trace* (@sources @max-length @blocked-by-zoc @threshold @fatal @checked)
                     > segment* (@order @kind=free|network|region-gated @length @network @layer)
  combat           @mandatory > resolver* (@kind @randomizer @min-odds @max-odds @rounding)
                              modifier* (@applies-to @kind @value @cap @phase)
  retreat?         @routed-by @distance @monotone @advance-after-combat @unsatisfiable @blocked-by
  randomizer*      @kind=die|deck|hand @size @side @fields
  weather?         @source=none|rolled|printed @randomizer @layer
  victory          condition* (@side @kind=immediate|scheduled|final @turns)
  (annex)          note | rule | list>item | table>col,row>cell   -- also allowed inside every container
```

`rule` carries `@id @ref @topic @phase @sides @units @turns @optional`. `@ref` is
the rulebook section. Every `@id` is an `xs:ID`, so it must be unique in the
document; attributes that name other elements (`blocked-by`, `units`, `phase`,
`network`, `layer`, `randomizer`, `bounded-by`, `exempt`, `projected-by`,
`stop-except`, `enter-only`, `hostile-to`, `side`) are `IDREFS` and must resolve.

## Conventions

- `shift` on terrain counts CRT columns in the defender's favour; on a
  `modifier`, `applies-to` says who benefits.
- `move-cost` on a hex is the entry cost; on a hexside it is added, unless the
  text says it replaces the hex cost (Dai Senso roads).
- `turns` selects game turns: `5`, `1-10`, `11+`, `3,5,7,9`. TRC dates are
  converted to turn numbers (turn 1 = May/June 1941); the file lists the mapping.
- Table bodies are prose or codes; XSD 1.0 cannot check that each row has one
  cell per column, so the validation script does.

## Validate

```bash
python validate.py
```

(`validate.py` sits beside the files; it loads the XSD, validates each
`*.xml`, and checks table cell counts.)
