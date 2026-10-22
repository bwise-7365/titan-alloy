# roomgraph

Checks a **partial design** — a room graph, anywhere in a search tree — against a
set of rules, and reports which rules it satisfies and which it breaks.

Geometry is out of scope. Every rule here is a statement about adjacency,
reachability, or centrality, and none of them needs to know how big a room is or
where its walls run. A plan with no doors, no entrance and no common area is a
legitimate input and gets a verdict like any other.

    java roomgraph.Main --plan clean-plan.xml --rules rules.xml \
                        [--report report.xml] [--svg out.svg] \
                        [--plan-schema plan.xsd] [--rules-schema rules.xsd]

Exit status: 1 when any CODE-severity rule is broken, 2 on a bad file, 0
otherwise.

## Three XML languages, three schemas

| File | Schema | What it is |
|---|---|---|
| `*.xml` plans | `plan.xsd` | rooms (kind, privacy, `lengthFeet` for passages, `outdoor` tag for exteriors) and connections. Nothing else. |
| `rules.xml` | `rules.xsd` | the rules, as data. |
| `report.xml` | `report.xsd` | the verdict of every rule on one design. |

**`rules.xml` is the specification, not the documentation.** `SEMANTICS.md` says
normatively what a conforming implementation must compute, down to the degenerate
cases; the Java in `src/` is one implementation of it, and an optimized C++
checker reading the same `rules.xml` is another. The conformance suite,
`roomgraph.ConformanceTest`, decides which is which.

## Not every adjacency is a door

The graph's edges are **connections**, each a `DOOR` or an `OPENING`. A living
room usually gives onto its hall through a cased opening with no leaf in it, and
that is a different fact about the house: it cannot be closed, and a code that
requires a garage to be "equipped with" a door is not satisfied by an archway.
The kind is required on every connection — there is no sensible default, and
guessing would be a silent substitution that surfaces as a bug later.

`noEdge` therefore takes `via="ANY|DOOR|OPENING"`. Two rules use it:
`IRC-R302.5.1b` (CODE — the garage separation must be *equipped with* a door;
R302.5.1 requires a 1⅜-inch solid door, and an archway is not one) and `BATH-DOOR`
(PATTERN — a bathroom must be closable; the model code never says so in as many
words, because nobody has ever needed telling).

## The outdoors is not one place

`EXTERIOR` vertices are now plural and tagged: `STREET`, `DRIVEWAY`, `GARDEN`,
`TERRACE`, `COURT`, `OTHER`. A bedroom giving onto a private garden is a pleasure;
a bedroom giving onto the sidewalk is the fault pattern 130 is about, and with a
single exterior vertex those were the same graph. `APL-130b` now names the street.

This forced a correction elsewhere. The pattern-129 route rules measure the
*daily path*, and that starts at the front door — so they take `dist` from
`<outdoor kind="STREET"/>`, not from the outdoors at large. Measured from any
exterior, a back-garden door would put a bedroom one step from "outside" and every
route rule would collapse. `clean-plan.xml` has exactly such a garden door, and is
the regression test for it.

What several exteriors do **not** fix: two doors onto the same `STREET` still
share a vertex, so leaving by the front door and entering by the garage remains a
walk of length two. The `avoid=exterior` and `exclude=exterior` arguments in
`rules.xml` are what suppress that, and they are still needed.

## Three verdicts, not two

`SATISFIED`, `BROKEN`, `UNDETERMINED`.

A rule about stairs, applied to a design in which no stair has been placed, is not
satisfied — it has not been *asked*. Reporting it as satisfied would tell a search
that a constraint had been met when nothing whatever has been done about it. The
**empty design** — no rooms, no doors, not even the outdoors — is a legitimate
input and yields `UNDETERMINED` for all sixteen rules.

Undeterminedness is **declared, never inferred**. Each rule carries an optional
`<given>` presupposition; when it fails, the rule is undetermined and its
assertion is not evaluated:

```xml
<rule id="APL-133" severity="PATTERN" message="...">
  <given><nonempty><kind value="STAIR"/></nonempty></given>
  <forAll var="s"> ... </forAll>
</rule>
```

Whether a rule applies is a design decision, and design decisions belong in the
rule file where they can be read and argued with, not in an inference buried in an
evaluator. The alternative — three-valued Kleene logic propagated through every
connective — would make a rule's meaning depend on corners its author cannot see,
and every implementation would get them differently.

A consumer reads a **missing** rule as `UNDETERMINED`, so an implementation may
report only what it has something to say about, in any order. Conformance is a
statement about content, and `roomgraph.ReportCompare` is its operational
definition; the conformance suite uses it rather than `diff`.

## How the rule language avoids becoming a programming language

One decision: **XML may name a derived set, but may not compute one.**

Any rule needing a closure — who can reach what, which rooms are cut vertices,
which room is the centroid — names a *derived set*, and Java supplies the answer.
Membership is then a plain set membership, because the fixpoint has already been
taken.

| Derived set | Parameters | Algorithm |
|---|---|---|
| `cutVertices` | `exclude` | Hopcroft–Tarjan, over all components |
| `reachable` | `from`, `avoid` | breadth-first search |
| `monotoneReachable` | `from` | BFS with a non-decreasing-privacy edge filter |
| `centroid` | `among`, `over`, `avoid` | argmin of total distance; returns the whole tie set |

Around them: ordinary set algebra (`union`, `intersect`, `minus`, `complement`,
`neighbors`, `closedNeighborhood`), two attribute filters (`attrAtLeast`,
`attrAtMost`), two derived scalars (`dist`, `count`), one attribute reader
(`attrOf`), and **one binder** (`forAll`) over one variable. No joins.

Note the distinction between the filters and the reader. A room with no
`lengthFeet` is simply not a member of `attrAtLeast name="lengthFeet" value="30"`
— that is a question about membership. But `attrOf` *reads* the value, and a room
lacking it is an **error**, never a zero: a corridor of unknown length must not
quietly pass a length limit.

A rule is therefore a first-order formula with one bound variable: no recursion
operator, no fixpoint, nothing that can diverge. Every fixpoint lives in the four
derived sets, and every rule's cost is a small multiple of a graph traversal —
which is what makes the checker safe to call at every node of a search.

This is a **stratification, not a weakening.** The derived sets compute transitive
closure and aggregation, which is more than a first-order rule could say and more
than plain Datalog could say; the point is not that the design sits below some line
on a chart of expressive power, but that recursion and aggregation are confined to
a closed catalog of four algorithms whose cost and termination are known, with a
first-order logic layered over their results. SEMANTICS.md §8 states this precisely.

The catalog is **closed**: Java owns the list of algorithms and XML may only name
them, so a misspelled primitive is a load-time error. The escape hatch stays open:
`Check` is a public interface, and a rule that outgrows the language can be
written in Java and mixed into the same list.

## Presuppositions, and why partial designs make them visible

A rule written for a *complete* design silently assumes every room is reachable.
On a fragment that assumption is false, and a rule whose presupposition fails does
not return "false" — it returns nonsense. The first version of this rule set, run
on a plan with one unattached bedroom, reported four findings from one missing
door, two of which **contradicted each other**: that the route to the bedroom
never passes the common area, and that every route to it passes through one. Both
fired because both distances were infinite, and ∞ < ∞ is false either way.

The fix is to state each presupposition *in the rule*: every rule that reasons
about routes now quantifies over the exterior's connected component. Four findings
become one true one — `PLAN-CONNECTED` — and the other rules correctly say nothing.
Look for the `PRESUPPOSITION` comments in `rules.xml`.

This is the sort of defect that only a partial design exposes, and it is an
argument for checking fragments even if you only care about finished plans.

## The example plans

| Plan | What it exercises |
|---|---|
| `clean-plan.xml` | passes every rule. A rule set nothing can pass is a rule set nobody will keep. |
| `example-plan.xml` | nine broken rules, three of them CODE. Exit 1. |
| `corridor-plan.xml` | breaks the pattern rules while breaking no code rule. Exit 0. |
| `through-common-plan.xml` | the open-plan house whose living room everyone must cross. |
| `conformance/empty-design.xml` | nothing at all: `<plan name="Empty design"/>`. All sixteen rules undetermined. |
| `conformance/outdoors-only.xml` | the outdoors and nothing else. Also all sixteen undetermined. |
| `conformance/no-doors.xml` | rooms placed, nothing joined. |
| `conformance/unattached-bedroom.xml` | the four-contradictory-findings case, now one finding. |
| `conformance/ensuite.xml` | the bedroom-as-passage that APL-131a is meant to catch. |
| `conformance/open-bath-garage.xml` | archways where doors are required. Graph-identical to an ordinary house; only the connection kinds differ. |

The two failure modes of pattern 129 are deliberately separate rules. A plan can
*bypass* the common area (nobody passes it, so it dies) or run *through* it
(everybody crosses it, so nobody lingers), and either can fail without the other;
`corridor-plan.xml` and `through-common-plan.xml` are the controls. Tangency —
the case Alexander actually wants — is neither.

### The corridor hole, closed

An earlier version of APL-129-bypass could not tell "passes the living-room door"
from "walks past the living-room door down a fifty-eight-foot corridor", because
the hall neighbors the living room and so counted as tangency. It does now: the
rooms that provide tangency are the closed neighborhood of the common area **minus
anything longer than thirty feet**, using the new `attrAtLeast` set filter.

`corridor-plan.xml` consequently now trips the rule, and `example-plan.xml` flags
bedroom 2 as well as bedroom 1, since its route ran down a sixty-two-foot hall.
Thirty feet is a design parameter, not a code requirement, and it sits in the data
precisely so that it can be argued with: Alexander's fifty feet (pattern 132) is
where a passage stops reading as a room *at all*, and contact in passing needs a
shorter figure than that.

## Building and conforming

No dependencies beyond the JDK. Java 17 or later.

    javac --release 17 -d build $(find src test -name '*.java')
    java -cp build roomgraph.ConformanceTest

`roomgraph.ConformanceTest` runs every plan in `conformance/` through the Java
checker in process, compares each report against the expected one with
`roomgraph.ReportCompare`, and exits 0 when every case conforms and 1 otherwise.
In IntelliJ, run the **ConformanceTest** configuration.

To check an independent implementation — an optimized C++ checker, say — pass it
with `--cmd`. The suite runs the command over every plan as `<command> --plan
<plan> --rules <rules> --report <tmp>` and compares each report it writes, exactly
as for the Java checker. The command is split on whitespace and does not go
through a shell. Its exit status is ignored, since a conforming checker exits
nonzero when a CODE rule breaks:

    java -cp build roomgraph.ConformanceTest --cmd "./build/roomgraph-cpp"

For a single report already on disk, `roomgraph.ReportCompare` is the same
comparison one plan at a time. It exits 0 when the two agree and 1 when they do
not, ignoring the order of verdicts and findings:

    java -cp build roomgraph.ReportCompare conformance/example-plan.expected.xml actual.xml

**In IntelliJ**, mark **only** `src` as the Sources Root. If an enclosing folder is
also named `roomgraph` it collides with the package name and you will get spurious
"cannot resolve" errors on classes that plainly exist. If that has already
happened: File → Invalidate Caches, and check the language level is 17 or above.
**In NetBeans with Ant**, a stock Java Application project with `src` as the source
package folder needs no further configuration.

## Provenance

Each rule in `rules.xml` carries a comment naming its source. The code rules are
from the 2021 IRC and the NYC Building Code; the pattern rules are from Alexander,
Ishikawa and Silverstein, *A Pattern Language* (1977), patterns 127, 129, 130,
131, 132, 133 and 158. The older idea that a plan is a graph and the graph is the
design is in Alexander's *Notes on the Synthesis of Form* (1964) and in the
space-syntax tradition of Hillier and Hanson, whose *Space is the Machine* is open
access at <https://discovery.ucl.ac.uk/id/eprint/3881/>.
