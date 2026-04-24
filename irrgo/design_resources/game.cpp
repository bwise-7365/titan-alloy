// ---------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ---------------------------------------------

// file includes go here

#include <vector>

#include "game.h"

using std::vector;

namespace TPCSPIGame {
  // ----------------------------------

  // global variable assignments go here

  unsigned int CountedItem::HighestID = 1000;

  // ----------------------------------

  // functions and methods go here

  // ----------------------------------

  Board::Board() : CountedItem() {
    // do nothing
  }


  Board::~Board() {
    // do nothing
  }

  // ----------------------------------

  Position::Position(Board* b) : CountedItem() {
    parent = nullptr;
    assert (nullptr != b);
    board = b;
    onMove = nullptr;
    offMove = nullptr;
  }


  Position::~Position() {
    // do nothing
  }

  // ----------------------------------


  void clearNMMoveList(vector<Move*>* moveList, unsigned int depth) {
    Move* move = nullptr;
    while (moveList->size() > 0) {
      move = (*moveList)[0];
      moveList->pop_front();
      if ((depth > 0) || (move != BestMove)) {
        delete move;
      }
      move = nullptr;
    }
    delete moveList;
    moveList = nullptr;
    return;
  }

  double Game::negaMax(Position* p1,
                              unsigned int depth, Player* player,
                              double alpha, double beta) {
    double max = -Infinity;
    bool verbose = false;
    double x = 0.0;
    // unsigned int gamma = 37; // obviously illegal
    bool bottomOut = false;
    Position* p2 = nullptr;
    if ((MaxSearchDepth == depth) || (true == p1->gameOverP(player))) {
      bottomOut = true;
      max = p1->staticEval(player);
    }
    else {
      vector<Move*>* moveList = p1->legalMoves(player);
      unsigned int numMoves = moveList->size();
      unsigned int i = 0;
      Move* move = nullptr;
      assert (numMoves > 0); // or else the game would be over
      for (i=0; i<numMoves; i++) {
        move = (*moveList)[i];
        p2 = p1->nextPosition(move); // make sure to swap onMove and offMove in nextPosition
        x = - negaMax(p2, depth+1, p2->getOnMove(), -beta, -alpha);
        delete p2;
        p2 = nullptr;
        if (x > max) {
          max = x;
          if (0 == depth) {
            BestValue = max;
            BestMove = move;
          }
        } // end of if (x > max)
        if (x > alpha) {
          alpha = x;
        }
        if (alpha >= beta) {
          // std::cout << "+" << std::flush; // note a cutoff
          clearNMMoveList(moveList, depth);
          return alpha;
        }
      }
      clearNMMoveList(moveList, depth);
      //     while (moveList->size() > 0) {
      //       move = (*moveList)[0];
      //       moveList->pop_front();
      //       if ((depth > 0) || (move != Position::bestMove)) {
      // 	delete move;
      //       }
      //       move = NULL;
      //     }
      //     delete moveList;
      //     moveList = NULL;


    }
    if (verbose) {
      /*
        cout <<endl  << flush;
        cout <<endl  << flush;
        cout << "          " << "In negaMax .... "<<endl ;
        cout << "          " << "remDepth =  "<< remDepth <<endl ;
        cout << "          " << "color =  "<< colorString(color) <<endl ;
        cout << "          " << "mover =  "<< colorString(board->onMove) <<endl ;
        cout << "          " << "eval =  "<< max <<endl ;
        if (false == bottomOut) {
        //    cout << "          " << "move =  "<< sControl -> bestMove <<endl ;
        }
        board->show();
        cout <<endl  << flush;
      */
    }
    if (false == bottomOut) {
      //    assert (true == p1->legalP(sControl -> bestMove, player));
    }

    assert (max > -Infinity);
    assert (max < +Infinity);

    return max;
  }


  // ----------------------------------

  Player::Player() : CountedItem() {
  }


  Player::~Player() {
    // do nothing
  }


  // ----------------------------------

  Move::Move(Player* p) : CountedItem() {
    assert (nullptr != p);
    player = p;
  }


  Move::~Move() {
    // do nothing
  }

} // namespace TPCSPIGame

// ---------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ---------------------------------------------
