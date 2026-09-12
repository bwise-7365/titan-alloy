# Common Abstractions Across Three Hex Wargames

A synthesis of `d-day-at-tarawa.md`, `the-russian-campaign.md` and `dai-senso.md`,
written to scope a reusable C++ game library resting on the `hexmap` ABC/QRS
coordinate system (`C:\repos\ghub-per\panj\hexmap`).

The three games were chosen as use cases, not as reimplementation targets. The
question this document answers is: **which mechanisms genuinely repeat, and which are
one-offs?** A mechanism that appears in all three, in the same shape, belongs in the
core library. One that appears in all three but in three incompatible shapes belongs
in the library as an *interface* with a game-supplied implementation. One that appears
once belongs entirely in that game's own layer.

The three games sit at very different points in the design space, which is what makes
them useful together:

| | D-Day at Tarawa | The Russian Campaign | Dai Senso |
|---|---|---|---|
| Publisher, year | Decision Games, 2014 | Consim Press / GMT, 2022 (5th) | Decision Games, 2011 |
| Players | 1 (solitaire) | 2 | 3 factions |
| Scale | 100 yards / 30–60 min | 40 miles / 2 months | 120–300 miles / 30–60 days |
| Map | one sheet, ~40 × 40 | one sheet, ~43 × 30 | two sheets |
| Randomiser | 54-card deck, no dice | one die | one die, plus card *selection* |
| Sides symmetric? | no, radically | yes | no, three-way |

---

## 1. Feature matrix

`Y` present in the ordinary form; `V` present but in a variant shape that would break
a naive shared implementation; `—` absent.

### 1.1 Map and geometry

| Mechanism | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| Single contiguous hex grid | Y | Y | V — two sheets, prefixed IDs |
| Printed hex ID scheme | Y — four digits, two per axis | V — row letter + number, doubled letters past Z | V — sheet letter + four digits |
| Total order over hexes required by rules | Y — "west to east", "lowest numbered" | Y — hex rows addressed by letter | Y — named edge spans |
| Hex terrain types | Y | Y | Y |
| Hexside (edge) terrain | Y — seawall, pier, reef | Y — river, blocked, coastline, strait, borders | Y — eight distinct types, in their own printed chart |
| Centre-to-centre link network | V — position connectors only | Y — railroads | Y — roads and rails, *also* charged as hexside costs |
| Named hex regions | Y — position groups, zones, water arcs | Y — countries, military districts, sea areas | Y — countries, dependents, regions, weather areas, naval zones |
| Overlapping (non-partitioning) region layers | Y | partly | Y — five independent labellings |
| Off-map spaces with node identity | Y | Y | Y — and partly spatial |
| Unit facing | V — landing only | — | V — Beachhead markers only |
| Directed rays / spine walks | Y — landing runs | — | — |

### 1.2 Units and stacking

| Mechanism | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| Printed combat strength | Y (both sides, different meaning) | Y — one factor, attack and defence | Y — separate attack and defence |
| Printed movement allowance | — | Y | Y |
| Steps / step reduction | V — US only, 1–4 | V — via replacement pool, not printed steps | Y |
| Unit type affects movement | Y | Y | Y |
| Hidden units | Y — core mechanic | — | — |
| Stacking limit | Y — 2 units, phase boundary | Y — by unit size, multiset constraint | Y — 3 units and 6 steps, plus nationality rules |
| Stacking violations repaired destructively | Y | Y | Y |
| Leader / HQ counters | Y — heroes, HQs, command posts | Y — Hitler, Stalin, army group HQs, Stavka | Y — HQ units with ranged support |
| Markers that behave as quasi-units | Y — garrisons, naval fire | Y — workers, partisans, railheads | Y — detachments, logistics, partisan bases, airdrops |

### 1.3 Movement

| Mechanism | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| Movement point budget | — (action budget instead) | V — every land hex costs exactly 1 MP | Y — real variable costs |
| Terrain movement *cost* | — | — | Y |
| Terrain forces a **stop** on entry | Y | Y | Y |
| Enemy presence forces a stop | Y — intense field of fire, adjacency | Y — enemy ZOC | Y — EZOC under weather |
| Must-stop-in-first-hex overflow rule | — | — | Y |
| Rail / strategic movement | — | Y | V — port-to-port and off-map-box moves |
| Sea movement | V — amphibious landing only | Y | Y — zonal, convoy-gated |
| Air / paradrop movement | — | Y — paratroops | Y — airdrops as a two-stage marker |
| Overrun during movement | — | V — automatic victory at 10-1 | Y |
| Second movement phase after combat | — | Y — second impulse | Y — reserve movement phase |

### 1.4 Zones of control

| Mechanism | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| ZOC = six adjacent hexes | V — US control, only for 2+ step infantry and tanks | Y | Y |
| ZOC blocked by hexside type | — | Y — water, blocked hexsides, strait | Y — mountain, all-sea, strait |
| Friendly unit negates enemy ZOC | — | — | Y |
| ZOC forces a stop | V — fields of fire do | Y | Y — only under weather |
| ZOC-to-ZOC movement forbidden | — | Y — "pinning" | — |
| ZOC blocks supply tracing | Y — both communication rules | Y | Y |
| ZOC blocks retreat | — | Y | Y |
| Reduced ZOC for special units | Y — disrupted units keep a partial effect | Y — partisans, battlegroups | Y — Policy Affected Countries |
| Authored, irregular projection sets | Y — printed fields of fire | — | — |

### 1.5 Supply

| Mechanism | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| Trace to a source set | Y — both sides | Y | Y |
| Bounded trace length | — (unlimited) | Y — 8 hexes, 4 in snow | V — 2 free hexes then network |
| Blocked by enemy ZOC | Y | Y | Y |
| Blocked by enemy-held points | Y — beach hexes, US control | Y — enemy cities | Y — enemy country cities and ports |
| Trace along a link network | — | Y — rail | Y — road and rail, with a one-stretch road limit |
| Trace across regions gated by occupancy | — | — | Y — naval zones need a convoy marker |
| Reachability to *k* ≥ 2 targets | Y — Japanese communication needs two | — | — |
| Out of supply is fatal | — | Y — immediate elimination | — (capability loss only) |
| Separate "combat supply" | — | Y — snow turns only | — |

### 1.6 Combat

| Mechanism | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| Odds ratio into a table | V — comparison bands, not a ratio | Y — 9 columns, rounded to defender | Y — 1-3 to 9-1, rounded to defender |
| Die roll | — | Y — one die | Y — one die |
| Column shift modifiers | V — reveal shifts one column right | Y — air, artillery; capped at 3 | Y — eight sources, cumulative |
| Die roll modifiers in combat | — | — | — |
| Strength multipliers from terrain | Y — doubling, halving | Y — doubling, saturating at ×2 | — (shifts instead) |
| Defence value depends on attack *directions* | Y — flanking, seawall | Y — river, Kerch Strait | Y — hexside shift, lowest applicable |
| Mandatory combat | — | Y | — |
| Minimum legal odds | — | Y — 1-6, else surrender | Y — raw 1-3, else no attack |
| Retreat results | V — close combat pushback only | Y — 1–2 hexes, attacker routes | Y — 1–3 hexes, monotone distance |
| Advance after combat | — | — | Y |
| Exchange results | — | Y | Y |
| Surrender distinct from elimination | Y — withdraw to reserve vs eliminate | Y — pool vs permanent box | V — Delay Box vs Force Pool |
| Second combat resolver in the same game | Y — attack chart, fire chart, close combat | — | V — Blitz and Regular use one resolver, two masks |
| Weapon / capability requirement matching | Y — set cover with wild cards | — | — |
| Hidden defender strength | Y | — | — |

### 1.7 Time, force generation, politics

| Mechanism | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| Fixed turn track | Y | Y | Y |
| Track used as a numeric register | Y — command range | Y — weather DRM, worker RPs | Y — scheduled events |
| Scheduled reinforcement | Y — printed arrival turn | Y — order of battle chart | V — mostly via cards |
| Random-draw reinforcement pool | Y — Japanese reserve, depth markers | — | V — Force Pool, but selection is deliberate |
| Replacement point economy | — | Y — Russian workers; Axis shopping list | Y — replacement locations |
| Delayed return of eliminated units | Y — overnight recycling | Y — replacement pools | Y — Delay Box with countdown |
| Weather | — | Y — rolled, with a cumulative DRM | Y — printed on the turn track, per region |
| Card-driven events | Y — one deck, drawn | — | Y — three decks, selected |
| Political state (countries, surrender) | — | Y — minor ally surrender, garrison triggers | Y — a whole subsystem |
| Victory by holding named hexes | Y — position hexes | Y — sudden death objective sets | Y — strategic hexes on a VP track |
| Sudden / automatic victory | Y — catastrophic loss | Y — both directions | Y — three ways |

---

## 2. What belongs in the core library

These appear in all three games in compatible shapes.

**Hex geometry with edge and corner addressing.** Adjacency, distance, direction,
rings, and the ability to name an edge between two hexes. All three games attach rules
to hexsides; two of them print a separate hexside column in their terrain chart.

**A hex attribute table** keyed by coordinate, with the game supplying the attribute
schema. All three want terrain type plus a handful of per-hex flags.

**An edge attribute table** keyed by edge address. Tarawa uses four edge attributes,
TRC seven, Dai Senso eight. This is not an optional extra.

**Region tables**: named sets of hexes, possibly overlapping, with membership queries
and with region-level state. All three use these; Dai Senso needs five independent
labellings at once.

**Adjacency filtered by an edge predicate.** TRC and Dai Senso both block ZOC across
particular hexside types; TRC blocks movement across coastlines; Dai Senso blocks
movement, ZOC, supply and retreat across different subsets of hexside types. The
primitive is `neighbours(hex, edge_predicate)`.

**Blocked path search with a source set and a blocking set**, with an optional length
bound. All three supply-trace rules are instances, as are TRC's retreat search and
rail movement.

**A second graph over hex centres.** TRC's railroads and Dai Senso's road/rail network
are both sparse networks that carry movement and supply independently of hex
adjacency. Tarawa's position connectors are the degenerate case. The library should
support named link networks with per-link and per-node state.

**Stacking as a phase-boundary invariant with a destructive repair.** All three enforce
limits at the end of a phase or segment, allow violation during it, and eliminate the
excess. The constraint itself differs — a count, a multiset over unit sizes, a count
plus a step total plus a nationality rule — so the *predicate* is game-supplied but the
*enforcement schedule* is shared.

**Step reduction and unit pools.** All three remove strength one step at a time and all
three distinguish "returnable to a pool" from "permanently gone." Dai Senso's Delay Box
and Tarawa's overnight recycling are both "returns after a delay."

**A phase/segment scheduler** with per-phase capability masks. TRC's two impulses, Dai
Senso's Blitz and Regular combat segments, and Tarawa's basic-versus-extended sequence
are all "the same machinery runs under a different mask."

**Tracks as registers.** All three store at least one numeric value by placing a marker
on a numbered track: Tarawa's command post range, TRC's weather DRM and worker
replacement points, Dai Senso's VP and ESV tracks. A `Track` type with a marker position
and a value mapping serves all of them.

**An ordered tie-break chain** over a candidate set, terminating in "the player chooses."
Tarawa needs this everywhere because it is solitaire; TRC needs it for retreat routing
and replacement substitution; Dai Senso needs it for the Control priority list.

---

## 3. Where the three disagree — interfaces, not fixed rules

These appear in all three but in shapes that cannot be unified into one parameterised
implementation. The library should define an interface and let each game supply the body.

### 3.1 Combat resolution

| | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| Input | strength comparison band, weapon set cover, reveal state | integer odds ratio, column shifts | integer odds ratio, net column shift |
| Randomiser | card draw, or none | one die | one die |
| Output | qualitative codes, one of which re-enters the lookup | 9 result codes | retreat code plus an attrition pair |

Tarawa additionally has **three** unrelated resolvers of its own — the attack chart,
the fire chart, and close combat by opposed card piles. Any attempt to write one
`resolveCombat(attack, defence, terrain, roll)` will fit TRC, roughly fit Dai Senso,
and fail Tarawa entirely.

**Recommendation.** A `CombatResolver` interface taking a `CombatContext` (participants,
target hex, attack directions, accumulated modifiers) and returning a list of
`CombatEffect` values. Provide an odds-table implementation in the library, because two
of three games want it, and let games register others.

### 3.2 Zones of control

Three genuinely different definitions:

- Tarawa: asymmetric. One side has *control* (own hex, plus six adjacent for two unit
  types only); the other has *authored fields of fire* that are not derived from range
  at all.
- TRC: symmetric six-hex ZOC, blocked by three hexside types, with a distinctive
  ZOC-to-ZOC movement prohibition.
- Dai Senso: symmetric six-hex ZOC, blocked by three hexside types, **negatable by a
  friendly unit**, and with that negation switched off by weather.

The negation rule alone means ZOC cannot be a precomputed hex set: it depends on
friendly occupancy and on the current weather in the hex.

**Recommendation.** A `ZocPolicy` interface answering `projectsInto(unit, from, to)`
and `isBlockedFor(hex, side, purpose)`, where *purpose* distinguishes movement, supply,
retreat and rail. The purpose parameter is not optional — TRC's partisans and Dai
Senso's weather rules both make ZOC purpose-dependent.

### 3.3 Movement

- Tarawa: a budget of **actions**, each buying a move of up to *n* hexes, with a stopping
  predicate on entry and no per-hex cost.
- TRC: a budget of **hexes**, every land hex costing exactly one, with a stopping
  predicate per unit type and terrain.
- Dai Senso: a budget of **movement points**, with real hex costs *and* real hexside
  costs, a first-hex overflow allowance, and three mutually exclusive movement
  procedures per phase.

The common shape is: *a budget, a step-wise walk, an entry legality predicate, and a
stopping predicate.* Point-cost movement is the special case where the budget decrements
by an edge-and-node weight. Tarawa is the special case where the weight is zero and the
budget is external.

**Recommendation.** Implement the general walk in the library, with `entryCost`,
`canEnter` and `mustStop` supplied by the game. Do not build the library around
Dijkstra over hex costs; two of these three games would not use it.

### 3.4 Supply

- Tarawa: unlimited-length blocked reachability, but to **two distinct targets** for one
  side and to a **region** for the other.
- TRC: bounded-length blocked reachability to a source set, plus reachability along a
  link network to a further source set.
- Dai Senso: a **layered** trace — two free hexes, then a link network with a
  one-contiguous-stretch limit on roads, then region-gated sea legs.

**Recommendation.** A `SupplyTrace` interface built on a shared bounded blocked-search
primitive, with the game supplying the source set, the blocking predicate, the per-segment
budgets, and the count threshold. The threshold is what Tarawa needs and the other two
do not; make it a parameter with default 1 rather than a special case.

### 3.5 Randomness

- Tarawa: a 54-card deck with three orthogonal result fields per card, sampled without
  replacement over an unknown horizon.
- TRC: one six-sided die.
- Dai Senso: one six-sided die, *plus* three card decks that are **selected from**, not
  drawn from, under a lattice of constraints.

These are not variants of one thing. A die is memoryless; Tarawa's deck is not; Dai
Senso's cards are not random at all at the point of use.

**Recommendation.** Three separate types — `Die`, `Deck` (shuffled, drawn, discarded,
conditionally reshuffled), and `Hand` (selected from under constraints) — over a common
`Card` payload type. Do not try to unify `Deck` and `Hand`.

### 3.6 Retreat

- Tarawa: no retreat; only pushback out of a close-combat hex to an adjacent hex,
  preferring one in communication.
- TRC: 1–2 hexes for the attacker, 2 for the defender; the **attacker** routes both sides;
  excluded from enemy ZOC, off-map, across coastlines and impassable hexsides; woods
  forbid retreat entirely, converting the result.
- Dai Senso: 1–3 hexes; the **owner** routes; each step must be strictly farther from the
  origin; two-tier priority with a friendly-unit fallback; unsatisfiable retreat converts
  to step losses; retreating into friendly units sweeps them along.

The monotone-distance requirement in Dai Senso and the attacker-routes rule in TRC are
mutually incompatible search formulations.

**Recommendation.** A `RetreatPolicy` interface. The library can offer the underlying
"legal adjacent hexes given a set of exclusions" primitive, which all three use.

---

## 4. Game-specific layers

Mechanisms appearing in exactly one game, which should live outside the core.

**Tarawa only:**

- Hidden units and depth markers, with reveal as a combat result.
- Authored fields of fire printed per hex, with intense/steady grades and runtime
  dilation.
- Water fire arcs bounded by printed hexside lines.
- Amphibious landing with facing, drift, and hexrow/spine runs.
- Close combat by opposed card piles.
- A defender action matrix unlocked progressively over time.
- Weapon-requirement set cover with wild cards.
- An action economy rather than a movement economy.

**The Russian Campaign only:**

- Rail gauge conversion with an advancing and retreating railhead frontier, validated at
  two points in time.
- Automatic victory attacks resolved during the movement phase.
- Off-board encirclement matching as a scalar debt between players.
- Leader elimination zeroing a nation's movement for one impulse.
- A cumulative weather die-roll modifier carried across turns.
- Partisans as an indestructible, relocating obstacle with a chosen subset of ZOC effects.
- Worker units as a spatial economy.

**Dai Senso only:**

- The three-faction structure, with the two Allied factions hostile to each other.
- The full political model: country status, alignment, posture, policies, truces,
  conquest, liberation, reactivation.
- Naval Zones as a region layer with occupancy state gating movement and supply.
- Option-card selection with a one-season pipeline and a constraint lattice.
- Beachhead markers as directional bridges out of All-Sea hexes.
- Overrun blocked by a counterfactual combat-shift evaluation.
- Airdrop markers as an intermediate counter state.
- The War State ratchet.
- Attacker first-loss ordering derived from force composition.

---

## 5. The case for edge and corner addressing

This is the section that bears most directly on `hexmap`'s three-basis-vector system.
`CoordABC` names hexes *and* vertices; `CoordQRS` names centre-to-centre steps. The
argument for keeping both is that the source material is full of rules that are written
about edges, and a hex-only coordinate system has to encode each of them as a pair of
hexes, losing the object the rule actually names.

### 5.1 Edge-attached attributes, counted

| Game | Hexside attributes on the map |
|---|---|
| Tarawa | seawall, pier, coral reef, water fire zone boundary — **4** |
| TRC | river (with identity), blocked hexside, all-water hexside, black coastline, Kerch Strait, country border, military district border — **7** |
| Dai Senso | mountain, river, lake, all-sea, strait (connected), strait (unconnected), naval zone border, country/dependent border, region border — **9** |

Dai Senso's printed Terrain Effects Chart is *itself* split into "Hex Terrain Type" and
"Hexside Terrain Type," each with its own movement-cost and combat-shift column. The
designers reached the same conclusion the coordinate system does.

### 5.2 Rules that name an edge, not a pair of hexes

- **TRC 14.1.1** — "Two adjacent river hexes are connected if a river crosses the
  **hexside** between the two hexes." The rule needs the river's occupancy of a specific
  edge *and* the river's identity, because the doubling applies only when attacker and
  defender are on different rivers. Storing "this hex is a river hex" is not enough.
- **TRC 8.5** — units may not cross **black coastal lines** except at one named hexside,
  the Kerch Strait between `KK19` and `KK20`. That single edge carries five separate rule
  effects: a movement stop, a ZOC block, a voluntary-attack rule, a defence doubling, and
  a supply permission.
- **Dai Senso, Terrain Effects Chart** — mountain hexsides cost +2 MP and give +2 to the
  defender; river hexsides +1 and +1; and only the **lowest** applicable hexside shift
  counts. Evaluating that requires enumerating the actual edges crossed by the attacking
  units, not the hexes they occupy.
- **Dai Senso, Beachhead Hexside** — a marker in an All-Sea hex points at one hexside.
  Units exit only through it; ZOC projects only across it; attacks are made only across
  it. The marker's state *is* an edge reference.
- **Dai Senso, Naval Zone Border Hexsides** — the naval regions are defined by an edge
  labelling, and two zones are adjacent if they share a border hexside. Region adjacency
  is derived from the edge set.
- **Tarawa 6.23** — water fire arcs are bounded by *fire arc lines* drawn on hexsides. A
  single printed dot colours every water hex reachable without crossing one of those
  lines. Membership is a flood fill whose walls are edges.
- **Tarawa, seawall hexsides** — the defensive doubling applies only if **every** adjacent
  attacker is attacking across such a hexside. The modifier is a predicate over the set of
  attack edges.

### 5.3 Centre-to-centre links that are not merely adjacency

- **TRC railroads** connect hex centres and form a network that is traversed by rail
  movement and followed by supply, with its own per-hex ownership state. It is not a
  subgraph of adjacency in any useful sense: a rail line may run through hexes whose
  terrain is irrelevant to it (9.3.3), and one named segment may be used for rail movement
  but not for normal movement or supply (8.6).
- **Dai Senso roads and rails** are the sharpest case. They are drawn centre to centre and
  carry the supply trace as a network. But the Terrain Effects Chart charges them as a
  **hexside** cost, at ½ MP for a one-step unit and 1 MP for a multi-step unit, replacing
  the destination hex's cost. The same drawn line is simultaneously a graph edge between
  centres and a modifier attached to the hexside it crosses. A coordinate system that can
  name both objects can represent that directly.
- **Tarawa fire position connectors** join position hexes into groups. A small static
  undirected graph whose connected components are the groups.

### 5.4 Rays, spines and directions

Tarawa's amphibious landing is the one place among the three that needs directions as
first-class values rather than as adjacency:

- Beach approach hexes carry a printed **facing arrow**, and the `PR` drift result rotates
  that facing "to the next hexside to the right" — a rotation among the six QRS directions.
- Landing results are **rays**: "move along the hexrow in the direction indicated until
  reaching a water hex marked with an LVT wreck." A ray in a fixed QRS direction with a
  stopping predicate.
- The Beach Red 1 run follows a **hex spine** "first to the left, then to the right in a
  zig-zag pattern." A hex spine is the chain of hexsides along a column boundary. In ABC
  terms it is a sequence of edge addresses; the zig-zag walk is an alternating sum of two
  QRS basis vectors.

A hex-only system can produce the same *sequence of hexes*, but it cannot name the spine
the rule is describing, and the implementation ends up as a special case rather than an
instance of a general operation.

### 5.5 Corners

Corner (vertex) addressing is the weakest of the three cases from this sample. None of the
three games attaches a rule to a vertex directly. Two indirect uses appear:

- The TRC map draws several rivers so that they pass through hex corners rather than
  running cleanly along a chain of hexsides; representing the river's geometry faithfully
  needs vertices even though no rule reads them.
- Tarawa's "dots are printed on the side of the hex nearest to the projecting position"
  (6.22) is a rendering convention that a vertex-aware system expresses naturally.

So corners earn their place in `hexmap` for **rendering and authoring fidelity**, not
because these three rulebooks demand them. That is worth stating plainly: the edge case is
strong and well evidenced; the corner case is presently a rendering argument.

---

## 6. Suggested scoping

A first cut at what to build, in dependency order.

**Layer 0 — geometry.** `hexmap` as it stands: `CoordABC`, `CoordQRS`, distance, edge
addressing, direction values and rotation. Add: edge-between-two-hexes construction, ray
cast with predicate, spine walk, ring and neighbourhood queries, and a stable total order
derived from a game-supplied printed-ID mapping.

**Layer 1 — map data.** Hex attribute table, edge attribute table, named link networks,
named regions with overlapping membership and region-level state. All schemas
game-supplied.

**Layer 2 — search.** Edge-filtered adjacency; the general step-wise walk with entry cost,
entry legality and stopping predicates; blocked reachability with a source set, a length
bound and a target-count threshold; region flood fill bounded by edges; connected
components over a link network; monotone-distance walk for retreats.

**Layer 3 — game state.** Counters with side, type, strength, steps and status flags;
stacks with a game-supplied constraint and a phase-boundary enforcement hook; pools with
delay countdowns; tracks as registers; mutable per-hex and per-region ownership with
last-toucher semantics.

**Layer 4 — scheduling and resolution.** Phase and segment scheduler with capability masks;
ordered tie-break chains; the `CombatResolver`, `ZocPolicy`, `SupplyTrace` and
`RetreatPolicy` interfaces, with an odds-table combat implementation and a
six-hex-with-edge-filter ZOC implementation provided as defaults.

**Not in the library.** Randomness containers beyond `Die` and a plain `Deck`; anything
political; anything naval-zonal; anything to do with hidden information. Those are
game-layer concerns, and the sample of three does not support generalising any of them.

---

## 7. Open questions

- **Does the library need hidden state at all?** Only Tarawa hides anything, and it hides
  it on one side only. A `revealed` flag on the counter plus query flavours covers it. A
  general fog-of-war system would be built on a sample size of one.
- **How much of the region machinery is really shared?** Tarawa and TRC use regions as
  static authored sets. Dai Senso uses them as mutable political state with five
  overlapping labellings. The static case is clearly shared; the mutable case may not be.
- **Should link networks and edge attributes be the same table?** Dai Senso's roads argue
  yes — one drawn line, two roles. TRC's railroads argue no — the network is traversed
  centre to centre and the hexsides crossed are irrelevant. Probably two tables with a
  documented correspondence, rather than one.
- **What is the right granularity for the phase scheduler?** TRC has impulses within player
  turns; Dai Senso has segments within phases within faction turns; Tarawa has a flat
  phase list that changes shape at turn 11. A three-level nesting (turn → sub-turn →
  phase) covers all three, but it may be simpler to treat the whole thing as a flat named
  sequence with capability masks and let games nest as they wish.
