/*
  Usapyon2, a USI shogi(japanese-chess) playing engine derived from 
  Stockfish 7 & nanoha-mini 0.2.2.1
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad  (Stockfish author)
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad  (Stockfish author)
  Copyright (C) 2014-2016 Kazuyuki Kawabata (nanoha-mini author)
  Copyright (C) 2015-2016 Yasuhiro Ike

  Usapyon2 is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Usapyon2 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <sstream>
#include <string>

#include "evaluate.h"
#ifdef NANOHA
#include "param_apery.h"
#endif
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "timeman.h"
#include "uci.h"

using namespace std;

extern void benchmark(const Position& pos, istream& is);
vector<Move> vIgnoreMoves;
vector<Move> vForceMove;

#ifdef USAPYON2
void clearIgnoreMoves() {
	vIgnoreMoves.clear();
}

void clearForceMove() {
	vForceMove.clear();
}

bool isIgnoreMove(Move m) {
	for (auto itr = vIgnoreMoves.begin(); itr != vIgnoreMoves.end(); ++itr) {
		if (*itr == m) {
			return true;
		}
	}
	return false;
}

bool isForceMove(Move m) {
	// 強制手がない場合、trueを返す
	if (vForceMove.size() == 0) {
		return true;
	}
	for (auto itr = vForceMove.begin(); itr != vForceMove.end(); ++itr) {
		if (*itr == m) {
			return true;
		}
	}
	return false;
}
#endif

namespace {

#if defined(NANOHA)
	// FEN string of the initial position, normal shogi
	const char* StartFEN = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1";
#else
  // FEN string of the initial position, normal chess
  const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
#endif

  // Stack to keep track of the position states along the setup moves (from the
  // start position to the position just before the search starts). Needed by
  // 'draw by repetition' detection.
  Search::StateStackPtr SetupStates;



#ifdef USAPYON2
  // ルートで無視する手を設定
  void ignoreMoves(Position& pos, istringstream& is) {
	  Move m;
	  string token;
	  // Parse move list
	  while (is >> token && (m = UCI::to_move(pos, token)) != MOVE_NONE) {
		  // Clearはどうするべきか→position関数で初期化
		  vIgnoreMoves.push_back(m);
	  }

#if 0
	  // 動作確認用
	  string str;
	  str = "2b8h+";
	  m = UCI::to_move(pos, str);
	  std::cout << "info string " << str << move_to_csa(m) << endl;
	  vIgnoreMoves.push_back(m);
	  str = "2b8h";
	  m = UCI::to_move(pos, str);
	  std::cout << "info string " << str << move_to_csa(m) << endl;
	  vIgnoreMoves.push_back(m);
#endif
  }

  // ルートで強制する手を設定
  void forceMove(Position &pos,istringstream& is) {
	  Move m;
	  string token;
	  // Parse Move
	  while (is >> token && (m = UCI::to_move(pos, token)) != MOVE_NONE) {
		  vForceMove.push_back(m);
	  }
#if 0
	  // 動作確認用
	  string str;
	  str = "P*7b";
	  if ((m = UCI::to_move(pos, str)) !=MOVE_NONE) {
		  std::cout << "info string " << str << move_to_csa(m) << endl;
		  vForceMove.push_back(m);
	  }
#endif
  }

#endif


  // position() is called when engine receives the "position" UCI command.
  // The function sets up the position described in the given FEN string ("fen")
  // or the starting position ("startpos") and then makes the moves given in the
  // following move list ("moves").

  void position(Position& pos, istringstream& is) {

    Move m;
    string token, fen;
#ifdef USAPYON2
	clearIgnoreMoves();
	clearForceMove();
#endif
    is >> token;

    if (token == "startpos")
    {
        fen = StartFEN;
        is >> token; // Consume "moves" token if any
    }
#ifdef NANOHA
	else if (token == "sfen")
#else
	else if (token == "fen")
#endif
        while (is >> token && token != "moves")
            fen += token + " ";
    else
        return;

#ifdef NANOHA
    pos.set(fen, Threads.main());
#else
	pos.set(fen, Options["UCI_Chess960"], Threads.main());
#endif
	SetupStates = Search::StateStackPtr(new std::stack<StateInfo>);

    // Parse move list (if any)
    while (is >> token && (m = UCI::to_move(pos, token)) != MOVE_NONE)
    {
        SetupStates->push(StateInfo());
#ifndef NANOHA
        pos.do_move(m, SetupStates->top(), pos.gives_check(m, CheckInfo(pos)));
#else
		pos.do_move(m, SetupStates->top());
#endif
	}

  }

  // setoption() is called when engine receives the "setoption" UCI command. The
  // function updates the UCI option ("name") to the given value ("value").

  void setoption(istringstream& is) {

    string token, name, value;

    is >> token; // Consume "name" token

    // Read option name (can contain spaces)
    while (is >> token && token != "value")
        name += string(" ", name.empty() ? 0 : 1) + token;

    // Read option value (can contain spaces)
    while (is >> token)
        value += string(" ", value.empty() ? 0 : 1) + token;

    if (Options.count(name))
        Options[name] = value;
    else
        sync_cout << "No such option: " << name << sync_endl;
  }


  // go() is called when engine receives the "go" UCI command. The function sets
  // the thinking time and other parameters from the input string, then starts
  // the search.

  void go(const Position& pos, istringstream& is) {

    Search::LimitsType limits;
    string token;

    limits.startTime = now(); // As early as possible!

    while (is >> token)
        if (token == "searchmoves")
            while (is >> token)
                limits.searchmoves.push_back(UCI::to_move(pos, token));

        else if (token == "wtime")     is >> limits.time[WHITE];
        else if (token == "btime")     is >> limits.time[BLACK];
        else if (token == "winc")      is >> limits.inc[WHITE];
        else if (token == "binc")      is >> limits.inc[BLACK];
        else if (token == "movestogo") is >> limits.movestogo;
        else if (token == "depth")     is >> limits.depth;
        else if (token == "nodes")     is >> limits.nodes;
#ifdef NANOHA
		else if (token == "movetime" || token == "byoyomi") {
			// byoyomiがあると、常に思考時間に反映される状態に…
			// limits.timeを見て調整することにする。
			// 残り時間10秒以下？で秒読みへ
			if (limits.time[pos.side_to_move()] <= 10) {
				is >> limits.movetime;
				int mg = Options["ByoyomiMargin"];
				if (limits.movetime - 100 > mg) {
					limits.movetime -= mg;
				}
			}
		}
  
#else
		else if (token == "movetime")  is >> limits.movetime;
#endif
        else if (token == "mate")      is >> limits.mate;
        else if (token == "infinite")  limits.infinite = 1;
        else if (token == "ponder")    limits.ponder = 1;

    Threads.start_thinking(pos, limits, SetupStates);
  }

} // namespace


/// UCI::loop() waits for a command from stdin, parses it and calls the appropriate
/// function. Also intercepts EOF from stdin to ensure gracefully exiting if the
/// GUI dies unexpectedly. When called with some command line arguments, e.g. to
/// run 'bench', once the command is executed the function returns immediately.
/// In addition to the UCI ones, also some additional debug commands are supported.

void UCI::loop(int argc, char* argv[]) {

  Position pos(StartFEN, Threads.main()); // The root position
  string token, cmd;

  for (int i = 1; i < argc; ++i)
      cmd += std::string(argv[i]) + " ";

  do {
      if (argc == 1 && !getline(cin, cmd)) // Block here waiting for input or EOF
          cmd = "quit";

      istringstream is(cmd);

      token.clear(); // getline() could return empty or blank line
      is >> skipws >> token;

      // The GUI sends 'ponderhit' to tell us to ponder on the same move the
      // opponent has played. In case Signals.stopOnPonderhit is set we are
      // waiting for 'ponderhit' to stop the search (for instance because we
      // already ran out of time), otherwise we should continue searching but
      // switching from pondering to normal search.
      if (    token == "quit"
          ||  token == "stop"
          || (token == "ponderhit" && Search::Signals.stopOnPonderhit))
      {
          Search::Signals.stop = true;
          Threads.main()->start_searching(true); // Could be sleeping
      }
      else if (token == "ponderhit")
          Search::Limits.ponder = 0; // Switch to normal search

#ifdef NANOHA
	  else if (token == "usi")
		  sync_cout << "id name " << engine_info(true)
		  << "\n" << Options
		  << "\nusiok" << sync_endl;

	  else if (token == "usinewgame")
	  {
		  Search::clear();
		  Time.availableNodes = 0;
	  }
	  else if (token == "gameover")
	  {
		  // 実は将棋所だけ？
		  Search::Signals.stop = true;
		  Threads.main()->start_searching(true); // Could be sleeping
	  }
#else
	  else if (token == "uci")
          sync_cout << "id name " << engine_info(true)
                    << "\n"       << Options
                    << "\nuciok"  << sync_endl;

      else if (token == "ucinewgame")
      {
          Search::clear();
          Time.availableNodes = 0;
      }
#endif
      else if (token == "isready")    sync_cout << "readyok" << sync_endl;
      else if (token == "go")         go(pos, is);
      else if (token == "position")   position(pos, is);
#ifdef USAPYON2
	  else if (token == "ignore_moves")   ignoreMoves(pos, is);
	  else if (token == "force_move")   forceMove(pos, is);
#endif
      else if (token == "setoption")  setoption(is);

      // Additional custom non-UCI commands, useful for debugging
#ifndef NANOHA
	  else if (token == "flip")       pos.flip();
#endif
      else if (token == "bench")      benchmark(pos, is);
      else if (token == "d")          sync_cout << pos << sync_endl;
#ifndef NANOHA
      else if (token == "eval")       sync_cout << Eval::trace(pos) << sync_endl;
	  else if (token == "perft")
      {
          int depth;
          stringstream ss;

          is >> depth;
          ss << Options["Hash"]    << " "
             << Options["Threads"] << " " << depth << " current perft";

          benchmark(pos, ss);
      }
#endif
	  else
          sync_cout << "Unknown command: " << cmd << sync_endl;

  } while (token != "quit" && argc == 1); // Passed args have one-shot behaviour

  Threads.main()->wait_for_search_finished();
}


/// UCI::value() converts a Value to a string suitable for use with the UCI
/// protocol specification:
///
/// cp <x>    The score from the engine's point of view in centipawns.
/// mate <y>  Mate in y moves, not plies. If the engine is getting mated
///           use negative values for y.

string UCI::value(Value v) {

  stringstream ss;

#ifdef USAPYON2
  // mate等が表示されると、マスター側で一番良い手を探すのに面倒くさい
  ss << "cp " << v * 100 / DPawn;
#else
  if (abs(v) < VALUE_MATE - MAX_PLY)
	  ss << "cp " << v * 100 / DPawn;
  else
	  ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;
#endif

  return ss.str();
}


/// UCI::square() converts a Square to a string in algebraic notation (g1, a7, etc.)

#ifndef NANOHA
std::string UCI::square(Square s) {
  return std::string{ char('a' + file_of(s)), char('1' + rank_of(s)) };
}
#endif

/// UCI::move() converts a Move to a string in coordinate notation (g1f3, a7a8q).
/// The only special case is castling, where we print in the e1g1 notation in
/// normal chess mode, and in e1h1 notation in chess960 mode. Internally all
/// castling moves are always encoded as 'king captures rook'.

#ifndef NANOHA
string UCI::move(Move m, bool chess960) {
	Square from = from_sq(m);
	Square to = to_sq(m);

	if (m == MOVE_NONE)
		return "(none)";

	if (m == MOVE_NULL)
		return "0000";

	if (type_of(m) == CASTLING && !chess960)
		to = make_square(to > from ? FILE_G : FILE_C, rank_of(from));

	string move = UCI::square(from) + UCI::square(to);

	if (type_of(m) == PROMOTION)
		move += " pnbrqk"[promotion_type(m)];

	return move;
}
#else
string UCI::move(Move m) {
	return move_to_uci(m);
}
#endif


/// UCI::to_move() converts a string representing a move in coordinate notation
/// (g1f3, a7a8q) to the corresponding legal Move, if any.

#ifndef NANOHA
Move UCI::to_move(const Position& pos, string& str) {

  if (str.length() == 5) // Junior could send promotion piece in uppercase
      str[4] = char(tolower(str[4]));

  for (const auto& m : MoveList<MV_LEGAL>(pos))
      if (str == UCI::move(m))
          return m;

  return MOVE_NONE;
}
#else
Move UCI::to_move(const Position& pos, string& str) {
	return move_from_uci(pos, str);
}
#endif