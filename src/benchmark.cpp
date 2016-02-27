/*
  Usapyon2, a USI shogi(japanese-chess) playing engine derived from 
  Stockfish 7 & nanoha-mini 0.2.2.1
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2014-2016 Kazuyuki Kawabata
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

#include <fstream>
#include <iostream>
#include <istream>
#include <vector>

#include "misc.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "uci.h"
#include "evaluate.h"


using namespace std;

namespace {

#ifdef NANOHA
// 指し手生成；指し手生成祭り局面と初期局面
static const string GenMoves[] = {
	"l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w RGgsn5p 1",
	"lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1",
	""
};
static const string EvalPos[] = {
	// 初期局面.
	"lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1",
	// 歩を突いた局面の左右反転と先後反転させたもので対称性を確認する.
	"lnsgkgsnl/1r5b1/ppppppppp/9/9/2P6/PP1PPPPPP/1B5R1/LNSGKGSNL b - 1",
	"lnsgkgsnl/1b5r1/ppppppppp/9/9/6P2/PPPPPP1PP/1R5B1/LNSGKGSNL b - 1",
	"lnsgkgsnl/1r5b1/ppppppppp/9/9/2P6/PP1PPPPPP/1B5R1/LNSGKGSNL w - 1",
	"lnsgkgsnl/1b5r1/ppppppppp/9/9/6P2/PPPPPP1PP/1R5B1/LNSGKGSNL w - 1",
	// 対称の確認.
	//   点対称
	"+l+n+sgkgsnl/1+r5b1/+pp+pp+ppppp/9/9/9/PPPP+PP+PP+P/1B5+R1/LNSGKG+S+N+L b - 1",
	//   左右対称
	"lnsgkg+s+n+l/1+b5r1/+pp+pp+pp+ppp/9/9/9/+PP+PP+PP+PPP/1+B5R1/LNSGKG+S+N+L b - 1",
	//   上下反転と左右反転
	"l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w RGgsn5p 1",
	"ln4Bkl/r8/p5gs1/1ppB2p1p/3p2sP1/P1P4pP/2NP1s3/5+p1GK/L6NL w GSN5Prg 1",
	"lkB4nl/8r/1sg5p/p1p2Bpp1/1Ps2p3/Pp4P1P/3s1PN2/KG1+p5/LN6L w GSN5Prg 1",
	""
};
#endif

#ifdef NANOHA
	// 局面数を 16 にする
	// 進歩本2の棋力判定問題のNo.1 - No.16
static const string Defaults[] = {
	"lR1B3nl/2gp5/ngk1+BspPp/1s2p2p1/p4S3/1Pp6/P5P1P/LGG6/KN5NL b Prs5p 1",
	"5S2l/1rP2s1k1/p2+B1gnp1/5np2/3G3n1/5S2p/P1+p1PpPP1/1P1PG2KP/L2+rLPGNL b Bs3p 1",
	"lR6l/1s1g5/1k1s1+P2p/1+bpp1+Bs2/1n1n2Pp1/2P6/S2R4P/K1GG5/9 b 2NPg2l9p 1",
	"l4g1nl/4g1k2/2n1sp1p1/p5pPp/5Ps2/1P1p2s2/P1G1+p1N1P/6K2/LN5RL b RBG3Pbs3p 1",
	"1n4g1k/6r2/1+P1psg1p+L/2p1pp3/3P5/p1P1PPPP1/3SGS3/1+p1K1G2r/9 b 2BNLPs2n2l3p 1",
	"+B2+R3n1/3+L2gk1/5gss1/p1p1p1ppl/5P2p/PPPnP1PP1/3+p2N2/6K2/L4S1RL b BGS3Pgnp 1",
	"3R4l/1kg6/2ns5/spppp2+Bb/p7p/1PPPP1g2/nSNSNP2P/KG1G5/5r2L b L4Pl2p 1",
	"ln5nl/2r2gk2/1p2sgbpp/pRspppp2/L1p4PP/3PP1G2/N4PP2/3BS1SK1/5G1NL b 3P 1",
	"ln7/1r2k1+P2/p3gs3/1b1g1p+B2/1p5R1/2pPP4/PP1S1P3/2G2G3/LN1K5 b SNL3Psnl5p 1",
	"3+P3+Rl/2+P2kg2/+B2psp1p1/4p1p1p/9/2+p1P+bnKP/P6P1/4G1S2/L4G2L b G2S2NLrn5p 1",
	"ln1gb2nl/1ks4r1/1p1g4p/p1pppspB1/5p3/PPPPP1P2/1KNG1PS1P/2S4R1/L2G3NL b Pp 1",
	"lr6l/4g1k1p/1s1p1pgp1/p3P1N1P/2Pl5/PPbBSP3/6PP1/4S1SK1/1+r3G1NL b N3Pgn2p 1",
	"l1ks3+Bl/2g2+P3/p1npp4/1sNn2B2/5p2p/2PP5/PPN1P1+p1P/1KSSg2P1/L1G5+r b GL4Pr 1",
	"ln3k1nl/2P1g4/p1lpsg1pp/4p4/1p1P1p3/2SBP4/PP1G1P1PP/1K1G3+r1/LN1s2PNR b BSPp 1",
	"+N6nl/1+R2pGgk1/5Pgp1/p2p1sp2/3B1p2p/P1pP4P/6PP1/L3G1K2/7NL b RNL2Pb3s2p 1",
	"ln1g5/1r4k2/p2pppn2/2ps2p2/1p7/2P6/PPSPPPPLP/2G2K1pr/LN4G1b b BG2SLPnp 1",
	""
#else
const vector<string> Defaults = {
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10",
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11",
  "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
  "rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
  "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
  "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
  "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
  "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
  "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
  "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
  "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
  "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
  "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
  "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
  "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
  "6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1",
  "3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1",
  "2K5/p7/7P/5pR1/8/5k2/r7/8 w - - 0 1",
  "8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1",
  "7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1",
  "8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1",
  "8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - - 0 1",
  "8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 0 1",
  "8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b - - 0 1",
  "5k2/7R/4P2p/5K2/p1r2P1p/8/8/8 b - - 0 1",
  "6k1/6p1/P6p/r1N5/5p2/7P/1b3PP1/4R1K1 w - - 0 1",
  "1r3k2/4q3/2Pp3b/3Bp3/2Q2p2/1p1P2P1/1P2KP2/3N4 w - - 0 1",
  "6k1/4pp1p/3p2p1/P1pPb3/R7/1r2P1PP/3B1P2/6K1 w - - 0 1",
  "8/3p3B/5p2/5P2/p7/PP5b/k7/6K1 w - - 0 1",

  // 5-man positions
  "8/8/8/8/5kp1/P7/8/1K1N4 w - - 0 1",     // Kc2 - mate
  "8/8/8/5N2/8/p7/8/2NK3k w - - 0 1",      // Na2 - mate
  "8/3k4/8/8/8/4B3/4KB2/2B5 w - - 0 1",    // draw

  // 6-man positions
  "8/8/1P6/5pr1/8/4R3/7k/2K5 w - - 0 1",   // Re5 - mate
  "8/2p4P/8/kr6/6R1/8/8/1K6 w - - 0 1",    // Ka2 - mate
  "8/8/3P3k/8/1p6/8/1P6/1K3n2 b - - 0 1",  // Nd2 - draw

  // 7-man positions
  "8/R7/2q5/8/6k1/8/1P5p/K6R w - - 0 124"  // Draw
#endif
};

} // namespace

/// benchmark() runs a simple benchmark by letting Stockfish analyze a set
/// of positions for a given limit each. There are five parameters: the
/// transposition table size, the number of search threads that should
/// be used, the limit value spent for each position (optional, default is
/// depth 13), an optional file name where to look for positions in FEN
/// format (defaults are the positions defined above) and the type of the
/// limit value: depth (default), time in millisecs or number of nodes.

void benchmark(const Position& current, istream& is) {

  string token;
  vector<string> fens;
  Search::LimitsType limits;

  // Assign default values to missing arguments
  string ttSize    = (is >> token) ? token : "16";
  string threads   = (is >> token) ? token : "1";
  string limit     = (is >> token) ? token : "13";
  string fenFile   = (is >> token) ? token : "default";
  string limitType = (is >> token) ? token : "depth";

  Options["Hash"]    = ttSize;
  Options["Threads"] = threads;
  Search::clear();

  if (limitType == "time")
      limits.movetime = stoi(limit); // movetime is in millisecs

  else if (limitType == "nodes")
      limits.nodes = stoi(limit);

  else if (limitType == "mate")
      limits.mate = stoi(limit);

  else
      limits.depth = stoi(limit);

  if (fenFile == "default") {
	  for (int i = 0; !Defaults[i].empty(); i++)
		  fens.push_back(Defaults[i]);
  }

  else if (fenFile == "current")
      fens.push_back(current.fen());

  else
  {
      string fen;
      ifstream file(fenFile);

      if (!file.is_open())
      {
          cerr << "Unable to open file " << fenFile << endl;
          return;
      }

      while (getline(file, fen))
          if (!fen.empty())
              fens.push_back(fen);

      file.close();
  }

  uint64_t nodes = 0;
  TimePoint elapsed = now();

  for (size_t i = 0; i < fens.size(); ++i)
  {
      Position pos(fens[i], Threads.main());

      cerr << "\nPosition: " << i + 1 << '/' << fens.size() << endl;

      if (limitType == "perft")
          nodes += Search::perft<true>(pos, limits.depth * ONE_PLY);

      else
      {
          Search::StateStackPtr st;
          limits.startTime = now();
          Threads.start_thinking(pos, limits, st);
          Threads.main()->wait_for_search_finished();
          nodes += Threads.nodes_searched();
      }
  }

  elapsed = now() - elapsed + 1; // Ensure positivity to avoid a 'divide by zero'

  dbg_print(); // Just before to exit

  cerr << "\n==========================="
       << "\nTotal time (ms) : " << elapsed
       << "\nNodes searched  : " << nodes
       << "\nNodes/second    : " << 1000 * nodes / elapsed << endl;
}

#ifdef NANOHA
namespace {
	struct ResultMate1 {
		int msec;
		int result;
		Move m;
	};
	void disp_moves(MoveStack mstack[], size_t n)
	{
		cerr << "     ";
		for (size_t i = 0; i < n; i++) {
			cerr << " " << move_to_csa(mstack[i].move);
			if (i % 6 == 5) cerr << endl << "     ";
		}
		cerr << endl;
	}
	string conv_per_s(const double loops, int t)
	{
		if (t == 0) t++;
		double nps = loops * 1000 / t;
		char buf[64];
		if (nps > 1000*1000) {snprintf(buf, sizeof(buf), "%.3f M", nps / 1000000.0); }
		else if (nps > 1000) {snprintf(buf, sizeof(buf), "%.3f k", nps / 1000.0); }
		else  {snprintf(buf, sizeof(buf), "%d ", int(nps)); }
		return string(buf);
	}
}

// 1手詰め or 3手詰め
void bench_mate(int argc, char* argv[]) {

	vector<string> sfenList;
	TimePoint time;
	vector<ResultMate1> result;
	int type = (string(argv[1]) == "mate1") ? 0 : 1;
	const char *typestr[] = { "Mate1ply", "Mate3play" };

	// デフォルト値を設定
	string fenFile = argc > 2 ? argv[2] : "default";
	bool bLoop    = argc > 3 ? (string(argv[3]) == "yes" ? true : false) : true;
	bool bDisplay = argc > 4 ? (string(argv[4]) == "no" ? false : true) : false;

	cerr << "Benchmark type: " << typestr[type] << " routine." << endl;

	if (fenFile != "default")
	{
		string fen;
		ifstream f(fenFile.c_str());

		if (!f.is_open())
		{
			cerr << "Unable to open file " << fenFile << endl;
			exit(EXIT_FAILURE);
		}

		while (getline(f, fen)) {
			if (!fen.empty()) {
				if (fen.compare(0, 5, "sfen ") == 0) {
					fen.erase(0, 5);
				}
				sfenList.push_back(fen);
			}
		}

		f.close();
	}
	else {
		for (int i = 0; !Defaults[i].empty(); i++) {
			sfenList.push_back(Defaults[i]);
		}
	}

	// ベンチ開始
	int loops = (bLoop ? 1000*1000 : 1000); // 1M回
	if (type != 0) loops /= 10;	// mate3はmate1より時間がかかるので、1/10にする

	ResultMate1 record;

	time = now();
	int total = 0;
	size_t i;
	for (i = 0; i < sfenList.size(); i++)
	{
		Move move = MOVE_NONE;
		Position pos(sfenList[i], Threads.main());
#if defined(_DEBUG) || !defined(NDEBUG)
		int failState;
		assert(pos.pos_is_ok(&failState));
#endif

		if (bLoop)
				cerr << "\nBench position: " << i + 1 << '/' << sfenList.size() << "  c=" << pos.side_to_move() << endl;

		volatile int v = 0;
		uint32_t info;
		int j;
		TimePoint rap_time = now();
		if (type == 0) {
			// 1手詰め
			if (pos.side_to_move() == BLACK) {
				for (j = 0; j < loops; j++) {
					v = pos.Mate1ply<BLACK>(move, info);
				}
			} else {
				for (j = 0; j < loops; j++) {
					v = pos.Mate1ply<WHITE>(move, info);
				}
			}
		} else {
			// 3手詰め
			for (j = 0; j < loops; j++) {
				v = pos.Mate3(pos.side_to_move(), move);
			}
		}
		rap_time = now() - rap_time;
		total += rap_time;
		if (bLoop && bDisplay) pos.print_csa(move);

		record.msec = rap_time;
		record.result = v;
		record.m = move;
		result.push_back(record);

		if (bLoop) cerr << v << "\t" << move_to_csa(move) << "  " << rap_time << "(ms)  "
		                << conv_per_s(loops, rap_time) << " times/s" << endl;
	}

	time = now() - time;
	if (time == 0) time = 1;

	cerr << "\n==============================="
		 << "\nTotal time (ms) : " << time << "(" << total << ")";
	cerr << "\n  Average : " << conv_per_s(static_cast<const double>(loops*sfenList.size()), time) << " times/s" << endl;

	int solved = 0;
	int unknown = 0;
	for (i = 0; i < result.size(); i++) {
		if (result[i].result == VALUE_MATE) {
			solved++;
		} else {
			unknown++;
		}
	}
	cerr << "Mate    =  " << solved  << endl;
	cerr << "Unknown =  " << unknown << endl;
	cerr << "Ave.time=  " << double(time) / result.size() << "(ms)" << endl;
	cerr << "Lopps   =  " << loops << endl;
	cerr << "Average =  " << conv_per_s(static_cast<const double>(loops*result.size()), time) << " times/s" << endl;
}

void bench_genmove(int argc, char* argv[]) {

	vector<string> sfenList;
	int time;

	// デフォルト値を設定
	string fenFile = argc > 2 ? argv[2] : "default";
	bool bDisplay = argc > 3 ? (string(argv[3]) == "yes" ? true : false) : false;

	cerr << "Benchmark type: generate moves." << endl;

	if (fenFile != "default")
	{
		string fen;
		ifstream f(fenFile.c_str());

		if (!f.is_open())
		{
			cerr << "Unable to open file " << fenFile << endl;
			exit(EXIT_FAILURE);
		}

		while (getline(f, fen)) {
			if (!fen.empty()) {
				if (fen.compare(0, 5, "sfen ") == 0) {
					fen.erase(0, 5);
				}
				sfenList.push_back(fen);
			}
		}

		f.close();
		cerr << "SFEN file is" << fenFile << "." << endl;
	}
	else {
		for (int i = 0; !GenMoves[i].empty(); i++) {
			sfenList.push_back(GenMoves[i]);
		}
		cerr << "SFENs is default." << endl;
	}

	time = now();

	MoveStack ss[MAX_MOVES];
	volatile MoveStack *mlist = NULL;
#if defined(NDEBUG)
	int loops = 1000*1000; // 1M回
#else
	int loops = 500*1000; // 500k回
#endif
	int j;
	for (size_t i = 0; i < sfenList.size(); i++)
	{
		Position pos(sfenList[i], Threads.main());
#if defined(_DEBUG)
		int failState;
		assert(pos.pos_is_ok(&failState));
#endif

		cerr << "\nBench position: " << i + 1 << '/' << sfenList.size() << endl;
		int rap_time = now();
		for (j = 0; j < loops; j++) {
			mlist = generate<MV_LEGAL>(pos, ss);
		}
		rap_time = now() - rap_time;
		if (bDisplay) pos.print_csa();
		cerr << "  Genmove(" << mlist - ss << "): " << rap_time << "(ms), " << conv_per_s(loops, rap_time) << "times/s" << endl;
		if (bDisplay) disp_moves(ss, mlist - ss);

		rap_time = now();
		for (j = 0; j < loops; j++) {
			mlist = generate<MV_CAPTURE>(pos, ss);
		}
		rap_time = now() - rap_time;
		cerr << "  Gencapture(" << mlist - ss << "): " << rap_time << "(ms), " << conv_per_s(loops, rap_time) << "times/s" << endl;
		if (bDisplay) disp_moves(ss, mlist - ss);

		rap_time = now();
		for (j = 0; j < loops; j++) {
			mlist = generate<MV_NON_CAPTURE>(pos, ss);
		}
		rap_time = now() - rap_time;
		cerr << "  noncapture(" << mlist - ss << "): " << rap_time << "(ms), " << conv_per_s(loops, rap_time) << "times/s" << endl;
		if (bDisplay) disp_moves(ss, mlist - ss);

		rap_time = now();
		for (j = 0; j < loops; j++) {
			mlist = generate<MV_CHECK>(pos, ss);
		}
		rap_time = now() - rap_time;
		cerr << "  gen_check(" << mlist - ss << "): " << rap_time << "(ms), " << conv_per_s(loops, rap_time) << "times/s" << endl;
		if (bDisplay) disp_moves(ss, mlist - ss);
	}

	time = now() - time;

	cerr << "\n==============================="
		 << "\nTotal time (ms) : " << time << endl;
}

void bench_eval(int argc, char* argv[]) {

	vector<string> sfenList;
	int time;

	// デフォルト値を設定
	string fenFile = argc > 2 ? argv[2] : "default";
	bool bDisplay = argc > 3 ? (string(argv[3]) == "yes" ? true : false) : false;

	cerr << "Benchmark type: evaluate." << endl;

	if (fenFile != "default")
	{
		string fen;
		ifstream f(fenFile.c_str());

		if (!f.is_open())
		{
			cerr << "Unable to open file " << fenFile << endl;
			exit(EXIT_FAILURE);
		}

		while (getline(f, fen)) {
			if (!fen.empty()) {
				if (fen.compare(0, 5, "sfen ") == 0) {
					fen.erase(0, 5);
				}
				sfenList.push_back(fen);
			}
		}

		f.close();
		cerr << "SFEN file is" << fenFile << "." << endl;
	}
	else {
		for (int i = 0; !EvalPos[i].empty(); i++) {
			sfenList.push_back(EvalPos[i]);
		}
	}

	time = now();

#if defined(NDEBUG)
	int loops = 1000*1000;	// 1M回
#else
	int loops =   50*1000;	// 50k回
#endif
	int j;
	volatile Value v = VALUE_ZERO;
	Value m = VALUE_ZERO;
	for (size_t i = 0; i < sfenList.size(); i++)
	{
		Position pos(sfenList[i], Threads.main());
#if defined(_DEBUG)
		int failState;
		assert(pos.pos_is_ok(&failState));
#endif

		cerr << "\nBench position: " << i + 1 << '/' << sfenList.size() << endl;
		int rap_time = now();
		for (j = 0; j < loops; j++) {
			v = evaluate(pos);
		}
		rap_time = now() - rap_time;
		if (bDisplay) pos.print_csa();
		cerr << "  evaluate():m=" << pos.get_material() << ", v= " << int(v) << ", margin=" << int(m) << ", time= " << rap_time << "(ms), " << conv_per_s(loops, rap_time) << " evaluate/s" << endl;
	}

	time = now() - time;

	cerr << "\n==============================="
		 << "\nTotal time (ms) : " << time << endl;
}

#endif