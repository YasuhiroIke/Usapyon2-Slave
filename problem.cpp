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

#include <fstream>
#include <iostream>
#include <vector>

#include "position.h"
#include "thread.h"
#include "search.h"
#include "uci.h"
#if defined(NANOHA)
#include "movegen.h"
#endif

using namespace std;

namespace {
// SEEの標準
// 進歩本2の棋力判定問題のNo.1 - No.5×手番(先手、後手)
const string tc_see[] = {
	"lR1B3nl/2gp5/ngk1+BspPp/1s2p2p1/p4S3/1Pp6/P5P1P/LGG6/KN5NL b Prs5p 1",
	"lR1B3nl/2gp5/ngk1+BspPp/1s2p2p1/p4S3/1Pp6/P5P1P/LGG6/KN5NL w Prs5p 1",
	"5S2l/1rP2s1k1/p2+B1gnp1/5np2/3G3n1/5S2p/P1+p1PpPP1/1P1PG2KP/L2+rLPGNL b Bs3p 1",
	"5S2l/1rP2s1k1/p2+B1gnp1/5np2/3G3n1/5S2p/P1+p1PpPP1/1P1PG2KP/L2+rLPGNL w Bs3p 1",
	"lR6l/1s1g5/1k1s1+P2p/1+bpp1+Bs2/1n1n2Pp1/2P6/S2R4P/K1GG5/9 b 2NPg2l9p 1",
	"lR6l/1s1g5/1k1s1+P2p/1+bpp1+Bs2/1n1n2Pp1/2P6/S2R4P/K1GG5/9 w 2NPg2l9p 1",
	"l4g1nl/4g1k2/2n1sp1p1/p5pPp/5Ps2/1P1p2s2/P1G1+p1N1P/6K2/LN5RL b RBG3Pbs3p 1",
	"l4g1nl/4g1k2/2n1sp1p1/p5pPp/5Ps2/1P1p2s2/P1G1+p1N1P/6K2/LN5RL w RBG3Pbs3p 1",
	"1n4g1k/6r2/1+P1psg1p+L/2p1pp3/3P5/p1P1PPPP1/3SGS3/1+p1K1G2r/9 b 2BNLPs2n2l3p 1",
	"1n4g1k/6r2/1+P1psg1p+L/2p1pp3/3P5/p1P1PPPP1/3SGS3/1+p1K1G2r/9 w 2BNLPs2n2l3p 1",
	""
};
// 進歩本2の棋力判定問題
const string ComShogi[] = {
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
	"ln3k2l/1r7/p1bp1g1pp/2p1p4/1pBP1ns2/4Pp3/PPSG3PP/1KG2R3/LN5NL b G2P2s2p 1",
	"ln6l/1r4gk1/p2psg1pp/2pb1pp2/1p2p1Ss1/2PP4P/PPSG1P3/2GB2R2/LNK5L b NPn2p 1",
	"ln1g5/1ks4+Rl/1pbg2+P2/pn2r1s1p/2Pp1P3/P3PS2P/1PGP1G3/1K1s5/LN6L b BN2P4p 1",
	"ln1g5/1ks5l/1pb6/1n2r1+P+Rp/2PpPP3/pGn2S2P/1P1P1G3/K2s5/LN6L b BGSP6p 1",
	"ln3g1nl/4g1s2/p1+Pp1p1k1/6ppp/3P5/2p1pPPPP/PP5R1/4G1SK1/LN1+r1G1NL b BSPbsp 1",
	"5g1+Ll/4g1s2/p4p2k/3p2pPp/3P1N1p1/2pbpPS1P/P+r5S1/4G3K/L3PG1NL b S2Nrb4p 1",
	"l+B6l/3+P2gk1/2p2g1s1/p2Gppp1p/1p5n1/P1+b2PPSP/2N2SNK1/6GS1/7rL b RN4Pl2p 1",
	"l+B6l/3+PRn1k1/2p2g1g1/p2Gpppp1/1p5Lp/P4PP1P/2+r4P1/6GSK/4P3L b BS2N3P2sn 1",
	"3+Rl2kl/S1S1+PG3/2+b1p1B2/7rp/1Kp2pPN1/6GPP/1P2P3L/9/L5+p2 b 2N5P2g2sn2p 1",
	"4pg1nl/2p2gkb1/p1n+RP2p1/r4pp1p/Bp7/3P1P2P/PP4PP1/2+p3SK1/LN3G1NL b 2Sgslp 1",
	"ln1g2+Rn1/1ks1g3l/1p2s2p1/p1pp1B2p/7P1/P1PSp3P/1P1P1G+p2/1K3P3/LN5+rL b BGS2Pnp 1",
	"lr6l/3+P+P1gS1/5k3/2pGs1spp/p3p4/1PPR1p2P/PGG3+b2/1K4+b2/LNN5L b S3P2n3p 1",
	"l2g4+B/2s6/2kn3+Rp/ppg1p1P2/1ns5b/2ppP4/PP1P1P1PP/LSGS5/KNG4+rL b NLP3p 1",
	"l3l1pn1/4k4/3ppg1+Rl/p2s5/9/P2+R2P2/1P1PPP3/2K1+p4/L6N1 b G3S2N3P2b2g4p 1",
	"ln5nl/2+R2sk2/pp5pp/4pb3/4Ppp2/6PP1/Psp2PB1P/3PS2R1/L1K4NL b 4GNPs2p 1",
	"l1l1B3l/6g2/4p1nk1/4spgpp/3p2B2/rpG1S1p1P/N3P1N2/PKG6/7R1 b SNLs8p 1",
	"lngg2+R2/1k4P1l/1ps2+P1pp/1nppp4/p6P1/P1P1P1S2/BP1P1S2P/2KG1+b1+r1/LN1N4L b GS2P 1",
	"l2r1g1nl/6sk1/5g3/3Ps1ppp/pRB2p3/2P1p1PPP/P2G1PSS1/6GK1/+b6NL b N4Pnlp 1",
	"l2r4l/1+R1pg1sk1/5gnl1/3P+b1ppp/p1B1pp3/2P3PPP/P3GPSS1/6GK1/7NL b S4P2n 1",
	"l+b1r1n2l/1p1p2sk1/2+R2gnl1/3G2p1p/p1B1pp1P1/2P3P1P/P3GPSS1/6GK1/7NL b 5Psn 1",
	"l2r1n2l/1p1p3k1/5g1l1/2+R3p1p/p3pp3/6PPP/P3GPNS1/6GK1/8L b BSN3Pbg2sn3p 1",
	"ln1g3nl/1k3rg2/4s1bpp/1pPsp4/2Bp2SP1/p3P4/1PNP1P2P/2KGG1R2/L6NL b S2P3p 1",
	"l2g3nl/6g2/1k1+N3pp/1pp1p4/2+rB2SP1/p3P4/1P1P1P2P/3G2R2/Ls2K2NL b 2SNbg6p 1",
	"ln1g4l/1ksg1+P3/ppppp2pp/1l3N3/9/1BPPPS2B/PP1G4P/1KS2+n3/LN1G2+r1+r b SP3p 1",
	"lnS+N4l/9/ksppp2pp/pp4p2/9/1PPPPS2B/P2G4P/1KS2+n3/LN1G1P+r1+r b GLbg3p 1",
	"lR4Bnl/5gs2/5nkpp/p1P1psp2/3P5/4P1PPP/P+p3G3/3+b2SK1/L4G1NL b G4Prsnp 1",
	"l1+L6/1k1rn2+R1/p1p1ppg2/1G1P1bp1p/2P6/2Ns1N2P/PPNpPP3/1SG1K1S2/L4G2L b 2Pbs2p 1",
	"l5k1l/4+Psg2/p1r2p1p1/4s1p1p/1p3n3/P3P4/BP3PP1P/2GS3R1/+bNK4NL b GNPgsl4p 1",
	"ln1g4l/1kss2g2/2pp1pn2/6rp1/PpPPP1pPp/p3SP3/BPS2GP1L/1KG3R2/LN5N1 b BPp 1",
	"l2g2ks1/4P4/2p1+S2pn/p2p2+r1p/5+B3/P3S2PP/1PPP+b1P2/1rG2P3/LN2KG1NL b GN2Psl2p 1",
	"l7l/4+N1+N1k/2+B1p1ng1/p5ppp/2P1S4/PpSp2P1P/2S1P4/1KG4R1/LN6+r b B4P2gsl2p 1",
	"7nl/5bgk1/2+Pp1gspp/P4p3/4P1PP1/1l1P5/2NG1+s2P/1g1S3R1/L2KB2NL b RSN6Pp 1",
	""
};
}

void solve_problem(int argc, char* argv[]) {
	vector<string> sfenList;
	Search::LimitsType limits;
	int time;

	// Assign default values to missing arguments
	string ttSize   = "128";
	string threads  = "1";
	string valStr   = "2";
	string sfenFile = "default";
	string valType  = "time";
	string outFile  = "";
	string suffix   = "mal";
	bool bOut = false;

	// -hash N	ハッシュサイズ(MB)
	// -threads N	スレッド数
	// -sec N	秒数
	// -nodes N	ノード数
	// -depth N	深さ
	// -o output file	結果を書き出すファイル
	// -suffix suffix	ファイル中に書き出す拡張子を指定する
	while (--argc) {
		argv++;
		cerr << "argv:" << *argv << endl;
		if (argv[0][0] == '-') {
			if (argc > 1) {
				if (strcmp(*argv, "-hash") == 0) {
					argc--;
					ttSize = *++argv;
				} else if (strcmp(*argv, "-threads") == 0) {
					argc--;
					threads = *++argv;
				} else if (strcmp(*argv, "-sec") == 0) {
					argc--;
					valType = "time";
					valStr = *++argv;
				} else if (strcmp(*argv, "-nodes") == 0) {
					argc--;
					valType = "nodes";
					valStr = *++argv;
				} else if (strcmp(*argv, "-depth") == 0) {
					argc--;
					valType = "depth";
					valStr = *++argv;
				} else if (strcmp(*argv, "-o") == 0) {
					argc--;
					outFile = *++argv;
					bOut = true;
				} else if (strcmp(*argv, "-suffix") == 0) {
					argc--;
					suffix = *++argv;
				} else {
					cerr << "Error!:argv = " << *argv << endl;
					exit(EXIT_FAILURE);
				}
			} else {
				cerr << "Error!:argv = " << *argv << endl;
				exit(EXIT_FAILURE);
			}
		} else {
			sfenFile = argv[0];
			break;
		}
	}
	cerr << "sfenFile = " << sfenFile << endl;
	cerr << "ttSize   = " << ttSize << endl;
	cerr << "threads  = " << threads << endl;
	cerr << "valType  = " << valType << endl;
	cerr << "valStr   = " << valStr << endl;
	cerr << "outFile  = " << outFile << endl;
	cerr << "output   = " << (bOut ? "true" : "false") << endl;
	cerr << "suffix   = " << suffix << endl;

	Options["Hash"]=ttSize;
	Options["Threads"]=threads;
	Options["OwnBook"]=true;
	Options["Output_AllDepth"]=true;

	// Search should be limited by nodes, time or depth ?
	if (valType == "nodes")
		limits.nodes = atoi(valStr.c_str());
	else if (valType == "time")
		limits.movetime = 1000 * atoi(valStr.c_str()); // maxTime is in ms
	else
		limits.depth = atoi(valStr.c_str());

	// Do we need to load positions from a given SFEN file ?
	if (sfenFile != "default")
	{
		// ファイルを指定された
		string fen;
		ifstream f(sfenFile.c_str());

		if (!f.is_open())
		{
			cerr << "Unable to open file " << sfenFile << endl;
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
	} else {
		for (int i = 0; !ComShogi[i].empty(); i++) {
			sfenList.push_back(ComShogi[i]);
		}
	}

	// ファイル出力する？
	FILE *fp = NULL;	// 出力用
	string prefix;
	int width = 1;
	if (bOut) {
		fp = fopen(outFile.c_str(), "w");
		if (fp == NULL) {
			perror(outFile.c_str());
			exit(EXIT_FAILURE);
		}
		if (sfenFile != "default") {
			prefix = sfenFile;
			size_t npos = prefix.rfind(".sfen");
			prefix.replace(npos, 5, "");
		} else {
			prefix = "ComShogi";
		}
		if (sfenList.size() >= 10000) {
			width = 5;
		} else if (sfenList.size() >= 1000) {
			width = 4;
		} else if (sfenList.size() >= 100) {
			width = 3;
		} else if (sfenList.size() >= 10) {
			width = 2;
		}
	}
	cerr << "prefix  = " << prefix << ", width = " << width << endl;

	// Ok, let's start the benchmark !
	int64_t totalNodes = 0;
	int64_t totalTNodes = 0;
	time = now();

	for (size_t i = 0; i < sfenList.size(); i++)
	{
		Move moves[MAX_MOVES] = { MOVE_NONE };
		Position pos(sfenList[i], Threads.main());

		int rap_time = now();
		cerr << "\nBench position: " << i + 1 << '/' << sfenList.size() << endl;

		Search::StateStackPtr st;
		limits.startTime = now();
		Threads.start_thinking(pos, limits, st);
		Threads.main()->wait_for_search_finished();
//		nodes += Threads.nodes_searched();
/*
if (!think(pos, limits, moves))
break;
*/

		totalNodes  += pos.nodes_searched();
		totalTNodes += pos.tnodes_searched();
		rap_time = now() - rap_time;
		if (bOut) {
			char buf[16];
			switch (width) {
			case 5:
				sprintf(buf, "%05u", i+1);
				break;
			case 4:
				sprintf(buf, "%04u", i+1);
				break;
			case 3:
				sprintf(buf, "%03u", i+1);
				break;
			case 2:
				sprintf(buf, "%02u", i+1);
				break;
			case 1:
			default:
				sprintf(buf, "%u", i+1);
				break;
			}
			double nps = (rap_time > 0) ? 1000.0*(pos.nodes_searched() + pos.tnodes_searched()) / rap_time : 0;
			fprintf(fp, "%s%s.%s\t%s\t%6.3f\t" PRI64 "\t" PRI64 "\t%6.3f\t0\n",
			            prefix.c_str(), buf, suffix.c_str(),
			            move_to_kif(moves[0]).c_str(),
			            rap_time / 1000.0,
			            pos.nodes_searched(), pos.tnodes_searched(), nps);
		}
	}

	time = now() - time;

	cerr << "\n==============================="
	     << "\nTotal time (ms) : " << time
	     << "\nNodes searched  : " << totalNodes
	     << "\nTNodes searched : " << totalTNodes
	     << "\nNodes/sec(all)  : " << static_cast<int>((totalNodes+totalTNodes) / (time / 1000.0)) << endl;

	if (bOut) {
		fclose(fp);
	}
}

// 静止探索のテスト.
void test_see(int argc, char* argv[])
{
	vector<string> sfenList;
	string sfenFile = argc > 2 ? argv[2] : "default";

	// Do we need to load positions from a given SFEN file ?
	if (sfenFile != "default") {
		string fen;
		ifstream f(sfenFile.c_str());

		if (!f.is_open())
		{
			cerr << "Unable to open file " << sfenFile << endl;
			exit(EXIT_FAILURE);
		}

		while (getline(f, fen)) {
			if (!fen.empty()) {
				sfenList.push_back(fen);
			}
		}
		f.close();
	} else {
		for (int i = 0; !tc_see[i].empty(); i++) {
			sfenList.push_back(tc_see[i]);
		}
	}

	static const char *piece_str[] = {
		"**", "FU", "KY", "KE", "GI", "KI", "KA", "HI",
		"OU", "TO", "NY", "NK", "NG", "++", "UM", "RY",
	};
	MoveStack ss1[MAX_MOVES];
	MoveStack ss2[MAX_MOVES];
	for (size_t i = 0; i < sfenList.size(); i++)
	{
		Position pos(sfenList[i], Threads.main());

		// 指し手生成のテスト
		//  合法手の数＝captureの数＋non-captureの数になるか？
		MoveStack *legal = generate<MV_LEGAL>(pos, ss1);
		MoveStack *mcap  = generate<MV_CAPTURE>(pos, ss2);
		MoveStack *mall  = generate<MV_NON_CAPTURE>(pos, mcap);
		if (legal - ss1 != mall - ss2) {
			cerr << "Error!:legal=" << legal-ss1 << ", capture=" << mcap - ss2 << ", noncapture=" << mall - mcap << endl;
			cerr << "Legal moves:\n";
			int j;
			for (j = 0; &ss1[j] != legal; j++) {
				cerr << move_to_csa(ss1[j].move) << " ";
			}
			cerr << "\nCapture moves:\n";
			for (j = 0; &ss2[j] != mcap; j++) {
				cerr << move_to_csa(ss2[j].move) << " ";
			}
			cerr << "\nCapture moves:\n";
			for (; &ss2[j] != mall; j++) {
				cerr << move_to_csa(ss2[j].move) << " ";
			}
			cerr << endl;
		}

		cerr << "\nBench position: " << i + 1 << '/' << sfenList.size() << "  cap moves:" << mcap - ss2  << "  non-cap moves:" << mall - mcap << endl;

		pos.print_csa();
		for (int j = 0; mall != &ss2[j]; j++) {
			Move m = ss2[j].move;
			int n = pos.see(m);
			if (move_captured(m) != EMP) {
				// 駒を取る
				if (is_promotion(m)) {
					cerr << "     " << move_to_csa(ss2[j].move) << "*: cap=";
				} else {
					cerr << "     " << move_to_csa(ss2[j].move) << " : cap=";
				}
				cerr << piece_str[type_of(move_captured(m))] << ",  see=" << n << endl;
			} else {
				// 駒を取らない
				if (is_promotion(m)) {
					cerr << "     " << move_to_csa(ss2[j].move) << "*: see=" << n << endl;
				} else if (n != 0) {
					cerr << "     " << move_to_csa(ss2[j].move) << " : see=" << n << endl;
				}
			}
		}
	}
}

void test_qsearch(int argc, char* argv[]) {
}

