// ---------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ---------------------------------------------
// This provides headers for two person,
// constant sum, perfect information games.
//
// In these games, the two Players take
// turns making Moves. This includes the simple
// moves like 'place stone' or 'move piece',
// as well as 'pass' (in Go) or 'switch colors'
// (in Hex). In Hex, the players always alternate,
// while the colors may not.
// In Gomoku, B and W take turns placing stones anywhere on
// a square 15x15 or 19x19 grid: first to get 5 in a row
// horizontally, vertically, or diagonally wins. 
//
// Connect-6 is like Gomoku, except that the goal is to
// get 6 in a row. The first player (B) places just one stone,
// and thereafter a move is to place two stones at once.
// Again, no captures. Often played on 19x19, but larger sizes
// have been proposed, like 23x23 or 57x57 (i.e. 3x3 array of Go boards).
//
// As of April 2018, the only game tree search is plain negaMax.
// That is reasonable for basic Mancala, but not
// for Irgo on non-trivial boards.
//
// For both Hex and Gomoku, adding
// more pieces cannot change the outcome, so simulating
// random games is easy: shuffle the empty positions
// and mark them in alternation.
// I think this also applies for Go with Chinese rules,
// as long as it is restricted to legal moves (and possible
// repeat moves when allowed by ko rule).
// ----------------------------------

#ifndef GAME_HEADER
#define GAME_HEADER

// ----------------------------------
// insert basic includes here

#include <iostream>
#include <vector>

namespace TPCSPIGame {


  // set this to be PAST the extreme +/- limit of your staticEval function
  const double Infinity;
  // ----------------------------------
  // forward declare classes, structs, etc. here

  class Game; // this holds the search tree from the current position
  class Board; // this is completely static and stateless in a game
  class Position; // this changes over time
  class Player; //
  class Move;


  // ----------------------------------
  // declare functions and structs here


  // ----------------------------------
  // full class declarations go here

  class Game {


    // negaMax is intially called with depth=0, alpha= - Infinity, and beta= + Infinity
    // it will go out to TPCSPIGame::MaxSearchDepth
    double negaMax(Position* p1, unsigned int depth, Player* player, double alpha, double beta);

    // These are only reliable when negaMax search is completed. They
    // only apply to the player and position that started the search.
    //
    Move* BestMove = nullptr;  // define the global var TPCSPIGame::BestMove
    double BestValue = 0.0;  // define the global var TPCSPIGame::BestValue


    unsigned int MaxSearchDepth = 2; // shallow default

  }

  class CountedItem {
  public:
    CountedItem() {idNum = HighestID++; };
   virtual ~CountedItem() {};
    unsigned int getID() { return idNum; };
  protected:
    unsigned int idNum = 0;
  private:
    static unsigned int HighestID;
  };



  class Board : public CountedItem {
  public:
    Board();
    ~Board();
  protected:
  private:
  };





  class Position : public CountedItem {
  public:
      Position() = delete; // no arg-less constructor
      Position(const Position&) = delete; // no copy constructor
      Position& operator=(const Position&) = delete; // no copy assignment
    Position(Board* b);
    virtual ~Position();



    // returns the value of this board to color
    // this color must be the one who can move, so we are
    // choosing his move to maximize his utility
    virtual double staticEval(Player*) =  0;

    virtual void doMove(Move*) =  0; // updates this position with the given Move
    virtual Position* nextPosition(Move*) = 0; // generates a whole new position with this as parent
    virtual bool legalMoveP(Move*) = 0;
    virtual bool gameOverP(Player*) = 0;
    virtual vector<Move*>* legalMoves(Player*) = 0;


    // Suppose PosX is derived type of Position.
    // Because Position::parent is protected, a PosX-type
    // object can only access its own parent data member,
    // not the parents of other PosX-type objects.
    // This makes it impossible to do a move by
    // spawning a child position and setting its
    // parent to oneself.
    // Hence the need for the access functions below.
    //
    Position* getParent() { return parent;} ;
    void setParent(Position* p) { parent = p; return;} ;

    Player* getOnMove() { return onMove; };
    void setOnMove(Player* p) { onMove = p; return;};

    Player* getOffMove() { return offMove; };
    void setOffMove(Player* p) { offMove = p; return;};

    Board* getBoard() { return board; };
    void setBoard(Board* b) { board = b; return; };

  protected:
    Position* parent = nullptr;
    Board* board = nullptr;


    Player* onMove = nullptr; // whose turn it is to move in this position
    Player* offMove= nullptr; // whose turn it will be next 


  private:
  };



  class Player : public CountedItem {
  public:
    Player();
    ~Player();
  protected:
  private:
  };



  class Move : public CountedItem {
  public:
    Move(Player* p); // player can not be NULL
    Move() = delete; // no arg-less constructor
    Move(const Move&) = delete; // no copy constructor
    Move& operator=(const Move&) = delete; // no copy assignment
    ~Move();
    Player* getPlayer() { return player; };
    void setPlayer(Player* p) { player = p; return; };
  protected:
    Player* player = nullptr;
  private:
  };

}; // end of TPCSPIGame namespace

// ----------------------------------

#endif
// ---------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ---------------------------------------------
