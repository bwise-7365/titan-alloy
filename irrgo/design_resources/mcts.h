// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------

#ifndef MCTS_H
#define MCTS_H

#include <assert.h>
#include <iostream>
#include <math.h>
#include <ostream>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abzar.h"

using std::ostream;

using panj::ReportingLevel;
// ----------------------------------------------

/*
  My target games are the following.

  Mancala
  Setup: each side has a row on N pits, each with M stones, then an empty kalah at the
  right end of their row.
  End: If no stones on your side when your turn begins,
  the  game is over the most stones (pits+kalah) wins. Note that your side can be
  empty at the end of your turn, then partially refilled by the opponent, then
  you can still play. Theoretically, if you have enough stones in the kalah,
  you can run out of stones in the pits and still win.

  Play: Pick stones from own pit and drop counter-clockwise (skipping opponent kalah).
  If your last stone lands in an empty pit on your side, capture: your own stone
  and the opposing pit's stones all go in your own kalah.

  Crucially, we will include run-on moves: if your last stone goes in your own
  kalah, you have the option to pass or to move again. (hence, if the pits
  on your own side are empty, you can pass and the game continues).


  Gomoku:
  Setup: empty 19x19 board.
  End: Whoever gets 4 (or 5) in a row first wins, horizontal, vertical, or diagonal.
  Getting 6 or more does not win.
  Play: Alternately place stones on 19x19 until either no vacant spaces remain
  or someone gets 4 in a row. First mover is said to have an advantage,
  so one version is that Black places a stone, then White can either
  switch that one stone to white or place a regular white stone.
  After that, Black moves and regular alternating placement continues.
  Note that random moves after victory have no effect, so random playout is easy:
  shuffle the empty spaces and fill in order.

  Gomoku with Dagger:
  Similar to gomoku, but trying to negate first-mover advantage.
  End: Whoever gets 4 (or 5) in a row first wins, horizontal, vertical, or diagonal.
  Getting 6 or more does not win.
  Play: First move by Black is one stone anywhere. White starts with posession of the dagger,
  which is the right to place 2 stones at once. He may either make two moves right away
  and pass it to Black, or make one move and keep the dagger to use later.
  (The dagger may never be used to win: the winning move must be a 1-stone placement.
  After a player uses the dagger, it passes to the other player, who may not use it right away.
  If B just received the dagger, then B must make at least one 1-stone placement before using it -
  unless using the dagger immediately is the only way to avoid a loss on W's next move.
  Note that W's first move can use the dagger because it was not "just received".)


  Connect6:
  Setup: empty 19x19, 23x23, or even 57x57 board.
  End: Whoever gets six in a row first wins.
  Play: Alternately places stones on empty points. On the first move,
  Black can place one stone anywhere. After that, players place two
  stones at once.
  Again, random moves after victory have no effect.


*/
// ----------------------------------------------

namespace MCTS {
  using panj::PanjException;
  using panj::W64;

  // set this to be PAST the extreme +/- limit of your staticEval function
  const double Infinity = +1.0E20;

  class Move;
  class Node;
  class Player;
  class Position;

  struct Reward {
    Reward(double rootR, double oppR) {
      rootReward = rootR;
      oppReward = oppR;
    }
    double rootReward = 0.0;
    double oppReward = 0.0;
  };

  // Top-level move selection is to search at least a minimum number of nodes,
  // then continue until the most-visited child and highest-scoring child are the same,
  // then return it. If they don't match soon enough, return the highest-scoring child.
  //
  // The nodeMax is always applied. The reason is that near the end of a game,
  // where there are few moves and short rollouts, it is easy to visit the same
  // dozen endstates hundreds of thousands of times. Thus, it might be allocated
  // a second, but only use 0.01 if the search is trivial.


  // MCTS::Searcher starts at root node and finds good moves
  class Searcher {
    using stopFN = bool(*)();
    friend class Node; // has to see 'root'
    friend class Position; // has to see 'root'

  public:
    Searcher();
    Searcher(const Searcher& m) = delete; // copy constructor
    Searcher& operator=(const Searcher& m) = delete; // copy assignment
    virtual ~Searcher();

    Move* doSearch(Position* p, unsigned int nodeMin, unsigned int nodeMax, ReportingLevel rl);

    void clear(); // wipe out root node and all below it
    void setSeed(W64 s);
    void setExpFactor(double ef);

    // set this function to use external stopping criteria.
    // If not set, it will use the default stopping rule: node counts.
    stopFN contP = nullptr; // return true iff search should continue
    unsigned int numNodes() const {
      return nodeCount;
    };

  protected:
    void growTree(ReportingLevel rl);

    Node* root = nullptr;
    std::mt19937_64 prng = std::mt19937_64();
    double expFactor = 5.0; // random default

  private:

    //
    unsigned int nodeCount = 0;

    // Search at least this many nodes before doing robust-max comparison.
    // If you exhaust the tree before reaching this limit, select root's highest-scoring child
    // (likely outcome in tic-tac-toe).
    unsigned int minNodes = (unsigned int) 1E4;


    // Maximum computational budget.
    // The search usually terminates quite quickly after minNodes is reached,
    // so this is just a backup, hard limit in case that does not happen.
    // If you reach this limit before exhausting the tree, select root's robust child
    // (likely outcome in gomoku)
    unsigned int maxNodes = (unsigned int) 1E6;

    static Searcher* theSearcher; // enforce singleton
  };


  class Node {
    friend class Position;
    friend class Searcher;

  public:
    Node(Searcher* s, Position* p, Node* pn);
    Node(const Node& m) = delete; // copy constructor
    Node& operator=(const Node& m) = delete; // copy assignment

    // Delete this Node, along with its moves and all of its child Nodes.
    virtual ~Node();

    unsigned int getVisitCount() const {
      return visitCount;
    }
    Move*  getIncoming() const {
      return incomingMove;
    }


    Node* treePolicy();  // find a node to rollout, either leaf or interior
    Node* tp2();

    Node* robustChild(double expFactor) const; // most-visited direct child node (ties broken by uctScore
    Node* bestChild(double expFactor) const;   // highest-scoring direct child node, using this node (e.g. the parent) mover's reward
    unsigned int treeSize() const;
    Reward* rollout() const;
    void backup(Reward* r); // assumes negamax

    void print(unsigned int depth, unsigned int maxDepth);       // with useful data
    void printShape(unsigned int depth, unsigned int maxDepth) const;  // nothing but ID

    uint64_t getNodeID() const {
      return myNodeID;
    }

  protected:
    uint64_t myNodeID = 0;
    static uint64_t highestNodeID;

    bool terminal();  // Is this node a terminal state of the game?
    bool fullyExpanded() const; // Have all legal moves have been expanded into Nodes?
    void setMoveList();
    unsigned int numUnExpanded() const; // how many Moves not yet expanded into a child Node (0 == fully expanded)
    Node* expand();       // pick an untried move,

    double uctScore(Node* kid, double ef) const; // using this node (e.g. the parent) mover's reward

    Node* parent = nullptr;
    Move* incomingMove = nullptr;      // move that led from parent to here
    Position* pstn = nullptr;          // current game state
    vector<Move*>* moves = nullptr;    // list of legal moves, if any
    vector<Node*>* children = nullptr;  // for each move, nullptr if it has NOT been tried

    unsigned int visitCount = 0; // total visits at or below this node

    // sum of rewards to (root, opp) players from simulated policy rollouts
    Reward simReward = Reward(0.0, 0.0);
    Searcher* searcher = nullptr;

    bool gameOverTestedP = false; // not yet
    bool gameOverP = false; //

    unsigned int randUnExp() const; // random index to unexpanded move

  private:
    void deleteMoves();
    void deleteChildren();
  };


  // Summary of the information necessary to play or score.
  // For example, in Go this would have to include not only
  // the current board position but the previous ones:
  // the ko-rule for determining legal moves depends on entire history.
  // Often, there are "off board" resources, captured pieces, etc.
  // (like the dagger in "Dagger GoMoku") which are part of the Position.
  class Position {
    friend class Node; // FIXME: temporary?
  public:
    Position(Searcher* s);
    Position(const Position& m) = delete; // copy constructor
    Position& operator=(const Position& m) = delete; // copy assignment
    virtual ~Position();


    // Returns the value of this board to this player/color.
    // This color must be the one who can move, so we are
    // choosing his move to maximize his utility
    virtual double staticEval(Player*) const = 0;
    // Suppose we have point-based scoring, like Go or Kalah.
    // A natural final eval for point-difference s is (Ns)/(1+N|s|)
    // So if N=2, then winning by +1 get 2/3, tie gets 0, losing by 1 get -2/3.
    // It's a risk-averse utility function.
    // What would N have to be so the winning by +1 for certain was just as desirable
    // as winning by +S with probability p but losing by 1 with probability (1-p)?
    // A little algebra shows N = (p(S+1)-2) / (2(1-p)S).
    // So if p=0.9 and S=5, we get N=3.8
    // Several plausible p & S value give N of about 4, so it's recommended.
    //
    // For real-valued payoffs, s = (x - mean)/stdv, then (Ns)/(1+N|s|).
    // Of course, s = +5 standard deviations is quite unlikely, so N would have to be
    // set by plausible p & S values. E.g p=0.95, S=2 => N = 4.25

    virtual void doMove(Move*) = 0; // modifies this position
    virtual Position* nextPosition(Move*) const = 0; // generates a whole new position with this as parent
    virtual bool legalMoveP(Move*) const = 0;
    virtual bool gameOverP() const = 0; // whether or not it is over often depends on who is onMove in this Position.
    virtual vector<Move*>* legalMoves(const Player*) const = 0;

    Player* getMover() const {
      return onMove;
    }
    void setMover(Player* m);

    // Note that we rollout positions, not nodes.
    // The reason is that the positions of a rollout are not
    // saved, only the final reward is used.
    // if the game is over, return value to root.
    // if the game is not over, pick a random move from this state,
    // apply, and return the defaultRollout from that.
    Reward* defaultRollout() const;

    uint64_t getPstnID() const {
      return myPositionID;
    }

    // FIXME: tracking down a "use after delete" bug
    virtual void assertInteg();
    bool deletedP = false;

  protected:
    uint64_t myPositionID = 0;
    static uint64_t highestPositionID;
    Searcher* searcher = nullptr;
    Player* onMove = nullptr; // whose turn it is to move in this position, which might not alternate.

  private:
  };



  // ----------------------------------------------
  // Generic Player and Move are quite simple.

  class Player {
  public:
    Player(string n);
    virtual ~Player();
    string getName() const {
      return name;
    }
  protected:
    string name = "";
  private:
  };

  class Move {
  public:
    Move(Player* p);
    Move(const Move& m) = delete; // no copy constructor
    Move& operator=(const Move& m) = delete; // no copy assignment
    virtual ~Move();

    virtual void assertInteg(); // FIXME: tracking down 'use after delete' error

    Player* getPlayer() const {
      return player;
    };
    void setPlayer(Player* p) {
      assert(nullptr != p);
      player = p;
      return;
    };

  protected:
    Player * player = nullptr;
  private:
    bool deletedP = false;
  };



} // end namespace MCTS

// notice that these are the std << operator, and so must
// be declared outside the Kalah namespace
//
ostream& operator << (ostream& s, MCTS::Reward r);

// ----------------------------------------------
#endif  //  MCTS_H
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
