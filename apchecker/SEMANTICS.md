# SEMANTICS

The normative definition of the rule language. `rules.xml` is a specification;
the Java in `src/` is one implementation of it. Anything else that reads the same
`rules.xml` and the same plan — a hand-optimized C++ checker, say — must produce
the same report, byte for byte, and this document says what that means.

Everything here is stated over a **partial design**: a plan may have no doors, no
entrance, no common area, unattached rooms, or nothing at all. None of that is an
error.

**A verdict has three values, not two.** A rule about stairs, applied to a design
in which no stair has been placed, is not *satisfied* — it has not been *asked*.
Its verdict is `UNDETERMINED`. Reporting it as satisfied would tell a search that
a constraint had been met when nothing whatever has been done about it, which is
worse than saying nothing.

## 1. The plan

A plan is a finite undirected graph *G* = (*V*, *E*) with no self-loops and no
parallel edges. Each edge carries a **kind**, `DOOR` or `OPENING`; the kind is not
part of the edge's identity, so a duplicate unordered pair is rejected whatever
its kind.

- *V* is the rooms, in declaration order. *V* **may be empty**: the empty design
  — no rooms, no doors, not even the outdoors — is the root of a search and a
  legitimate input.
- **Any number** of rooms have kind `EXTERIOR`, including none, and they are
  exactly the rooms whose privacy is `EXTERIOR`. Each carries an `outdoor` tag —
  `STREET`, `DRIVEWAY`, `GARDEN`, `TERRACE`, `COURT`, `OTHER` — which is required
  for an exterior room and forbidden for any other. The empty design has none, and
  then ⟦`exterior`⟧ = ∅ and every traversal from it reaches nothing.

  Several exterior vertices do **not** remove the walk-around-the-outside
  artifact: two doors onto the same `STREET` still share a vertex, so leaving by
  the front door and entering by the garage door is a walk of length two. The
  `avoid=` and `exclude=` arguments in `rules.xml` are what suppress it.
- *E* is the doors, an unordered pair each. `E` may be empty.
- Privacy ranks are totally ordered: `EXTERIOR`(0) < `ENTRY`(1) < `PUBLIC`(2) <
  `SEMI_PRIVATE`(3) < `PRIVATE`(4).
- `lengthFeet` is present exactly on rooms of kind `PASSAGE`, and nowhere else.
  A rule that reads it from a room lacking it is an **error**, not a zero. See §5.

## 2. Arithmetic

Distances are counts of doors: non-negative integers, plus a value **∞** meaning
"no such walk exists". The order is: *n* < ∞ for every integer *n*, and ∞ = ∞.

An implementation may use IEEE doubles, as the Java does, since ∞ is exactly
representable and comparisons behave as required. It may equally use an integer
with a sentinel. It must **not** use a large finite number as a stand-in for ∞:
`APL-129-through` turns on `dist < ∞` and would silently invert.

`lengthFeet` is a decimal. Comparisons against it are exact; no tolerance is
applied and none may be.

## 3. Set expressions

Write ⟦·⟧ for the value of an expression, a subset of *V*.

**Base**

| Expression | Value |
|---|---|
| `all` | *V* |
| `exterior` | { the `EXTERIOR` room } |
| `interior` | *V* minus the `EXTERIOR` room |
| `emptySet` | ∅ |
| `kind value="K"` | { *v* : kind(*v*) = K } |
| `privacy level="P"` | { *v* : privacy(*v*) = P } |
| `privacyAtLeast level="P"` | { *v* : rank(privacy(*v*)) ≥ rank(P) } |
| `outdoor kind="K"` | { *v* : *v* is exterior and its outdoor tag is K } |
| `var name="x"` | { the room *x* is bound to } — an error if *x* is unbound |
| `attrAtLeast name="a" value="c"` | { *v* : *v* has attribute *a*, and its value ≥ c } |
| `attrAtMost name="a" value="c"` | { *v* : *v* has attribute *a*, and its value ≤ c } |

A room lacking the attribute is **not a member** of `attrAtLeast` or
`attrAtMost`. That is not a defaulted value: the set is "the rooms whose
`lengthFeet` is at least 30", and a room with no `lengthFeet` is not one of them.
Contrast `attrOf` (§4), which *reads* the value and must fail when it is absent.
Membership and arithmetic are different questions and get different answers.

**Algebra.** `union` and `intersect` take two or more operands; `minus` and the
predicates below take exactly two; the rest take one. `complement` is relative to
*V*.

- ⟦`neighbors` S⟧ = N(⟦S⟧) \ ⟦S⟧, where N(A) = { *v* : ∃*u*∈A, {*u*,*v*} ∈ *E* }.
  **The argument is removed.** Two adjacent rooms of the same kind do not make
  each other members of `neighbors(kind:K)`.
- ⟦`closedNeighborhood` S⟧ = N(⟦S⟧) ∪ ⟦S⟧.

**Derived.** These are the four places an algorithm runs. A rule may name them;
it may not define them.

- ⟦`reachable` from=F avoid=A⟧ = every room reachable in *G* − ⟦A⟧ from some room
  of ⟦F⟧ \ ⟦A⟧, **including those source rooms themselves** (distance 0). If
  ⟦F⟧ ⊆ ⟦A⟧ the result is ∅.

- ⟦`monotoneReachable` from=F⟧ = every room *v* for which there is a walk
  *u* = *w*₀, *w*₁, …, *w*ₖ = *v* with *u* ∈ ⟦F⟧ and
  rank(privacy(*wᵢ*)) ≤ rank(privacy(*wᵢ₊₁*)) at every step. Sources are
  included. Note this is a walk, not a path, and the constraint is on
  consecutive rooms, not on the endpoints.

- ⟦`cutVertices` exclude=X⟧ = the articulation points of the subgraph of *G*
  induced on *V* \ ⟦X⟧. **The graph may be disconnected**; the articulation
  points of a disconnected graph are the union of those of its components. A
  room whose removal disconnects nothing — an isolated room, a leaf — is not a
  cut vertex.

- ⟦`centroid` among=M over=O avoid=A⟧: let *C* = ⟦M⟧ \ ⟦A⟧ and *T* = ⟦O⟧ \ ⟦A⟧.
  For each *c* ∈ *C*, let total(*c*) = Σ_{*t*∈*T*} d(*c*, *t*), the distance
  taken in *G* − ⟦A⟧. If any *t* is unreachable from *c*, total(*c*) = ∞ and *c*
  is not a minimizer. The result is **every** *c* attaining the minimum finite
  total; ties are not broken. If no *c* has a finite total, the result is ∅.
  If *T* = ∅, every *c* has total 0 and the result is *C*.

  A tie is a fact about the plan. An implementation that returned one arbitrary
  minimizer would make its verdict depend on room declaration order, which is a
  bug the reference implementation once had.

## 4. Scalar expressions

- ⟦`count` S⟧ = |⟦S⟧|.
- ⟦`literal` value="c"⟧ = c. ⟦`infinity`⟧ = ∞.
- ⟦`dist` from=F to=T avoid=A⟧ = min over *u* ∈ ⟦F⟧\⟦A⟧, *v* ∈ ⟦T⟧\⟦A⟧ of the
  distance from *u* to *v* in *G* − ⟦A⟧. **If either set is empty after removing
  ⟦A⟧, the result is ∞.** If some *u* is also some *v*, the result is 0.
- ⟦`attrOf` name="lengthFeet" var="x"⟧ = the `lengthFeet` of the room *x* is
  bound to. If that room has none, **the implementation must abort with an
  error**, not substitute a default. A corridor of unknown length must not
  quietly pass a length limit; that is why `APL-132` restricts its domain to
  `PASSAGE`.

## 5. Predicates

A predicate is **two-valued**: satisfied, or broken with reasons. Undeterminedness
lives at the rule level (§5a), not inside the predicate language. That is a
deliberate choice: a three-valued Kleene logic propagated through every connective
would make the meaning of a rule depend on subtle interactions the author cannot
see, and each implementation would get the corners differently.

A predicate evaluates to *satisfied*, or to *broken* together with a non-empty
list of **failures**. A failure carries the rooms and doors to blame and the
values of the message's placeholders.

| Predicate | Satisfied when | Failures when broken |
|---|---|---|
| `subset` A B | ⟦A⟧ ⊆ ⟦B⟧ | one per room of ⟦A⟧ \ ⟦B⟧; placeholder `room` |
| `disjoint` A B | ⟦A⟧ ∩ ⟦B⟧ = ∅ | one per room of the intersection; placeholder `room` |
| `nonempty` A | ⟦A⟧ ≠ ∅ | exactly one, with no rooms and no placeholders |
| `isEmpty` A | ⟦A⟧ = ∅ | one per room of ⟦A⟧; placeholder `room` |
| `noEdge via="V"` A B | no connection **of manner V** joins ⟦A⟧ to ⟦B⟧ | one per offending connection; placeholders `a` (the ⟦A⟧ end) and `b` (the ⟦B⟧ end) |
| `hasCycle` A | the subgraph induced on ⟦A⟧ contains a circuit | exactly one, blaming all of ⟦A⟧ |
| `compare` op x y | ⟦x⟧ op ⟦y⟧ | exactly one, with no rooms |

`via` is `ANY`, `DOOR` or `OPENING`, and is **required**: there is no sensible
default, and guessing would be a silent substitution that surfaces as a bug later.
`ANY` forbids the adjacency outright; `OPENING` forbids only the unclosable sort,
which is how "the garage separation shall be equipped with a door" is stated — the
garage may adjoin the house, but not through an archway.

`noEdge` orients each witness so that `a` is always the ⟦A⟧ end, whichever way the
connection was declared. Without that, half the messages would read backwards.

**The binder.** `forAll var="x"` D P: satisfied when P is satisfied for every
binding of *x* to a room of ⟦D⟧. **Vacuously satisfied when ⟦D⟧ = ∅** — a plan
with no stairs satisfies `APL-133`. Each failing binding contributes the body's
failures, each extended with the placeholder *x* and with that room added to the
blame list. Bindings are visited in declaration order (§6).

There is exactly one binder and it binds one variable. Two bound variables and a
join between them would make this a query language, and the cost of a rule would
stop being predictable from reading it.

**Connectives.**

- `and`: satisfied when all parts are. Broken: the concatenated failures of every
  broken part. **No short-circuit** — all parts are evaluated, so that all
  failures are reported.
- `or`: satisfied when some part is; evaluation stops at the first satisfied
  part. Broken: the concatenated failures of all parts.
- `not`: satisfied when the operand is broken. Broken: one failure, with no
  rooms and no placeholders. **Witnesses are lost**; a message under a `not`
  cannot name anything.
- `implies` A C: satisfied when A is broken, or when A and C are both satisfied.
  **A's failures are discarded**: a guard that does not fire is not a fault. A
  message may therefore not use a placeholder that only A could supply, and the
  loader rejects a rule that tries.

## 5a. The verdict of a rule

A rule is `id`, `severity`, `message`, an optional `<given>` presupposition, and
the asserted predicate. Its verdict is computed thus, in order:

1. **If the design has no rooms at all**, the verdict is `UNDETERMINED`, and
   neither predicate is evaluated. Nothing has been placed, so there is nothing
   any rule can yet be right or wrong about.
2. **Otherwise, if `<given>` is present and broken**, the verdict is
   `UNDETERMINED`. The `<given>`'s failures are **discarded**: an inapplicable
   rule is not a fault. A message may therefore not use a placeholder that only
   the presupposition could supply, and the loader rejects a rule that tries.
3. **Otherwise** the asserted predicate is evaluated: `SATISFIED` if it holds,
   `BROKEN` with its failures if it does not.

`UNDETERMINED` is produced in exactly these two ways and is **never inferred**.
An empty `forAll` domain remains vacuously satisfied, an empty `noEdge` operand
remains vacuously satisfied, and so on; if a rule ought to be undetermined in
those cases, it says so with a `<given>`. Whether a rule applies is a design
decision, and design decisions belong in the rule file where they can be read and
argued with, not in an inference buried in an evaluator.

## 6. Canonical order and conformance

Two reports **agree** when, for every rule, they give the same outcome, and for a
broken rule the same *set* of findings. Two things do **not** count as a
disagreement:

1. the **order** of the verdicts, or of the findings within a verdict;
2. the **omission** of an `UNDETERMINED` verdict. A rule missing from a report is
   read as undetermined, so an implementation may report only what it has
   something to say about.

That is the operational definition, and `roomgraph.ReportCompare` implements it.
A byte-for-byte diff would be a stronger condition than the specification
requires, and would reject a correct implementation for printing in a different
order.

The reference implementation emits **every** rule, in the order `rules.xml`
declares them, with room witnesses in plan declaration order and door witnesses in
door declaration order. That is a courtesy to a human reader, not a requirement on
a conforming one.

## 7. Load-time checks

The schema, `rules.xsd`, enforces the grammar and the three-sorted type
discipline: `setExpr`, `scalarExpr` and `predicate` are separate substitution
groups, so a scalar in a set position is a schema error. `attrOf` declares its
operand as `<var>`, so "an attribute belongs to one bound room" is grammatical.

Three conditions the schema cannot see, which a conforming loader must check
before running anything:

1. every `<var name="x"/>` lies within a `<forAll var="x">`;
2. no `forAll` shadows a name already in scope;
3. every `{placeholder}` in a message is a variable the rule binds, or a witness
   its predicates can produce (`room`, or `a` and `b`), with the antecedent of an
   `implies` contributing nothing.

A rule file that fails any of these is rejected. Discovering the fault later,
when a plan happens to break the rule, is the worst possible time.

## 8. What the language deliberately cannot do

**The two levels are stratified, and that is the whole architecture.**

A rule, as written in `rules.xml`, is a first-order formula with **at most one
bound variable**, over a signature of monadic predicates (`kind`, `privacy`,
`outdoor`, the attribute filters), one binary relation used only through
`neighbors` and `closedNeighborhood`, and the precomputed relations of §3. It has
no recursion operator and no fixpoint operator. It cannot define one, and it
cannot diverge.

Everything that *needs* a fixpoint lives in the four derived sets of §3, computed
in Java. Membership in a derived set is a plain set membership, because the
closure has already been taken.

**Cost.** Each derived set is one traversal — a BFS for `reachable` and
`monotoneReachable`, a DFS for `cutVertices`, and |`among`| BFS runs for
`centroid` — memoized on its evaluated arguments. A rule is then evaluated in
O(|*V*| + |*E*|) per binding, with at most |*V*| bindings. This is what makes the
checker safe to call from inside a search that will run it at every node it
visits: the cost of a rule can be read off the rule.

**What the stratification is not.** It is tempting to say the rule language "sits
below Datalog"; the statement needs care, and unqualified it is wrong.

- Of a *rule*, it is true: no recursion is available to its author, so a rule is
  strictly weaker than a Datalog program.
- Of the *system*, it is false. `reachable` is transitive closure, which is
  exactly what Datalog can express and first-order logic cannot. `centroid` and
  `count` are aggregation, which plain Datalog cannot express at all. The system
  is not weaker than Datalog; it is **incomparable** — weaker in the recursion a
  rule may use, stronger in aggregation.

The point of the design is not to be below some line on a chart of expressive
power. It is that recursion and aggregation are confined to a **closed catalog**
of four algorithms, whose cost and termination are known, and the rule author gets
a first-order logic over their results. (Datalog, for the reader who wants the
reference point: Abiteboul, Hull and Vianu, *Foundations of Databases*,
Addison-Wesley 1995, chapters 12–13; free from the authors at
<http://webdam.inria.fr/Alice/>.)

The catalog is **closed**. A rule may name a derived set; it may not define one.
Adding an algorithm means new code and a new element in `rules.xsd` — a
deliberate speed bump, because a rule file that could define its own fixpoints
would be a programming language wearing a schema.
