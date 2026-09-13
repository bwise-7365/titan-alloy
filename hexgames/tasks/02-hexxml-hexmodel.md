Copyright Ben Paul Wise. All Rights Reserved.

# Task 02: hexxml + hexmodel + hexrules loaders (milestone M3)

status: done   (reviewed and accepted by the coordinator 2026-09-13; side question raised as an XSD proposal in PLAN.md)
worker: W2 (sonnet)          started: 2026-09-13          finished: 2026-09-13
resume: DONE. hexxml (facade + 5 doc models), hexmodel (Quantities/Board/Roster/Position + the three
  builders) and hexrules (RuleSetBuilder, Ledger, PackageLoader) all implemented and tested.
  Full repo build is green: tools\build-dev.cmd win-msvc-debug -> 90/90 tests pass (core 67, hygiene
  1, package 2, records 15 [W3's hexrecord], xsd 5), hygiene_banner_check clean across all 149
  banner-checked files. See the log below for API additions, the trc.package.xml fix, and open
  questions for the coordinator/Ben.
inputs:
  hexgames/hexxml/                 (empty: you create the facade and the five document models)
  hexgames/hexmodel/Ids.h, Quantities.h, Board.h, Roster.h, Position.h, CMakeLists.txt   (contract)
  hexgames/hexrules/RuleSet.h, Ledger.h, Package.h, CMakeLists.txt                       (contract)
  hexgames/hexcoord/Grid.h, HexAddress.h, Abc.h, Direction.h   (done; use Grid for ids and lattice)
  hexgames/doc/2026-09-12-design/test-lists/hexmodel-hexsearch-tests.md  (hexmodel part)
  hexgames/doc/2026-09-12-design/test-lists/hexrules-hexengine-tests.md  (hexrules part)
  hexgames/uml/hexmodel-classes.puml, seq-load-package.puml
  hexgames/game_rules/xml/hexrules.xsd + README.md; map_graphics/xml/hexsheet.xsd + README.md;
    unit_graphics/xml/hexcounters.xsd + README.md; packages/xml/hexpackage.xsd; game_records/xml/hexsave.xsd
  the four instance documents of each language (read, never edit)
  hexgames/packages/xml/trc.package.xml, game_records/xml/trc-test.xml
  hexgames/CLAUDE.md, .clang-format, cmake/HexGamesGtest.cmake
outputs:
  hexgames/hexxml/CMakeLists.txt (STATIC, PRIVATE tinyxml2::tinyxml2), XmlDocument.{h,cpp} (facade),
    RulesDoc.{h,cpp}, SheetDoc.{h,cpp}, CounterSetDoc.{h,cpp}, PackageDoc.{h,cpp}, SaveDoc.{h,cpp}, test/
  hexgames/hexmodel/*.cpp (Quantities, Board, Roster, Position) + BoardBuilder/RosterBuilder/PositionBuilder
    headers (.h) as the contract's friend classes; CMakeLists STATIC; test/
  hexgames/hexrules/*.cpp (RuleSetBuilder, Ledger checkLedger, PackageLoader); CMakeLists STATIC; test/
  packages/xml/trc.package.xml corrections ONLY if a binding is provably wrong (note them here)
acceptance:
  tools\build-dev.cmd win-msvc-debug -> ctest 100%; new suites carry labels core / package
  hygiene_banner_check green; no file outside hexxml/, hexmodel/, hexrules/ (and this file) touched
  every declaration in the contract headers is defined; API changes listed here
  the hexxml facade is delivered FIRST (W3 codes against its API in parallel): see the brief
log:
- 2026-09-13 created by the coordinator
- 2026-09-13 W2: hexxml facade delivered first (XmlDocument.h/.cpp, CMakeLists STATIC linking
  tinyxml2::tinyxml2 PRIVATE, PUBLIC include dir ../..), with hexxml/test/XmlDocumentTest.cpp (4
  cases: attribute/child reading, missing-attribute throw naming file:line:element:attr, malformed
  bool throw, parse-error throw). Full build/test green (39/39, hygiene_banner_check passed).
  requiredAs<T>/optionalAs<T> implemented via explicit instantiation for int, double, bool,
  std::string, std::uint64_t so the header stays free of <tinyxml2.h>. One API note: XmlNode's
  single-arg private constructor could not be called from std::vector::emplace_back (allocator
  construct has no access even with `friend class XmlDocument`), so children()/children(name) build
  the vector with push_back(XmlNode(...)) instead (constructed inside XmlNode's own member function,
  then moved via the implicit public move constructor) -- no header API change.
- 2026-09-13 W2: RulesDoc.{h,cpp} + SheetDoc.{h,cpp} delivered with tests against the real TRC/Dai
  Senso/Tarawa rule docs and all four sheet docs, plus synthetic malformed fixtures (missing id, bad
  table arity, bad enum). Design note: every hexxml doc struct is prefixed by its document (Sheet*;
  RulesDoc's own names are left bare since it was written first) to avoid cross-header name
  collisions inside namespace HexXml (hexrules and hexsheet both have a "region", for instance).
  RulesDoc's annex (rule/note/list/table) is collected two ways: structurally where a container has
  its own annex children (a resolver's CRT table lives in ResolverDoc.tables), and flattened across
  the whole document into doc.allRules/allLists/allTables/allNotes regardless of nesting depth, per
  the brief's "every rule anywhere collected". SheetDoc keeps the mixed-content
  hexes/hex/edge/path/link/region/label/panel choice as eight same-kind vectors, each in document
  order, rather than one cross-kind ordered variant list -- nothing in BoardBuilder needs the
  cross-kind interleaving (bulk/per-hex terrain, edges, links and regions write disjoint Board
  state), so the simpler shape was chosen; flagging here in case a consumer needs true document
  order across kinds. Full build/test green (47/47).
- 2026-09-13 W2: CounterSetDoc.{h,cpp}, PackageDoc.{h,cpp}, SaveDoc.{h,cpp} delivered (hexcounters,
  hexpackage, hexsave), each tested against its real TRC document (214 counters incl. family
  breakdown 197/3/2/12 unit/support/leader/marker; trc.package.xml bindings incl. the two-counter
  leader binding; trc-test.xml scenario incl. the OMB-space unit) plus a synthetic bad-enum fixture
  for CounterSetDoc. hexxml is now feature-complete per the brief: XmlDocument facade + all five
  document models, 52 test cases, all green. NOTE: `ctest` currently reports one unrelated failure,
  hygiene_banner_check, caused by hexrecord/test/{CanonicalOrderTest,WriteRecordTest}.cpp missing
  banners -- those are W3's in-flight files, outside my hexxml/hexmodel/hexrules/tasks scope, not
  touched or caused by this work; every file this task owns is banner-clean. Moving on to hexmodel.
- 2026-09-13 W2: hexmodel and hexrules complete. Full build/test green, 90/90. Summary below;
  everything is additive/logged per the acceptance criteria ("API changes listed here").

  ARCHITECTURE NOTE (biggest structural decision): BoardBuilder, RosterBuilder and PositionBuilder
  must be declared in namespace HexModel (Board.h/Roster.h/Position.h each say `friend class
  XxxBuilder;` unqualified, which binds to HexModel::XxxBuilder), so their headers live in
  hexmodel/ as the brief says. But BoardBuilder needs a RuleSet parameter and RosterBuilder/
  PositionBuilder need a RuleSet too (unit-type/side/phase/network/layer/space/track lookups all
  live on RuleSet), and hexmodel must not depend on hexrules (hexrules already depends on hexmodel;
  the other way round would be a cycle). Resolution: the three headers only forward-declare
  `namespace HexRules { class RuleSet; }`; their .cpp files (which need the complete type) are
  still physically stored under hexmodel/ but are compiled into the hexrules CMake target
  (hexrules/CMakeLists.txt lists ../hexmodel/BoardBuilder.cpp etc. as sources) -- hexrules already
  links hexmodel (PUBLIC, so hexxml comes along too), so every header those three .cpp files need
  is visible with no link cycle. Friendship is a language concept independent of which library
  archives the resulting .obj, so Board/Roster/Position's private members stay reachable correctly.

  API ADDITIONS (all additive; no existing declaration removed or changed):
  - Board.h: networkId/layerId/regionId/spaceId/trackOf/trackId (name -> dense id lookups, the
    Board-side mirror of RuleSet's own side()/unitType()/terrain() lookups, needed by
    PositionBuilder and PackageLoader to resolve a save/scenario document's plain-string ids) and
    linkCount()/regionCount()/networkCount()/layerCount()/trackCount() (sizes PositionBuilder needs
    to pre-size Position's per-network/per-layer/per-track vectors). Backing private members added
    (byCentre_, byId_, networkByName_, layerByName_, regionByNamePerLayer_, spaceByName_,
    trackOfSpace_, trackCount_) -- implementation detail, not part of the public surface otherwise.
  - Roster.h: private byCounter_ map backing find(CounterId).
  - Position.h: private removeFromStack/addToStack helpers (need friend access to the per-hex/
    per-space stacks, so they are members rather than free functions in Position.cpp).
  - RuleSet.h: RegionLayerSpec gained regionNames (display names BoardBuilder could not otherwise
    recover, since a sheet <region> only carries free text, not a rules region id) and regionSides
    (a region's own @side, used by RosterBuilder -- see the open question below). Private
    phaseByName_ map added: PhaseNode keeps only id + display name, not the document's own id
    string, yet RuleSet::phase(const std::string&) must resolve exactly that string (mode/@phase,
    rule/@phase, modifier/@phase all reference phases by that id), so the builder-side name table
    is retained on RuleSet itself rather than only living transiently in the builder.
  - RuleSet.cpp did not exist before this task: side()/unitType()/terrain()/edgeTerrain()/phase(),
    HostilityMatrix::enemiesOf/hostileP and Table::cell were declared in the contract but had no
    definition anywhere; they are implemented here (linear search over the small vectors, which the
    house style prefers over adding more lookup maps than necessary).

  TRC.PACKAGE.XML FIX (a provably wrong binding, permitted by this task's outputs list): all six
  unit-type match regexes anchored on the literal suffix (e.g. `-infantry$`), so a counter that is
  a second physical printing of the same historical unit -- 26 of them, e.g.
  "r-ru-3-infantry-2", "g-lu-stuka-stuka-3", "r-wo-1-worker-3" -- failed to bind at all. Verified
  against the real 214-counter document; every pattern now accepts an optional trailing "-N" via
  `(-[0-9]+)?` before the `$` anchor. PackageTest.TrcLoads confirms all 202 non-marker counters
  (214 - 12 markers) now build into the Roster and PackageLoader::check() reports zero problems.

  OPEN QUESTION for the coordinator/Ben -- RosterBuilder cannot always determine UnitSpec::side:
  a unit-type's own @side singles out most counters already (luftwaffe/ss/panzer-grenadier =
  axis-only, paratroop/guards/worker = russian-only, stuka = axis, sturmovik = russian), but the
  shared types (infantry, armour, motorized, cavalry, mountain, hq, leader) are the very same
  rules unit-type on every side, so the individual counter's side is carried only by its id's own
  naming convention (TRC: a "g-"/"r-" nationality prefix) -- a per-game fact that hexrules.xsd,
  hexpackage.xsd (the <unit> binding has type/counters/match only, nothing for side) and
  PackageLoader::load's fixed (manifest, ValueLineReader) signature have nowhere to carry. Neither
  the XSD (a review gate per CLAUDE.md) nor Package.h's signature (a contract header) were changed
  to avoid pre-empting that review. RosterBuilder instead: (1) uses the unit-type's own singleton
  side mask when there is one; (2) else looks for a region, in any region layer, whose id or name
  equals the counter's style (TRC's "countries" layer marks Finland/Hungary/Rumania/Russia this
  way -- RegionLayerSpec::regionSides, added above, carries this); (3) else assigns the rules'
  first side as an explicit, commented, known-wrong placeholder -- never a throw, since no test in
  either accepted list checks UnitSpec::side, but flagged here because it is not a correct answer.
  A real fix most likely wants a small, reviewed hexpackage.xsd addition (a side attribute on
  <unit>, or a new binding kind).

  OTHER NOTED SIMPLIFICATIONS (none affect an accepted test):
  - RosterBuilder's back-face handling: a literal back <value> or derived="same" is honoured;
    derived="reduced"/"concealed" and ref-based backs are left as a nullopt Strengths (recomputing
    a reduced value needs a game-specific step-value table this milestone does not have).
  - A counter with no printed <value> at all (Stuka/Sturmovik support counters, which show only
    text) gets an all-nullopt Strengths rather than a thrown error.
  - PackageLoader::check()'s scenario-hex-existence check reads the sheet document's own listed hex
    ids directly rather than requiring a fully-built Board, so an unrelated sheet-terrain-binding
    failure elsewhere in the same package cannot hide an independent scenario problem -- check()'s
    whole point is collecting every independent problem in one pass.
  - hexrules/test fixtures package-broken.xml + rules/sheet/counters/scenario-broken.xml exercise
    PackageTest.BindingProblemsAreNamed's four planted, independent problems (unknown sheet
    terrain, unbound counter, space bound to a missing panel, scenario hex that does not exist).
  - BoardTest/PackageTest's "four games" cases (BoardTest.EveryPackageAvailableBuilds,
    PackageTest.FourPackagesCheckClean-equivalent) iterate whatever package manifests exist under
    packages/xml/ today -- only trc.package.xml -- rather than fabricating Dai Senso/Tarawa/PGG
    manifests, which is a separate, larger task not in this one's outputs; they will automatically
    start covering the other three games once those manifests are authored.
  - Dense-id minting, for the record: RuleSetBuilder mints SideId/UnitTypeId/TerrainId/
    EdgeTerrainId/PhaseId/NetworkId/LayerId/RegionId(per layer)/SpaceId/RandomizerId in each list's
    document order; BoardBuilder mints TrackId only over the subset of RuleSet spaces whose kind is
    "track", in that relative order (Position::tracks_ holds only those, not one slot per space).

  Every declaration in the five contract headers (Ids.h, Quantities.h, Board.h, Roster.h,
  Position.h, RuleSet.h, Ledger.h, Package.h) is now defined. No file outside hexxml/, hexmodel/,
  hexrules/, packages/xml/trc.package.xml and this task file was touched.

Copyright Ben Paul Wise. All Rights Reserved.
