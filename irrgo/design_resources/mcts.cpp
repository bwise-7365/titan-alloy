// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

#include "mcts.h"
#include <chrono>  // high resolution clock
#include <ostream>
#include <random>

// ----------------------------------------------
// global variable assignments go here, if any
// ----------------------------------------------
namespace MCTS {

  uint64_t Node::highestNodeID = 10000;
  uint64_t Position::highestPositionID = 10000;
  Searcher* Searcher::theSearcher = nullptr;

  Searcher::Searcher() {
    // enforce singleton
    assert(nullptr == theSearcher);
    theSearcher = this;
    setSeed(panj::PRNG::dSeed); // fixed default, unless they change it.
  }

  void Searcher::setSeed(W64 s) {
    using namespace std::chrono;
    
    W64 s2 = s;
    if (0 == s) {
      cout << "Truly random seed requested" << endl;

      microseconds ms = duration_cast<microseconds>(system_clock::now().time_since_epoch());
      s2 = ms.count(); // microseconds since the Unix Epoch
      s2 = panj::qTrans(panj::rotr(s2, 7));
      //cout << "seed #1: " << s2 << endl;

      // This does work:
      //std::random_device rd;
      //std::mt19937_64 mt1(rd());
      //std::uniform_int_distribution<uint64_t> dist(0, panj::MASK64);
      //s2 = dist(mt1);
      //cout << "seed #2: " << s2 << endl;
    }
    cout << "Seeding Searcher with " << s2 << endl << flush;
    prng.seed(s2);
    return;
  }

  void Searcher::setExpFactor(double ef) {
    assert(ef >= 0.0);
    expFactor = ef;
    return;
  }

  Searcher::~Searcher() {
    clear();
  }

  Move * Searcher::doSearch(Position * p, unsigned int nodeMin, unsigned int nodeMax, ReportingLevel rl) {
    assert(nodeMin > 0);
    assert (nodeMax > nodeMin);
    p->assertInteg();

    clear();
    // this deletes the root node (if any), and its position.
    // make sure it does not delete the main position of your ongoing game,
    // i.e. that the Position in the root node is NOT p.

    p->assertInteg();

    minNodes = nodeMin;
    maxNodes = nodeMax;
    if ((rl > ReportingLevel::Silent) && (nullptr == contP)) {
      printf("Starting search with budget  %u/%u\n", minNodes, maxNodes);
      cout << flush;
    }

    p->assertInteg();
    root = new Node(this, p, nullptr);
    p->assertInteg();

    assert(nullptr != root->searcher);
    assert(this == root->searcher);

    Node* rtrnNode = nullptr;

    // Build the list of moves, but without kids.
    // Hence, it still counts as a leaf node.
    root->setMoveList();

    // search at least the minimum number of nodes.
    if (rl > ReportingLevel::Silent) {
      cout << "Starting main search " << endl << endl << flush;
    }
    bool searchP = (minNodes > 0);
    while (searchP) {
      growTree(rl);
      auto vc = root->getVisitCount();
      if (nullptr == contP) {
        searchP = (minNodes > vc);
      }
      else {
        searchP = (contP() && (maxNodes > root->getVisitCount()));
      }
    }

    if (rl > ReportingLevel::Silent) {
      cout << "Finished main search." << endl;
      cout << "Root visit count: " << root->visitCount << endl;
      if (rl == ReportingLevel::Medium) {
        cout << "Final shape: " << endl;
        root->printShape(0, 2);
      }
      if (rl > ReportingLevel::Medium) {
        cout << "Final shape: " << endl;
        root->printShape(0, 3);
      }
      cout << endl;
    }

    // Having done the main search, we now do a quick search to get the robust child.

    if (rl > ReportingLevel::Silent) {
      cout << "Starting convergence search " << endl << endl << flush;
      cout << flush; // FIXME: place for breakpoint
    }
    searchP = true;
    while (searchP) {// search until either estimates converge or we run out of time.
      growTree(rl);
      Node* bc = root->bestChild(expFactor);
      Node* rc = root->robustChild(expFactor);
      bool matchP = (rc == bc);
      bool budgetP = (root->getVisitCount() > maxNodes);
      searchP = ((!matchP) && (!budgetP));
      if (rl > ReportingLevel::Silent) {
        if (matchP) {
          cout << "Robust child and best child are the same" << endl;
          cout << "Root visit count: " << root->visitCount << endl;
        }
        if (budgetP) {
          cout << "Search budget exhausted" << endl;
          cout << "Root visit count: " << root->visitCount << endl;
        }
      }
    }
    rtrnNode = root->robustChild(expFactor);

    if (rl > ReportingLevel::Silent) {
      cout << "Finished convergence search." << endl;
      if (rl == ReportingLevel::Medium) {
        cout << "Final shape: " << endl;
        root->printShape(0, 2);
      }
      if (rl > ReportingLevel::Medium) {
        cout << "Final shape: " << endl;
        root->printShape(0, 3);
      }
      cout << endl << flush;
    }

    Move* rtrnMove = rtrnNode->getIncoming();
    rtrnMove->assertInteg();

    if (rl > ReportingLevel::Silent) {
      int rNdx = -1;
      for (unsigned int i = 0; i < root->moves->size(); i++) {
        if (rtrnMove == root->moves->at(i)) {
          rNdx = i;
        }
      }
      cout << "Index of selected move was " << rNdx << " leading to " << rtrnNode->getNodeID() << endl;
    }
    rtrnMove->assertInteg();
    return rtrnMove;
  }

  void Searcher::growTree(ReportingLevel rl) {
    unsigned int vc = root->getVisitCount();
    if (rl > ReportingLevel::Low) {
      printf("Starting growTree iteration %u/%u/%u\n", vc, minNodes, maxNodes);
      if (rl > ReportingLevel::Medium) {
        root->print(0, 10000);
      }
    }

    // at the start of the first call to growTree,
    // the tree size is 1, with total visit 0.
    Node* v1 = root->treePolicy();
    Reward* delta = v1->rollout();
    v1->backup(delta);

    if (rl > ReportingLevel::Low) {
      printf("After growTree iteration %u/%u/%u\n", vc, minNodes, maxNodes);
      if (rl > ReportingLevel::Medium) {
        root->print(0, 10000);
      }
      cout << endl << flush;
    }
    return;
  }

  void Searcher::clear() {
    delete root; // root Node and everything in or below it
    root = nullptr;
    return;
  }

  // ----------------------------------------------
  Node::Node(Searcher* s, Position* p, Node* pn) {
    assert(nullptr != s);
    searcher = s;
    assert(nullptr != p);
    pstn = p;
    pstn->assertInteg();
    myNodeID = highestNodeID++;
    // NULL parent node is allowed at root
    parent = pn;
    //cout << "Created Node " << myNodeID << endl << flush;
    //cout << flush; // FIXME: place for breakpoint
  }


  Node::~Node() {
    deleteChildren();
    deleteMoves();
    searcher = nullptr;
    incomingMove = nullptr; // FIXME: already deleted elsewhere?
    delete pstn;
    pstn = nullptr;
    //cout << "Deconstructed Node " << myNodeID << endl << flush;
    //cout << flush; // FIXME: place for breakpoint
  }

  bool Node::terminal() {
    if (!gameOverTestedP) {
      gameOverP = pstn->gameOverP();
      gameOverTestedP = true;
    }
    return gameOverP;
  }

  bool Node::fullyExpanded() const {
    bool kidsP = (nullptr != children);
    unsigned int nue = kidsP ? numUnExpanded() : 0;
    bool fe = kidsP && (0 == nue);
    return fe;
  }

  Node* Node::treePolicy() {
    Node* v = this;
    bool tp = terminal(); // Is this node a terminal state of the game?
    if (!tp) {
      bool fe = fullyExpanded();
      if (!fe) {
        v = expand(); // child from some untried move at this node, which creates a new direct child of this node
      }
      else {
        v = bestChild(searcher->expFactor); // best direct child of this node, from this mover's perspective
        v = v->treePolicy(); // continue down until we reach a terminal or expandable node
      }
    }
    assert(nullptr != v);
    return v;
  }

  Node* Node::expand() {
    //cout << "Expanding node " << myNodeID << endl << flush;
    //cout << flush;
    assert(false == pstn->gameOverP()); // precondition to call this
    if (this == searcher->root) {
      assert(nullptr == incomingMove);
    }
    else {
      assert(nullptr != incomingMove);
    }
    setMoveList();
    unsigned int ki = randUnExp();
    Move* mi = (*moves)[ki];
    assert(nullptr != mi);
    mi->assertInteg();
    Position* pi = pstn->nextPosition(mi);
    Node* ni = new Node(searcher, pi, this);
    ni->incomingMove = mi;
    mi->assertInteg();
    (*children)[ki] = ni;
    return ni;
  }

  void Node::setMoveList() {
    if (nullptr == moves) {
      assert(nullptr != pstn);
      assert(nullptr != pstn->searcher);
      moves = pstn->legalMoves(pstn->getMover());
      unsigned int numMoves = moves->size();
      assert(numMoves > 0); // game continues, so there must be at least one move.
      assert(nullptr == children);
      children = new vector<Node*>(numMoves);
      for (unsigned int i = 0; i < numMoves; i++) {
        children->at(i) = nullptr;
      }
    }
    return;
  }

  unsigned int Node::numUnExpanded() const {
    assert(nullptr != moves);
    assert(nullptr != children);
    const unsigned int n = moves->size();
    const unsigned int m = children->size();
    assert(n == m);

    unsigned int childNodeCount = 0;
    for (unsigned int i = 0; i < m; i++) {
      const Node* k = (*children)[i]; // just to be really clear about what it is
      if (nullptr != k) {
        childNodeCount++;
      }
    }
    return (m - childNodeCount);
  }


  unsigned int Node::randUnExp() const {

    unsigned int numMoves = moves->size();
    assert(numMoves > 0); // game continues
    const unsigned int remKids = numUnExpanded();
    assert(remKids > 0); // precondition to call this

    // we have to choose randomly from among those moves which remain unexpanded,
    // so we build an array of them and choose from it.
    auto remMoves = vector<int>();
    remMoves.resize(remKids);
    unsigned int kidCounter = 0;
    for (unsigned int i = 0; i < numMoves; i++) {
      if (nullptr == (*children)[i]) {
        remMoves[kidCounter] = i;
        kidCounter++;
      }
    }

    unsigned int ki = remMoves[0]; // first move with no corresponding kid-node
    if (remKids > 1) {
      std::uniform_int_distribution<int> distribution(0, remKids - 1);
      auto k = distribution(searcher->prng);  // generates number in the range 0..n-1, inclusive
      ki = remMoves[k];
    }

    //cout << "Randomly selected move " << ki << " from " << numMoves << " unexpanded:";
    //for (unsigned int i = 0; i < remKids; i++) {
    //  cout << " " << remMoves[i];
    //}
    //cout << endl << flush;

    assert(nullptr == (*children)[ki]); // verify: chose unexpanded move

    return ki;
  }

  // most-visited direct child node.
  // Strictly speaking, it is a direct child such that
  // no other direct child has been visited more.
  // There may be other direct children with the same high visitCount,
  // so we prefer the one with the higher uctScore.
  Node* Node::robustChild(double ef) const {
    Node* bestKid = nullptr;
    unsigned int maxVC = 0;
    double maxS = -Infinity;
    assert(nullptr != children);
    const unsigned int n = children->size();
    for (unsigned int i = 0; i < n; i++) {
      Node* k = (*children)[i];
      if (nullptr != k) {
        unsigned int vc = k->visitCount;
        double s = uctScore(k, ef);
        if ((vc > maxVC) || ((vc == maxVC) && (s > maxS))) {
          maxVC = vc;
          maxS = s;
          bestKid = k;
        }

      }
    }
    assert(maxVC > 0);
    assert(maxS > -Infinity);
    assert(nullptr != bestKid); // precondition: has to have at least one (visited) child
    return bestKid;
  }


  // highest-scoring direct child node, from this node's perspective
  Node* Node::bestChild(double ef) const {
    Node* bestKid = nullptr;
    const double minScore = -Infinity;
    double maxScore = -Infinity;
    assert(nullptr != children);
    const unsigned int n = children->size();
    for (unsigned int i = 0; i < n; i++) {
      Node* k = (*children)[i];
      if (nullptr != k) {
        double s = uctScore(k, ef);
        if (s > maxScore) {
          maxScore = s;
          bestKid = k;
        }
      }
    }
    assert(nullptr != bestKid); // precondition: has to have at least one (visited) child
    assert(maxScore > minScore);
    return bestKid;
  }


  double Node::uctScore(Node* kid, double ef) const {
    assert(ef >= 0.0);        // upper confidence bound, not lower
    double myVC = (double)visitCount;
    double kidVC = (double)(kid->visitCount);
    assert(kidVC > 0);         // no division by zero? Or just provide tiny value, to ensure large uct score?
    assert(myVC >= kidVC); // total visits in parent >= visits in child

    double sr = 0.0;

    if (pstn->getMover() == searcher->root->pstn->getMover()) {
      sr = kid->simReward.rootReward;
    }
    else {
      sr = kid->simReward.oppReward;
    }

    double meanScore = sr / kidVC;

    // see MCTS-Exploration-Term-Notes
    double expTerm = sqrt((2.0 * log(myVC)) / kidVC);

    double score = meanScore + (ef*expTerm);
    return score;
  }


  unsigned int Node::treeSize() const {
    unsigned int count = 1;
    if (nullptr != children) {
      int nk0 = children->size();
      size_t nk1 = children->size();
      unsigned int nk2 = children->size();
      assert(nk0 == nk1);
      assert(nk1 == nk2);
      assert(nk2 == nk0);
      auto nk = children->size();
      for (unsigned int i = 0; i < nk; i++) {
        Node* ki = (*children)[i];
        if (nullptr != ki) {
          count += ki->treeSize();
        }
      }
    }
    return count;
  }

  // We require that defaultRollout do the staticEval from root's perspective,
  // therefore we must swap +/- signs after staticEval of
  // terminal node, as is required in a two-person, zero-sum game.
  void Node::backup(Reward* r) {
    visitCount += 1;
    simReward.rootReward += r->rootReward;
    simReward.oppReward += r->oppReward;
    if (nullptr != parent) {
      parent->backup(r);
    }
    else {
      delete r;
      r = nullptr;
    }
    return;
  }

  Reward* Node::rollout() const {
    assert(nullptr != pstn);
    //cout << "Start rollout at node " << myNodeID << " with position " << pstn->getPstnID() << endl << flush;
    Reward* r = pstn->defaultRollout();
    return r;
  }

  void Node::deleteMoves() {
    if (nullptr != moves) {
      const unsigned int n = moves->size();
      for (unsigned int i = 0; i < n; i++) {
        Move* mi = (*moves)[i];
        if (nullptr != mi) {
          delete mi;
          mi = nullptr;
        }
        (*moves)[i] = nullptr;
      }
      delete moves;
      moves = nullptr;
    }
    return;
  }

  void Node::deleteChildren() {
    if (nullptr != children) {
      const unsigned int n = children->size();
      for (unsigned int i = 0; i < n; i++) {
        Node* ci = (*children)[i];
        if (nullptr != ci) {
          delete ci;
          ci = nullptr;
        }
        (*children)[i] = nullptr;
      }
      delete children;
      children = nullptr;
    }
    return;
  }


  void Node::print(unsigned int depth, unsigned int maxDepth) {
    if (depth > maxDepth) {
      return;
    }
    char* spaces = new char[depth + 2];
    for (unsigned int i = 0; i < depth; i++) {
      spaces[i] = '-';
    }
    spaces[depth] = ' ';
    spaces[depth + 1] = 0;

    unsigned int nm = (nullptr == moves) ? 0 : moves->size();
    unsigned int nk = 0;
    if ((nm > 0) && (nullptr != children)) {
      for (unsigned int i = 0; i < nm; i++) {
        if (nullptr != children->at(i)) {
          nk = nk + 1;
        }
      }
    }
    double vc = (double)visitCount;
    double ucts = 0.0;
    if (nullptr != parent) {
      ucts = parent->uctScore(this, searcher->expFactor);
      cout << spaces << myNodeID << "  parent:      " << parent->getNodeID() << endl;
    }

    cout << spaces << myNodeID << "  tree size:   " << treeSize() << endl;
    cout << spaces << myNodeID << "  visit count: " << visitCount << endl;
    cout << spaces << myNodeID << "  rewards:     " << simReward << endl;
    cout << spaces << myNodeID << "  mean reward: " << (simReward.rootReward / vc) << ", " << (simReward.oppReward / vc) << endl;
    if (nullptr != parent) {
      cout << spaces << myNodeID << "  uct score:   " << ucts << endl;
    }
    cout << spaces << myNodeID << "  num moves:   " << nm << endl;
    cout << spaces << myNodeID << "  num children:" << nk << endl;
    cout << endl << flush;

    delete[] spaces;
    spaces = nullptr;

    if ((nm > 0) && (nullptr != children)) {
      for (unsigned int i = 0; i < nm; i++) {
        auto ki = children->at(i);
        if (nullptr != ki) {
          ki->print(1 + depth, maxDepth);
        }
      }
    }
    return;
  }

  void Node::printShape(unsigned int depth, unsigned int maxDepth) const {
    if (depth > maxDepth) {
      return;
    }
    for (unsigned int i = 0; i < depth; i++) {
      cout << "-";
    }
    cout << myNodeID << endl << flush;

    unsigned int nm = (nullptr == moves) ? 0 : moves->size();
    if ((nm > 0) && (nullptr != children)) {
      for (unsigned int i = 0; i < nm; i++) {
        auto ki = children->at(i);
        if (nullptr != ki) {
          ki->printShape(1 + depth, maxDepth);
        }
      }
    }
    return;
  }
  // ----------------------------------------------

  Position::Position(Searcher* s) {
    assert(nullptr != s);
    searcher = s;
    myPositionID = highestPositionID++;
    //cout << "Created Position " << myPositionID << endl << flush;
  }

  Position::~Position() {
    searcher = nullptr;
    onMove = nullptr;
    myPositionID = 0;
    deletedP = true;
    //cout << "Deconstructed Position " << myPositionID << endl << flush;
  }

  void Position::assertInteg() {
    assert(false == deletedP);
    assert(nullptr != searcher);
    assert(0 != myPositionID);
    assert(nullptr != onMove);
    return;
  }

  void Position::setMover(Player * m) {
    assert(nullptr != m);
    onMove = m;
    return;
  }

  Reward* Position::defaultRollout() const {
    Reward* r = nullptr;
    bool doneP = gameOverP();
    if (doneP) {
      // The plain vanilla MCTS-UCT algorithm assumes the rewards are in [-1,+1]
      double rootR = staticEval(searcher->root->pstn->onMove);
      assert(rootR >= -1.0);
      assert(1.0 >= rootR);

      r = new Reward(0.0, 0.0); // delete at root by Node::backup
      r->rootReward = rootR;
      r->oppReward = -rootR;
      // Note that we use ROOT mover, not last mover, and
      // 'backup' will swap +/- signs as necessary.
      // We require that backup considers 'reward' to be from root's perspective.
      //cout << "Ended rollout at position " << myPositionID << " with " << (*r) << endl << flush;
    }
    else {
      vector<Move*>* moves = legalMoves(onMove);
      assert(nullptr != moves); // game continues
      unsigned int n = moves->size();
      assert(n > 0); // game continues, right?
      unsigned int i = 0;
      if (n > 1) {
        std::uniform_int_distribution<int> distribution(0, n - 1);
        i = distribution(searcher->prng);  // generates number in the range 0..n-1, inclusive
      }

      Move* mi = (*moves)[i];
      Position* pi = nextPosition(mi);
      r = pi->defaultRollout();

      delete pi;
      pi = nullptr;
      for (unsigned int i = 0; i < n; i++) {
        Move* m = (*moves)[i];
        delete m;
        m = nullptr;
        (*moves)[i] = nullptr;
      }
      delete moves;
      moves = nullptr;
    }
    return r;
  }

  // ----------------------------------------------
  Player::Player(string n) {
    assert(n.size() > 0);
    name = n;
  }
  Player::~Player() {
    // nothing yet
  }

  // ----------------------------------------------
  Move::Move(Player* p) {
    assert(nullptr != p);
    player = p;
  }

  Move::~Move() {
    player = nullptr;
    deletedP = true;
  }

  void Move::assertInteg() {
    assert(nullptr != player);
    assert(false == deletedP);
    return;
  }
} // namespace MCTS




ostream& operator << (ostream& s, MCTS::Reward r) {
  s << "[Reward " << r.rootReward << ", " << r.oppReward << "]";
  return s;
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
