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

#include <algorithm>
#include <cassert>
#include <cstring>   // For std::memset, std::memcmp
#include <iomanip>
#include <sstream>

#ifndef NANOHA
#include "bitcount.h"
#endif
#include "misc.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#ifdef NANOHA
#include <cstdio>
#include <iostream>
#include "param_apery.h"
#endif

using std::string;

#if defined(NANOHA)
const Value PieceValueMidgame[32] = {
	VALUE_ZERO,
	Value(DPawn   *2),
	Value(DLance  *2),
	Value(DKnight *2),
	Value(DSilver *2),
	Value(DGold   *2),
	Value(DBishop *2),
	Value(DRook   *2),
	Value(DKing   *2), 
	Value(DPawn   +DProPawn),
	Value(DLance  +DProLance),
	Value(DKnight +DProKnight),
	Value(DSilver +DProSilver),
	Value(DGold   *2),
	Value(DBishop +DHorse),
	Value(DRook   +DDragon),
	Value(DKing   *2), 
	Value(DPawn   *2),
	Value(DLance  *2),
	Value(DKnight *2),
	Value(DSilver *2),
	Value(DGold   *2),
	Value(DBishop *2),
	Value(DRook   *2),
	Value(DKing   *2), 
	Value(DPawn   +DProPawn),
	Value(DLance  +DProLance),
	Value(DKnight +DProKnight),
	Value(DSilver +DProSilver),
	Value(DGold   *2),
	Value(DBishop +DHorse),
	Value(DRook   +DDragon),
};
const Value PieceValueEndgame[32] = {
	VALUE_ZERO,
	Value(DPawn   *2),
	Value(DLance  *2),
	Value(DKnight *2),
	Value(DSilver *2),
	Value(DGold   *2),
	Value(DBishop *2),
	Value(DRook   *2),
	Value(DKing   *2), 
	Value(DPawn   +DProPawn),
	Value(DLance  +DProLance),
	Value(DKnight +DProKnight),
	Value(DSilver +DProSilver),
	Value(DGold   *2),
	Value(DBishop +DHorse),
	Value(DRook   +DDragon),
	Value(DKing   *2), 
	Value(DPawn   *2),
	Value(DLance  *2),
	Value(DKnight *2),
	Value(DSilver *2),
	Value(DGold   *2),
	Value(DBishop *2),
	Value(DRook   *2),
	Value(DKing   *2), 
	Value(DPawn   +DProPawn),
	Value(DLance  +DProLance),
	Value(DKnight +DProKnight),
	Value(DSilver +DProSilver),
	Value(DGold   *2),
	Value(DBishop +DHorse),
	Value(DRook   +DDragon),
};
unsigned char Position::DirTbl[0xA0][0x100];	// 方向用[from][to]
#else
Value PieceValue[PHASE_NB][PIECE_NB] = {
	{ VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg },
	{ VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg } };

#endif

namespace Zobrist {
#ifndef NANOHA
  Key psq[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
  Key enpassant[FILE_NB];
  Key castling[CASTLING_RIGHT_NB];
  Key side;
  Key exclusion;
#endif // !NANOHA
}

#if defined(NANOHA)
Key Position::zobrist[GRY + 1][0x100];
Key Position::zobSideToMove;	// 手番を区別する
Key Position::zobExclusion;		// NULL MOVEかどうか区別する
#endif

Key Position::exclusion_key() const { return st->key ^ zobExclusion; }

namespace {

#if defined(NANOHA)
	// To convert a Piece to and from a FEN char
	const string PieceToChar(".PLNSGBRKPLNS.BR.plnsgbrkplns.br");

	struct PieceType2Str : public std::map<PieceType, std::string> {
	PieceType2Str() {
		operator[](PIECE_TYPE_NONE) = "* ";
		operator[](FU) = "FU";
		operator[](KY) = "KY";
		operator[](KE) = "KE";
		operator[](GI) = "GI";
		operator[](KI) = "KI";
		operator[](KA) = "KA";
		operator[](HI) = "HI";
		operator[](OU) = "OU";
		operator[](TO) = "TO";
		operator[](NY) = "NY";
		operator[](NK) = "NK";
		operator[](NG) = "NG";
		operator[](UM) = "UM";
		operator[](RY) = "RY";
	}
	string from_piece(Piece p, bool bc=true) const {
		if (p == EMP) return string(" * ");

		Color c = color_of(p);
		string str = bc ? ((c == BLACK) ? "+" : "-") : "";
		std::map<PieceType, std::string>::const_iterator it;
		it = find(type_of(p));
		if (it == end()) {
			std::cerr << "Error!" << std::endl;
			abort();
		}
		str += it->second;
		return str;
	}
	};

	struct PieceLetters : public std::map<char, Piece> {

		PieceLetters() {
			operator[]('K') = SOU;	operator[]('k') = GOU;
			operator[]('R') = SHI;	operator[]('r') = GHI;
			operator[]('B') = SKA;	operator[]('b') = GKA;
			operator[]('G') = SKI;	operator[]('g') = GKI;
			operator[]('S') = SGI;	operator[]('s') = GGI;
			operator[]('N') = SKE;	operator[]('n') = GKE;
			operator[]('L') = SKY;	operator[]('l') = GKY;
			operator[]('P') = SFU;	operator[]('p') = GFU;
			operator[]('.') = EMP;
		}

		string from_piece(Piece p) const {
			if (p == SOU) return string("K");
			if (p == GOU) return string("k");

			std::map<char, Piece>::const_iterator it;
			for (it = begin(); it != end(); ++it)
			if (it->second == (int(p) & ~PROMOTED)) {
				return ((int(p) & PROMOTED) ? string("+") : string(""))+string(1,it->first);
			}

			assert(false);
			return 0;
		}
	};

	PieceLetters pieceLetters;
	PieceType2Str pieceCSA;
#else
const string PieceToChar(" PNBRQK  pnbrqk");
#endif

#ifndef NANOHA
// min_attacker() is a helper function used by see() to locate the least
// valuable attacker for the side to move, remove the attacker we just found
// from the bitboards and scan for new X-ray attacks behind it.

template<int Pt>
PieceType min_attacker(const Bitboard* bb, Square to, Bitboard stmAttackers,
                       Bitboard& occupied, Bitboard& attackers) {

  Bitboard b = stmAttackers & bb[Pt];
  if (!b)
      return min_attacker<Pt+1>(bb, to, stmAttackers, occupied, attackers);

  occupied ^= b & ~(b - 1);

  if (Pt == PAWN || Pt == BISHOP || Pt == QUEEN)
      attackers |= attacks_bb<BISHOP>(to, occupied) & (bb[BISHOP] | bb[QUEEN]);

  if (Pt == ROOK || Pt == QUEEN)
      attackers |= attacks_bb<ROOK>(to, occupied) & (bb[ROOK] | bb[QUEEN]);

  attackers &= occupied; // After X-ray that may add already processed pieces
  return (PieceType)Pt;
}

template<>
PieceType min_attacker<KING>(const Bitboard*, Square, Bitboard, Bitboard&, Bitboard&) {
  return KING; // No need to update bitboards: it is the last cycle
}
#endif
} // namespace


/// CheckInfo constructor
#ifndef NANOHA
CheckInfo::CheckInfo(const Position& pos) {

  Color them = ~pos.side_to_move();
  ksq = pos.square<KING>(them);

  pinned = pos.pinned_pieces(pos.side_to_move());
  dcCandidates = pos.discovered_check_candidates();

  checkSquares[PAWN]   = pos.attacks_from<PAWN>(ksq, them);
  checkSquares[KNIGHT] = pos.attacks_from<KNIGHT>(ksq);
  checkSquares[BISHOP] = pos.attacks_from<BISHOP>(ksq);
  checkSquares[ROOK]   = pos.attacks_from<ROOK>(ksq);
  checkSquares[QUEEN]  = checkSquares[BISHOP] | checkSquares[ROOK];
  checkSquares[KING]   = 0;
}
#endif

/// operator<<(Position) returns an ASCII representation of the position

#ifndef NANOHA
std::ostream& operator<<(std::ostream& os, const Position& pos) {

  os << "\n +---+---+---+---+---+---+---+---+\n";

  for (Rank r = RANK_8; r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= FILE_H; ++f)
          os << " | " << PieceToChar[pos.piece_on(make_square(f, r))];

      os << " |\n +---+---+---+---+---+---+---+---+\n";
  }

  os << "\nFen: " << pos.fen() << "\nKey: " << std::hex << std::uppercase
     << std::setfill('0') << std::setw(16) << pos.key() << std::dec << "\nCheckers: ";

  for (Bitboard b = pos.checkers(); b; )
      os << UCI::square(pop_lsb(&b)) << " ";

  return os;
}
#else
// 将棋用
std::ostream& operator<<(std::ostream& os, const Position& pos) {
	const char* dottedLine = "\n+---+---+---+---+---+---+---+---+---+\n";
	os << dottedLine;
	for (Rank rank = RANK_1; rank <= RANK_9; rank++)
	{
		os << dottedLine << '|';
		for (File file = FILE_9; file >= FILE_1; file--)
		{
			Square sq = make_square(file, rank);
			Piece piece = pos.piece_on(sq);

			char c = (color_of(pos.piece_on(sq)) == BLACK ? ' ' : ' ');
			os << c << pieceLetters.from_piece(piece) << c << '|';
		}
	}
	os << dottedLine << "Fen is: " << pos.fen() << "\nKey is: 0x" << std::hex << pos.key() << std::dec << std::endl;

	return os;
}
#endif


/// Position::init() initializes at startup the various arrays used to compute
/// hash keys.

void Position::init() {

  PRNG rng(1070372);
#ifdef NANOHA
  int j, k;
  for (j = 0; j < GRY + 1; j++) for (k = 0; k < 0x100; k++)
	  zobrist[j][k] = rng.rand<Key>() << 1;
#else
  for (Color c = WHITE; c <= BLACK; ++c)
      for (PieceType pt = PAWN; pt <= KING; ++pt)
          for (Square s = SQ_A1; s <= SQ_H8; ++s)
              Zobrist::psq[c][pt][s] = rng.rand<Key>();

  for (File f = FILE_A; f <= FILE_H; ++f)
      Zobrist::enpassant[f] = rng.rand<Key>();

  for (int cr = NO_CASTLING; cr <= ANY_CASTLING; ++cr)
  {
      Zobrist::castling[cr] = 0;
      Bitboard b = cr;
      while (b)
      {
          Key k = Zobrist::castling[1ULL << pop_lsb(&b)];
          Zobrist::castling[cr] ^= k ? k : rng.rand<Key>();
      }
  }
  Zobrist::side = rng.rand<Key>();
  Zobrist::exclusion = rng.rand<Key>();
#endif
  zobSideToMove = (rng.rand<Key>() <<1 ) | 1;
  zobExclusion = (rng.rand<Key>() << 1);
}


/// Position::operator=() creates a copy of 'pos' but detaching the state pointer
/// from the source to be self-consistent and not depending on any external data.

Position& Position::operator=(const Position& pos) {

  std::memcpy(this, &pos, sizeof(Position));
  std::memcpy(&startState, st, sizeof(StateInfo));
  st = &startState;
  nodes = 0;

  assert(pos_is_ok());

  return *this;
}


/// Position::clear() erases the position object to a pristine state, with an
/// empty board, white to move, and no castling rights.

void Position::clear() {
#ifndef NANOHA
	std::memset(this, 0, sizeof(Position));
	startState.epSquare = SQ_NONE;
	st = &startState;

	for (int i = 0; i < PIECE_TYPE_NB; ++i)
		for (int j = 0; j < 16; ++j)
			pieceList[WHITE][i][j] = pieceList[BLACK][i][j] = SQ_NONE;

#endif // !NANOHA
	std::memset(this, 0, sizeof(Position));
	st = &startState;
	memset(st, 0, sizeof(StateInfo));
	// 将棋はBLACKが先番.
	sideToMove = BLACK;
	tnodes = 0;
#if defined(CHK_PERFORM)
	count_Mate1plyDrop = 0;		// 駒打ちで詰んだ回数
	count_Mate1plyMove = 0;		// 駒移動で詰んだ回数
	count_Mate3ply = 0;			// Mate3()で詰んだ回数
#endif // defined(CHK_PERFORM)
#define FILL_ZERO(x)	memset(x, 0, sizeof(x))
	FILL_ZERO(banpadding);
	FILL_ZERO(ban);
	FILL_ZERO(komano);
	FILL_ZERO(effect);
	FILL_ZERO(pin);
	hand[0] = hand[1] = 0;
	FILL_ZERO(knkind);
	FILL_ZERO(knpos);
	material = 0;
	bInaniwa = false;
#undef FILL_ZERO
	nodes = 0;
}


/// Position::set() initializes the position object with the given FEN string.
/// This function is not very robust - make sure that input FENs are correct,
/// this is assumed to be the responsibility of the GUI.

#ifdef NANOHA
void Position::set(const string& fenStr, Thread* th) {
	char token;
	std::istringstream fen(fenStr);

	unsigned char tmp_ban[9][9] = { { '\0' } };
	int tmp_hand[GRY + 1] = { 0 };

	clear();
	fen >> std::noskipws;

	// 1. Piece placement
	int dan = 0;
	int suji = 0; // この「筋」というのは、普通とは反転したものになる。
	while ((fen >> token) && !isspace(token))
	{
		if (token == '+') {
			// 成り駒
			fen >> token;
			if (pieceLetters.find(token) == pieceLetters.end()) goto incorrect_fen;
			tmp_ban[dan][suji++] = static_cast<unsigned char>(pieceLetters[token] | PROMOTED);
		}
		else if (pieceLetters.find(token) != pieceLetters.end())
		{
			tmp_ban[dan][suji++] = static_cast<unsigned char>(pieceLetters[token]);
		}
		else if (isdigit(token)) {
			suji += (token - '0');	// 数字の分空白
		}
		else if (token == '/') {
			if (suji != 9) goto incorrect_fen;
			suji = 0;
			dan++;
		}
		else {
			goto incorrect_fen;
		}
		if (dan > 9 || suji > 9) goto incorrect_fen;
	}
	if (dan != 9 && suji != 9) goto incorrect_fen;

	// 手番取得
	if (!fen.get(token) || (token != 'w' && token != 'b')) goto incorrect_fen;
	sideToMove = (token == 'b') ? BLACK : WHITE;
	// スペース飛ばす
	if (!fen.get(token) || token != ' ') goto incorrect_fen;

	// 持ち駒
	while ((fen >> token) && token != ' ') {
		int num = 1;
		if (token == '-') {
			break;
		}
		else if (isdigit(token)) {
			num = token - '0';
			fen >> token;
			if (isdigit(token)) {
				num = 10 * num + token - '0';
				fen >> token;
			}
		}
		if (pieceLetters.find(token) == pieceLetters.end()) goto incorrect_fen;
		tmp_hand[pieceLetters[token]] = num;
	}
	init_position(tmp_ban, tmp_hand);
	fen >> std::skipws >> gamePly;
	
	// set_stateの中で行う
	// st->key = compute_key();

	st->hand = hand[sideToMove].h;
	st->effect = (sideToMove == BLACK) ? effectB[kingG] : effectW[kingS];
	material = compute_material();

	thisThread = th;
	set_state(st);

	assert(pos_is_ok());
	return;

incorrect_fen:
	std::cerr << "Error in SFEN string: " << fenStr << std::endl;
}
#else
void Position::set(const string& fenStr, bool isChess960, Thread* th) {
	/*
	A FEN string defines a particular position using only the ASCII character set.

	A FEN string contains six fields separated by a space. The fields are:

	1) Piece placement (from white's perspective). Each rank is described, starting
	with rank 8 and ending with rank 1. Within each rank, the contents of each
	square are described from file A through file H. Following the Standard
	Algebraic Notation (SAN), each piece is identified by a single letter taken
	from the standard English names. White pieces are designated using upper-case
	letters ("PNBRQK") whilst Black uses lowercase ("pnbrqk"). Blank squares are
	noted using digits 1 through 8 (the number of blank squares), and "/"
	separates ranks.

	2) Active color. "w" means white moves next, "b" means black.

	3) Castling availability. If neither side can castle, this is "-". Otherwise,
	this has one or more letters: "K" (White can castle kingside), "Q" (White
	can castle queenside), "k" (Black can castle kingside), and/or "q" (Black
	can castle queenside).

	4) En passant target square (in algebraic notation). If there's no en passant
	target square, this is "-". If a pawn has just made a 2-square move, this
	is the position "behind" the pawn. This is recorded regardless of whether
	there is a pawn in position to make an en passant capture.

	5) Halfmove clock. This is the number of halfmoves since the last pawn advance
	or capture. This is used to determine if a draw can be claimed under the
	fifty-move rule.

	6) Fullmove number. The number of the full move. It starts at 1, and is
	incremented after Black's move.
	*/

	unsigned char col, row, token;
	size_t idx;
	Square sq = SQ_A8;
	std::istringstream ss(fenStr);

	clear();
	ss >> std::noskipws;

	// 1. Piece placement
	while ((ss >> token) && !isspace(token))
	{
		if (isdigit(token))
			sq += Square(token - '0'); // Advance the given number of files

		else if (token == '/')
			sq -= Square(16);

		else if ((idx = PieceToChar.find(token)) != string::npos)
		{
			put_piece(color_of(Piece(idx)), type_of(Piece(idx)), sq);
			++sq;
		}
	}

	// 2. Active color
	ss >> token;
	sideToMove = (token == 'w' ? WHITE : BLACK);
	ss >> token;

	// 3. Castling availability. Compatible with 3 standards: Normal FEN standard,
	// Shredder-FEN that uses the letters of the columns on which the rooks began
	// the game instead of KQkq and also X-FEN standard that, in case of Chess960,
	// if an inner rook is associated with the castling right, the castling tag is
	// replaced by the file letter of the involved rook, as for the Shredder-FEN.
	while ((ss >> token) && !isspace(token))
	{
		Square rsq;
		Color c = islower(token) ? BLACK : WHITE;
		Piece rook = make_piece(c, ROOK);

		token = char(toupper(token));

		if (token == 'K')
			for (rsq = relative_square(c, SQ_H1); piece_on(rsq) != rook; --rsq) {}

		else if (token == 'Q')
			for (rsq = relative_square(c, SQ_A1); piece_on(rsq) != rook; ++rsq) {}

		else if (token >= 'A' && token <= 'H')
			rsq = make_square(File(token - 'A'), relative_rank(c, RANK_1));

		else
			continue;

		set_castling_right(c, rsq);
	}

	// 4. En passant square. Ignore if no pawn capture is possible
	if (((ss >> col) && (col >= 'a' && col <= 'h'))
		&& ((ss >> row) && (row == '3' || row == '6')))
	{
		st->epSquare = make_square(File(col - 'a'), Rank(row - '1'));

		if (!(attackers_to(st->epSquare) & pieces(sideToMove, PAWN)))
			st->epSquare = SQ_NONE;
	}

	// 5-6. Halfmove clock and fullmove number
	ss >> std::skipws >> st->rule50 >> gamePly;

	// Convert from fullmove starting from 1 to ply starting from 0,
	// handle also common incorrect FEN with fullmove = 0.
	gamePly = std::max(2 * (gamePly - 1), 0) + (sideToMove == BLACK);

	chess960 = isChess960;
	thisThread = th;
	set_state(st);

	assert(pos_is_ok());
}
#endif

#ifndef NANOHA
/// Position::set_castling_right() is a helper function used to set castling
/// rights given the corresponding color and the rook starting square.

void Position::set_castling_right(Color c, Square rfrom) {

	Square kfrom = square<KING>(c);
	CastlingSide cs = kfrom < rfrom ? KING_SIDE : QUEEN_SIDE;
	CastlingRight cr = (c | cs);

	st->castlingRights |= cr;
	castlingRightsMask[kfrom] |= cr;
	castlingRightsMask[rfrom] |= cr;
	castlingRookSquare[cr] = rfrom;

	Square kto = relative_square(c, cs == KING_SIDE ? SQ_G1 : SQ_C1);
	Square rto = relative_square(c, cs == KING_SIDE ? SQ_F1 : SQ_D1);

	for (Square s = std::min(rfrom, rto); s <= std::max(rfrom, rto); ++s)
		if (s != kfrom && s != rfrom)
			castlingPath[cr] |= s;

	for (Square s = std::min(kfrom, kto); s <= std::max(kfrom, kto); ++s)
		if (s != kfrom && s != rfrom)
			castlingPath[cr] |= s;
}

#endif // !NANOHA



/// Position::set_state() computes the hash keys of the position, and other
/// data that once computed is updated incrementally as moves are made.
/// The function is only used when a new position is set up, and to verify
/// the correctness of the StateInfo data when running in debug mode.
#ifdef NANOHA
Key Position::compute_key() const {
	Key k = 0;
	int z;
	for (int dan = 1; dan <= 9; dan++) {
		for (int suji = 0x10; suji <= 0x90; suji += 0x10) {
			z = suji + dan;
			if (!empty(Square(z))) {
				k ^= zobrist[piece_on(Square(z))][z];
			}
		}
	}
	if (side_to_move() != BLACK)
		k ^= zobSideToMove;

	return k;
}

void Position::set_state(StateInfo* si) const {
	si->key = compute_key();
}
#else
void Position::set_state(StateInfo* si) const {

  si->key = si->pawnKey = si->materialKey = 0;
  si->nonPawnMaterial[WHITE] = si->nonPawnMaterial[BLACK] = VALUE_ZERO;
  si->psq = SCORE_ZERO;

  si->checkersBB = attackers_to(square<KING>(sideToMove)) & pieces(~sideToMove);

  for (Bitboard b = pieces(); b; )
  {
      Square s = pop_lsb(&b);
      Piece pc = piece_on(s);
      si->key ^= Zobrist::psq[color_of(pc)][type_of(pc)][s];
      si->psq += PSQT::psq[color_of(pc)][type_of(pc)][s];
  }

  if (si->epSquare != SQ_NONE)
      si->key ^= Zobrist::enpassant[file_of(si->epSquare)];

  if (sideToMove == BLACK)
      si->key ^= Zobrist::side;

  si->key ^= Zobrist::castling[si->castlingRights];

  for (Bitboard b = pieces(PAWN); b; )
  {
      Square s = pop_lsb(&b);
      si->pawnKey ^= Zobrist::psq[color_of(piece_on(s))][PAWN][s];
  }

  for (Color c = WHITE; c <= BLACK; ++c)
      for (PieceType pt = PAWN; pt <= KING; ++pt)
          for (int cnt = 0; cnt < pieceCount[c][pt]; ++cnt)
              si->materialKey ^= Zobrist::psq[c][pt][cnt];

  for (Color c = WHITE; c <= BLACK; ++c)
      for (PieceType pt = KNIGHT; pt <= QUEEN; ++pt)
          si->nonPawnMaterial[c] += pieceCount[c][pt] * PieceValue[MG][pt];
}
#endif


#ifdef NANOHA
/// Position::fen() returns a FEN representation of the position.
const string Position::fen() const {
	std::ostringstream ss;
	Square sq;
	int emptyCnt;

	for (Rank rank = RANK_1; rank <= RANK_9; rank++)
	{
		emptyCnt = 0;
		for (File file = FILE_9; file >= FILE_1; file--)
		{
			sq = make_square(file, rank);

			if (!empty(sq))
			{
				if (emptyCnt)
				{
					ss << emptyCnt;
					emptyCnt = 0;
				}
				if ((piece_on(sq) - 1) & PROMOTED) {
					ss << "+";
				}
				ss << PieceToChar[piece_on(sq)];
			}
			else {
				emptyCnt++;
			}
		}

		if (emptyCnt)
			ss << emptyCnt;

		if (rank < RANK_9)
			ss << '/';
	}

	ss << (sideToMove == WHITE ? " w " : " b ");

	// 持ち駒
	if (handS.h == 0 && handG.h == 0) {
		ss << "-";
	}
	else {
		unsigned int n;
		static const char *tbl[] = {
			"",   "",   "2",  "3",  "4",  "5",  "6",  "7",
			"8",  "9", "10", "11", "12", "13", "14", "15",
			"16", "17", "18"
		};
#define ADD_HAND(piece,c)	n = h.get ## piece(); if (n) {ss << tbl[n]; ss << #c; }

		Hand h = handS;
		ADD_HAND(HI, R)
		ADD_HAND(KA, B)
		ADD_HAND(KI, G)
		ADD_HAND(GI, S)
		ADD_HAND(KE, N)
		ADD_HAND(KY, L)
		ADD_HAND(FU, P)

		h = handG;
		ADD_HAND(HI, r)
		ADD_HAND(KA, b)
		ADD_HAND(KI, g)
		ADD_HAND(GI, s)
		ADD_HAND(KE, n)
		ADD_HAND(KY, l)
		ADD_HAND(FU, p)

#undef ADD_HAND
	}
	return ss.str();
}
#else
/// Position::fen() returns a FEN representation of the position. In case of
/// Chess960 the Shredder-FEN notation is used. This is mainly a debugging function.
const string Position::fen() const {

  int emptyCnt;
  std::ostringstream ss;

  for (Rank r = RANK_8; r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= FILE_H; ++f)
      {
          for (emptyCnt = 0; f <= FILE_H && empty(make_square(f, r)); ++f)
              ++emptyCnt;

          if (emptyCnt)
              ss << emptyCnt;

          if (f <= FILE_H)
              ss << PieceToChar[piece_on(make_square(f, r))];
      }

      if (r > RANK_1)
          ss << '/';
  }

  ss << (sideToMove == WHITE ? " w " : " b ");

  if (can_castle(WHITE_OO))
      ss << (chess960 ? char('A' + file_of(castling_rook_square(WHITE |  KING_SIDE))) : 'K');

  if (can_castle(WHITE_OOO))
      ss << (chess960 ? char('A' + file_of(castling_rook_square(WHITE | QUEEN_SIDE))) : 'Q');

  if (can_castle(BLACK_OO))
      ss << (chess960 ? char('a' + file_of(castling_rook_square(BLACK |  KING_SIDE))) : 'k');

  if (can_castle(BLACK_OOO))
      ss << (chess960 ? char('a' + file_of(castling_rook_square(BLACK | QUEEN_SIDE))) : 'q');

  if (!can_castle(WHITE) && !can_castle(BLACK))
      ss << '-';

  ss << (ep_square() == SQ_NONE ? " - " : " " + UCI::square(ep_square()) + " ")
     << st->rule50 << " " << 1 + (gamePly - (sideToMove == BLACK)) / 2;

  return ss.str();
}
#endif

#ifndef NANOHA
/// Position::game_phase() calculates the game phase interpolating total non-pawn
/// material between endgame and midgame limits.

Phase Position::game_phase() const {

  Value npm = st->nonPawnMaterial[WHITE] + st->nonPawnMaterial[BLACK];

  npm = std::max(EndgameLimit, std::min(npm, MidgameLimit));

  return Phase(((npm - EndgameLimit) * PHASE_MIDGAME) / (MidgameLimit - EndgameLimit));
}
#endif

#ifndef NANOHA

/// Position::check_blockers() returns a bitboard of all the pieces with color
/// 'c' that are blocking check on the king with color 'kingColor'. A piece
/// blocks a check if removing that piece from the board would result in a
/// position where the king is in check. A check blocking piece can be either a
/// pinned or a discovered check piece, according if its color 'c' is the same
/// or the opposite of 'kingColor'.

Bitboard Position::check_blockers(Color c, Color kingColor) const {

  Bitboard b, pinners, result = 0;
  Square ksq = square<KING>(kingColor);

  // Pinners are sliders that give check when a pinned piece is removed
  pinners = (  (pieces(  ROOK, QUEEN) & PseudoAttacks[ROOK  ][ksq])
             | (pieces(BISHOP, QUEEN) & PseudoAttacks[BISHOP][ksq])) & pieces(~kingColor);

  while (pinners)
  {
      b = between_bb(ksq, pop_lsb(&pinners)) & pieces();

      if (!more_than_one(b))
          result |= b & pieces(c);
  }
  return result;
}


/// Position::attackers_to() computes a bitboard of all pieces which attack a
/// given square. Slider attacks use the occupied bitboard to indicate occupancy.

Bitboard Position::attackers_to(Square s, Bitboard occupied) const {

  return  (attacks_from<PAWN>(s, BLACK)    & pieces(WHITE, PAWN))
        | (attacks_from<PAWN>(s, WHITE)    & pieces(BLACK, PAWN))
        | (attacks_from<KNIGHT>(s)         & pieces(KNIGHT))
        | (attacks_bb<ROOK  >(s, occupied) & pieces(ROOK,   QUEEN))
        | (attacks_bb<BISHOP>(s, occupied) & pieces(BISHOP, QUEEN))
        | (attacks_from<KING>(s)           & pieces(KING));
}


#endif // !NANOHA

// pl_is_legal/is_pseudo_legalに移動。is_pseudo_legalはpl_is_legalを呼び出しているだけ
#ifndef NANOHA
/// Position::legal() tests whether a pseudo-legal move is legal
bool Position::legal(Move m, Bitboard pinned) const {

	assert(is_ok(m));
	assert(pinned == pinned_pieces(sideToMove));

	Color us = sideToMove;
	Square from = from_sq(m);

	assert(color_of(moved_piece(m)) == us);
	assert(piece_on(square<KING>(us)) == make_piece(us, KING));

	// En passant captures are a tricky special case. Because they are rather
	// uncommon, we do it simply by testing whether the king is attacked after
	// the move is made.
	if (type_of(m) == ENPASSANT)
	{
		Square ksq = square<KING>(us);
		Square to = to_sq(m);
		Square capsq = to - pawn_push(us);
		Bitboard occupied = (pieces() ^ from ^ capsq) | to;

		assert(to == ep_square());
		assert(moved_piece(m) == make_piece(us, PAWN));
		assert(piece_on(capsq) == make_piece(~us, PAWN));
		assert(piece_on(to) == NO_PIECE);

		return   !(attacks_bb<  ROOK>(ksq, occupied) & pieces(~us, QUEEN, ROOK))
			&& !(attacks_bb<BISHOP>(ksq, occupied) & pieces(~us, QUEEN, BISHOP));
	}

	// If the moving piece is a king, check whether the destination
	// square is attacked by the opponent. Castling moves are checked
	// for legality during move generation.
	if (type_of(piece_on(from)) == KING)
		return type_of(m) == CASTLING || !(attackers_to(to_sq(m)) & pieces(~us));

	// A non-king move is legal if and only if it is not pinned or it
	// is moving along the ray towards or away from the king.
	return   !pinned
		|| !(pinned & from)
		|| aligned(from, to_sq(m), square<KING>(us));
}
/// Position::pseudo_legal() takes a random move and tests whether the move is
/// pseudo legal. It is used to validate moves from TT that can be corrupted
/// due to SMP concurrent access or hash position key aliasing.

bool Position::pseudo_legal(const Move m) const {

	Color us = sideToMove;
	Square from = from_sq(m);
	Square to = to_sq(m);
	Piece pc = moved_piece(m);

	// Use a slower but simpler function for uncommon cases
	if (type_of(m) != NORMAL)
		return MoveList<LEGAL>(*this).contains(m);

	// Is not a promotion, so promotion piece must be empty
	if (promotion_type(m) - KNIGHT != NO_PIECE_TYPE)
		return false;

	// If the 'from' square is not occupied by a piece belonging to the side to
	// move, the move is obviously not legal.
	if (pc == NO_PIECE || color_of(pc) != us)
		return false;

	// The destination square cannot be occupied by a friendly piece
	if (pieces(us) & to)
		return false;

	// Handle the special case of a pawn move
	if (type_of(pc) == PAWN)
	{
		// We have already handled promotion moves, so destination
		// cannot be on the 8th/1st rank.
		if (rank_of(to) == relative_rank(us, RANK_8))
			return false;

		if (!(attacks_from<PAWN>(from, us) & pieces(~us) & to) // Not a capture
			&& !((from + pawn_push(us) == to) && empty(to))       // Not a single push
			&& !((from + 2 * pawn_push(us) == to)              // Not a double push
				&& (rank_of(from) == relative_rank(us, RANK_2))
				&& empty(to)
				&& empty(to - pawn_push(us))))
			return false;
	}
	else if (!(attacks_from(pc, from) & to))
		return false;

	// Evasions generator already takes care to avoid some kind of illegal moves
	// and legal() relies on this. We therefore have to take care that the same
	// kind of moves are filtered out here.
	if (checkers())
	{
		if (type_of(pc) != KING)
		{
			// Double check? In this case a king move is required
			if (more_than_one(checkers()))
				return false;

			// Our move must be a blocking evasion or a capture of the checking piece
			if (!((between_bb(lsb(checkers()), square<KING>(us)) | checkers()) & to))
				return false;
		}
		// In case of king moves under check we have to remove king so as to catch
		// invalid moves like b1a1 when opposite queen is on c1.
		else if (attackers_to(to, pieces() ^ from) & pieces(~us))
			return false;
	}

	return true;
}
#endif // !NANOHA

#ifndef NANOHA
/// Position::gives_check() tests whether a pseudo-legal move gives a check

bool Position::gives_check(Move m, const CheckInfo& ci) const {

	assert(is_ok(m));
	assert(ci.dcCandidates == discovered_check_candidates());
	assert(color_of(moved_piece(m)) == sideToMove);

	Square from = from_sq(m);
	Square to = to_sq(m);

	// Is there a direct check?
	if (ci.checkSquares[type_of(piece_on(from))] & to)
		return true;

	// Is there a discovered check?
	if (ci.dcCandidates
		&& (ci.dcCandidates & from)
		&& !aligned(from, to, ci.ksq))
		return true;

	switch (type_of(m))
	{
	case NORMAL:
		return false;

	case PROMOTION:
		return attacks_bb(Piece(promotion_type(m)), to, pieces() ^ from) & ci.ksq;

		// En passant capture with check? We have already handled the case
		// of direct checks and ordinary discovered check, so the only case we
		// need to handle is the unusual case of a discovered check through
		// the captured pawn.
	case ENPASSANT:
	{
		Square capsq = make_square(file_of(to), rank_of(from));
		Bitboard b = (pieces() ^ from ^ capsq) | to;

		return  (attacks_bb<  ROOK>(ci.ksq, b) & pieces(sideToMove, QUEEN, ROOK))
			| (attacks_bb<BISHOP>(ci.ksq, b) & pieces(sideToMove, QUEEN, BISHOP));
	}
	case CASTLING:
	{
		Square kfrom = from;
		Square rfrom = to; // Castling is encoded as 'King captures the rook'
		Square kto = relative_square(sideToMove, rfrom > kfrom ? SQ_G1 : SQ_C1);
		Square rto = relative_square(sideToMove, rfrom > kfrom ? SQ_F1 : SQ_D1);

		return   (PseudoAttacks[ROOK][rto] & ci.ksq)
			&& (attacks_bb<ROOK>(rto, (pieces() ^ kfrom ^ rfrom) | rto | kto) & ci.ksq);
	}
	default:
		assert(false);
		return false;
	}
}
#endif // !NANOHA


// NANOHAが定義されている時には、shogi.cppのdo_move/undo_moveを使う
#ifndef NANOHA
/// Position::do_move() makes a move, and saves all information necessary
/// to a StateInfo object. The move is assumed to be legal. Pseudo-legal
/// moves should be filtered out before this function is called.

void Position::do_move(Move m, StateInfo& newSt, bool givesCheck) {

  assert(is_ok(m));
  assert(&newSt != st);

  ++nodes;
  Key k = st->key ^ Zobrist::side;

  // Copy some fields of the old state to our new StateInfo object except the
  // ones which are going to be recalculated from scratch anyway and then switch
  // our state pointer to point to the new (ready to be updated) state.
  std::memcpy(&newSt, st, offsetof(StateInfo, key));
  newSt.previous = st;
  st = &newSt;

  // Increment ply counters. In particular, rule50 will be reset to zero later on
  // in case of a capture or a pawn move.
  ++gamePly;
  ++st->rule50;
  ++st->pliesFromNull;

  Color us = sideToMove;
  Color them = ~us;
  Square from = from_sq(m);
  Square to = to_sq(m);
  PieceType pt = type_of(piece_on(from));
  PieceType captured = type_of(m) == ENPASSANT ? PAWN : type_of(piece_on(to));

  assert(color_of(piece_on(from)) == us);
  assert(piece_on(to) == NO_PIECE || color_of(piece_on(to)) == (type_of(m) != CASTLING ? them : us));
  assert(captured != KING);

  if (type_of(m) == CASTLING)
  {
      assert(pt == KING);

      Square rfrom, rto;
      do_castling<true>(us, from, to, rfrom, rto);

      captured = NO_PIECE_TYPE;
      st->psq += PSQT::psq[us][ROOK][rto] - PSQT::psq[us][ROOK][rfrom];
      k ^= Zobrist::psq[us][ROOK][rfrom] ^ Zobrist::psq[us][ROOK][rto];
  }

  if (captured)
  {
      Square capsq = to;

      // If the captured piece is a pawn, update pawn hash key, otherwise
      // update non-pawn material.
      if (captured == PAWN)
      {
          if (type_of(m) == ENPASSANT)
          {
              capsq -= pawn_push(us);

              assert(pt == PAWN);
              assert(to == st->epSquare);
              assert(relative_rank(us, to) == RANK_6);
              assert(piece_on(to) == NO_PIECE);
              assert(piece_on(capsq) == make_piece(them, PAWN));

              board[capsq] = NO_PIECE; // Not done by remove_piece()
          }

          st->pawnKey ^= Zobrist::psq[them][PAWN][capsq];
      }
      else
          st->nonPawnMaterial[them] -= PieceValue[MG][captured];

      // Update board and piece lists
      remove_piece(them, captured, capsq);

      // Update material hash key and prefetch access to materialTable
      k ^= Zobrist::psq[them][captured][capsq];
      st->materialKey ^= Zobrist::psq[them][captured][pieceCount[them][captured]];
      prefetch(thisThread->materialTable[st->materialKey]);

      // Update incremental scores
      st->psq -= PSQT::psq[them][captured][capsq];

      // Reset rule 50 counter
      st->rule50 = 0;
  }

  // Update hash key
  k ^= Zobrist::psq[us][pt][from] ^ Zobrist::psq[us][pt][to];

  // Reset en passant square
  if (st->epSquare != SQ_NONE)
  {
      k ^= Zobrist::enpassant[file_of(st->epSquare)];
      st->epSquare = SQ_NONE;
  }

  // Update castling rights if needed
  if (st->castlingRights && (castlingRightsMask[from] | castlingRightsMask[to]))
  {
      int cr = castlingRightsMask[from] | castlingRightsMask[to];
      k ^= Zobrist::castling[st->castlingRights & cr];
      st->castlingRights &= ~cr;
  }

  // Move the piece. The tricky Chess960 castling is handled earlier
  if (type_of(m) != CASTLING)
      move_piece(us, pt, from, to);

  // If the moving piece is a pawn do some special extra work
  if (pt == PAWN)
  {
      // Set en-passant square if the moved pawn can be captured
      if (   (int(to) ^ int(from)) == 16
          && (attacks_from<PAWN>(to - pawn_push(us), us) & pieces(them, PAWN)))
      {
          st->epSquare = (from + to) / 2;
          k ^= Zobrist::enpassant[file_of(st->epSquare)];
      }

      else if (type_of(m) == PROMOTION)
      {
          PieceType promotion = promotion_type(m);

          assert(relative_rank(us, to) == RANK_8);
          assert(promotion >= KNIGHT && promotion <= QUEEN);

          remove_piece(us, PAWN, to);
          put_piece(us, promotion, to);

          // Update hash keys
          k ^= Zobrist::psq[us][PAWN][to] ^ Zobrist::psq[us][promotion][to];
          st->pawnKey ^= Zobrist::psq[us][PAWN][to];
          st->materialKey ^=  Zobrist::psq[us][promotion][pieceCount[us][promotion]-1]
                            ^ Zobrist::psq[us][PAWN][pieceCount[us][PAWN]];

          // Update incremental score
          st->psq += PSQT::psq[us][promotion][to] - PSQT::psq[us][PAWN][to];

          // Update material
          st->nonPawnMaterial[us] += PieceValue[MG][promotion];
      }

      // Update pawn hash key and prefetch access to pawnsTable
      st->pawnKey ^= Zobrist::psq[us][PAWN][from] ^ Zobrist::psq[us][PAWN][to];
      prefetch(thisThread->pawnsTable[st->pawnKey]);

      // Reset rule 50 draw counter
      st->rule50 = 0;
  }

  // Update incremental scores
  st->psq += PSQT::psq[us][pt][to] - PSQT::psq[us][pt][from];

  // Set capture piece
  st->capturedType = captured;

  // Update the key with the final value
  st->key = k;

  // Calculate checkers bitboard (if move gives check)
  st->checkersBB = givesCheck ? attackers_to(square<KING>(them)) & pieces(us) : 0;

  sideToMove = ~sideToMove;

  assert(pos_is_ok());
}


/// Position::undo_move() unmakes a move. When it returns, the position should
/// be restored to exactly the same state as before the move was made.

void Position::undo_move(Move m) {

  assert(is_ok(m));

  sideToMove = ~sideToMove;

  Color us = sideToMove;
  Square from = from_sq(m);
  Square to = to_sq(m);
  PieceType pt = type_of(piece_on(to));

  assert(empty(from) || type_of(m) == CASTLING);
  assert(st->capturedType != KING);

  if (type_of(m) == PROMOTION)
  {
      assert(relative_rank(us, to) == RANK_8);
      assert(pt == promotion_type(m));
      assert(pt >= KNIGHT && pt <= QUEEN);

      remove_piece(us, pt, to);
      put_piece(us, PAWN, to);
      pt = PAWN;
  }

  if (type_of(m) == CASTLING)
  {
      Square rfrom, rto;
      do_castling<false>(us, from, to, rfrom, rto);
  }
  else
  {
      move_piece(us, pt, to, from); // Put the piece back at the source square

      if (st->capturedType)
      {
          Square capsq = to;

          if (type_of(m) == ENPASSANT)
          {
              capsq -= pawn_push(us);

              assert(pt == PAWN);
              assert(to == st->previous->epSquare);
              assert(relative_rank(us, to) == RANK_6);
              assert(piece_on(capsq) == NO_PIECE);
              assert(st->capturedType == PAWN);
          }

          put_piece(~us, st->capturedType, capsq); // Restore the captured piece
      }
  }

  // Finally point our state pointer back to the previous state
  st = st->previous;
  --gamePly;

  assert(pos_is_ok());
}
#endif

// 将棋にはcastlingはない
#ifndef NANOHA
/// Position::do_castling() is a helper used to do/undo a castling move. This
/// is a bit tricky, especially in Chess960.
template<bool Do>
void Position::do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto) {

	bool kingSide = to > from;
	rfrom = to; // Castling is encoded as "king captures friendly rook"
	rto = relative_square(us, kingSide ? SQ_F1 : SQ_D1);
	to = relative_square(us, kingSide ? SQ_G1 : SQ_C1);

	// Remove both pieces first since squares could overlap in Chess960
	remove_piece(us, KING, Do ? from : to);
	remove_piece(us, ROOK, Do ? rfrom : rto);
	board[Do ? from : to] = board[Do ? rfrom : rto] = NO_PIECE; // Since remove_piece doesn't do it for us
	put_piece(us, KING, Do ? to : from);
	put_piece(us, ROOK, Do ? rto : rfrom);
}
#endif // !NANOHA



/// Position::do(undo)_null_move() is used to do(undo) a "null move": It flips
/// the side to move without executing any move on the board.

void Position::do_null_move(StateInfo& newSt) {

	assert(!in_check());
	assert(&newSt != st);

	std::memcpy(&newSt, st, sizeof(StateInfo));
	newSt.previous = st;
	st = &newSt;

	// Back up the information necessary to undo the null move to the supplied
	// StateInfo object.
	// Note that differently from normal case here backupSt is actually used as
	// a backup storage not as a new state to be used.
#if !defined(NANOHA)
	newSt.epSquare = st->epSquare;
	newSt.value = st->value;
#endif

#if !defined(NANOHA)
	// Update the necessary information
	if (st->epSquare != SQ_NONE)
		st->key ^= zobEp[st->epSquare];
#endif

	st->key ^= zobSideToMove;
	prefetch((char*)TT.first_entry(st->key));

	sideToMove = flip(sideToMove);
#if !defined(NANOHA)
	st->epSquare = SQ_NONE;
	st->rule50++;
#endif
	st->pliesFromNull = 0;
#if !defined(NANOHA)
	st->value += (sideToMove == WHITE) ? TempoValue : -TempoValue;
#endif
#if defined(NANOHA)
	if (in_check()) {
		print_csa();
		MYABORT();
	}
#endif
	assert(pos_is_ok());
}


/// Position::undo_null_move() unmakes a "null move".

void Position::undo_null_move() {

#if defined(NANOHA)
	if (in_check()) {
		print_csa();
		MYABORT();
	}
#else
	assert(!checkers());
#endif

	// Restore information from the our backup StateInfo object
	st = st->previous;
	sideToMove = flip(sideToMove);
	assert(pos_is_ok());
}

/// Position::key_after() computes the new hash key after the given move. Needed
/// for speculative prefetch. It doesn't recognize special moves like castling,
/// en-passant and promotions.
#ifdef NANOHA
Key Position::key_after(Move m) const {
	Color us = sideToMove;
	Square from = from_sq(m);
	Square to = to_sq(m);
	Piece pt = piece_on(from);
	Piece captured = piece_on(to);


	Key k = st->key ^ zobSideToMove;

	if (captured != EMP) {
		k ^= zobrist[captured][to];
	}

	return k ^ zobrist[pt][to] ^ zobrist[pt][from];
}
#else
Key Position::key_after(Move m) const {

	Color us = sideToMove;
	Square from = from_sq(m);
	Square to = to_sq(m);
	PieceType pt = type_of(piece_on(from));
	PieceType captured = type_of(piece_on(to));
	Key k = st->key ^ Zobrist::side;

	if (captured)
		k ^= Zobrist::psq[~us][captured][to];

	return k ^ Zobrist::psq[us][pt][to] ^ Zobrist::psq[us][pt][from];
}
#endif



/// Position::see() is a static exchange evaluator: It tries to estimate the
/// material gain or loss resulting from a move.
#if defined(NANOHA)
static int SEERec(int kind, int *attacker, int *defender)
{
	//  手番の一番安い駒を動かす
	//  value = 取った駒の交換値(ふつうの2枚分)
	if (*attacker == 0) return 0;

	int value = NanohaTbl::KomaValueEx[kind];
	return Max(value - SEERec(*attacker, defender, attacker + 1), 0);
}
#endif

Value Position::see_sign(Move m) const {

  assert(is_ok(m));
#ifdef NANOHA
  Square from = move_from(m);
  Square to = move_to(m);
#endif

  // Early return if SEE cannot be negative because captured piece value
  // is not less then capturing one. Note that king moves always return
  // here because king midgame value is set to 0.
#if defined(NANOHA)
  if (move_is_drop(m) == false && PieceValueMidgame[piece_on(to)] >= PieceValueMidgame[piece_on(from)])
	  return (Value)1;

  return see(m);
#else
  if (PieceValue[MG][moved_piece(m)] <= PieceValue[MG][piece_on(to_sq(m))])
	  return VALUE_KNOWN_WIN;

  return see(m);
#endif
}

Value Position::see(Move m) const {
#if defined(NANOHA)
	//  value = 成りによる加点 + 取りによる加点;
	int from = move_from(m);
	int to = move_to(m);
	int fKind = move_ptype(m);
	int tKind = is_promotion(m) ? Piece(fKind | PROMOTED) : fKind;
	int cap = move_captured(m) & ~GOTE;
	Color us = side_to_move();
	int value = is_promotion(m) ? NanohaTbl::KomaValuePro[fKind] + NanohaTbl::KomaValueEx[cap] : NanohaTbl::KomaValueEx[cap]; // = (promote + capture) value

																															  //
	const effect_t *dKiki = (us == BLACK) ? effectW : effectB;
	int defender[32];	// to に利いている守りの駒
	int ndef = 0;
	effect_t k = EXIST_EFFECT(dKiki[to]);
	while (k) {
		unsigned long id;
		_BitScanForward(&id, k);
		k &= k - 1;
		if (id < 16) {
			int z = to - NanohaTbl::Direction[id];
			defender[ndef++] = ban[z] & ~GOTE;
			if (dKiki[z] & (0x100u << id)) {
				z = SkipOverEMP(to, -NanohaTbl::Direction[id]);
				defender[ndef++] = ban[z] & ~GOTE;
			}
		}
		else {
			int z = SkipOverEMP(to, -NanohaTbl::Direction[id]);
			defender[ndef++] = ban[z] & ~GOTE;
			if (dKiki[z] & (0x1u << id)) {
				z = SkipOverEMP(to, -NanohaTbl::Direction[id]);
				defender[ndef++] = ban[z] & ~GOTE;
			}
		}
	}
	defender[ndef] = 0;
	assert(ndef < 32 - 1);

	if (ndef == 0) return (Value)value; // no defender -> stop SEE

	const effect_t *aKiki = (us == BLACK) ? effectB : effectW;
	int attacker[32];	// to に利いている攻めの駒
	int natk = 0;
	k = EXIST_EFFECT(aKiki[to]);
	while (k) {
		unsigned long id;
		_BitScanForward(&id, k);
		k &= k - 1;
		if (id < 16) {
			int z = to - NanohaTbl::Direction[id];
			if (from != z) attacker[natk++] = ban[z] & ~GOTE;
			if (dKiki[z] & (0x100u << id)) {
				z = SkipOverEMP(to, -NanohaTbl::Direction[id]);
				attacker[natk++] = ban[z] & ~GOTE;
			}
		}
		else {
			int z = SkipOverEMP(to, -NanohaTbl::Direction[id]);
			if (from != z) attacker[natk++] = ban[z] & ~GOTE;
			if (dKiki[z] & (0x1u << id)) {
				z = SkipOverEMP(to, -NanohaTbl::Direction[id]);
				attacker[natk++] = ban[z] & ~GOTE;
			}
		}
	}
	attacker[natk] = 0;
	assert(natk < 32 - 1);

	int i, j;
	int *p;
	int n;
	p = defender;
	n = ndef;
	for (i = 0; i < n - 1; i++) {
		for (j = i + 1; j < n; j++) {
			if (NanohaTbl::KomaValueEx[p[i]] > NanohaTbl::KomaValueEx[p[j]]) {
				int tmp = p[i];
				p[i] = p[j];
				p[j] = tmp;
			}
		}
	}
	p = attacker;
	n = natk;
	for (i = 0; i < n - 1; i++) {
		for (j = i + 1; j < n; j++) {
			if (NanohaTbl::KomaValueEx[p[i]] > NanohaTbl::KomaValueEx[p[j]]) {
				int tmp = p[i];
				p[i] = p[j];
				p[j] = tmp;
			}
		}
	}
	return (Value)(value - SEERec(tKind, defender, attacker)); // counter
#else
  Square from, to;
  Bitboard occupied, attackers, stmAttackers;
  Value swapList[32];
  int slIndex = 1;
  PieceType captured;
  Color stm;

  assert(is_ok(m));

  from = from_sq(m);
  to = to_sq(m);
  swapList[0] = PieceValue[MG][piece_on(to)];
  stm = color_of(piece_on(from));
  occupied = pieces() ^ from;

  // Castling moves are implemented as king capturing the rook so cannot
  // be handled correctly. Simply return VALUE_ZERO that is always correct
  // unless in the rare case the rook ends up under attack.
  if (type_of(m) == CASTLING)
      return VALUE_ZERO;

  if (type_of(m) == ENPASSANT)
  {
      occupied ^= to - pawn_push(stm); // Remove the captured pawn
      swapList[0] = PieceValue[MG][PAWN];
  }

  // Find all attackers to the destination square, with the moving piece
  // removed, but possibly an X-ray attacker added behind it.
  attackers = attackers_to(to, occupied) & occupied;

  // If the opponent has no attackers we are finished
  stm = ~stm;
  stmAttackers = attackers & pieces(stm);
  if (!stmAttackers)
      return swapList[0];

  // The destination square is defended, which makes things rather more
  // difficult to compute. We proceed by building up a "swap list" containing
  // the material gain or loss at each stop in a sequence of captures to the
  // destination square, where the sides alternately capture, and always
  // capture with the least valuable piece. After each capture, we look for
  // new X-ray attacks from behind the capturing piece.
  captured = type_of(piece_on(from));

  do {
      assert(slIndex < 32);

      // Add the new entry to the swap list
      swapList[slIndex] = -swapList[slIndex - 1] + PieceValue[MG][captured];

      // Locate and remove the next least valuable attacker
      captured = min_attacker<PAWN>(byTypeBB, to, stmAttackers, occupied, attackers);
      stm = ~stm;
      stmAttackers = attackers & pieces(stm);
      ++slIndex;

  } while (stmAttackers && (captured != KING || (--slIndex, false))); // Stop before a king capture

  // Having built the swap list, we negamax through it to find the best
  // achievable score from the point of view of the side to move.
  while (--slIndex)
      swapList[slIndex - 1] = std::min(-swapList[slIndex], swapList[slIndex - 1]);

  return swapList[0];
#endif
}


/// Position::is_draw() tests whether the position is drawn by 50-move rule
/// or by repetition. It does not detect stalemates.

#if defined(NANOHA)
bool Position::is_draw(int& ret) const {
	ret = 0;
	int i = 5, e = st->pliesFromNull;

	if (i <= e)
	{
		StateInfo* stp = st->previous->previous;
		int rept = 0;
		// 王手をかけている
		bool cont_checking = (st->effect && stp->effect) ? true : false;
		// 王手をかけられている
		bool cont_checked = (st->previous->effect && stp->previous->effect) ? true : false;

		do {
			stp = stp->previous->previous;
			if (stp->effect == 0) cont_checking = false;
			if (stp->previous->effect == 0) cont_checked = false;

			if (stp->key == st->key && stp->hand == st->hand) {
				rept++;
				// 過去に3回(現局面含めて4回)出現していたら千日手.
				if (rept >= 3) {
					if (cont_checking) { ret = 1; return false; }
					if (cont_checked) { ret = -1; return false; }
					return true;
				}
			}

			i += 2;

		} while (i < e);
	}
	return false;
}
#else
bool Position::is_draw() const {

  if (st->rule50 > 99 && (!checkers() || MoveList<LEGAL>(*this).size()))
      return true;

  StateInfo* stp = st;
  for (int i = 2, e = std::min(st->rule50, st->pliesFromNull); i <= e; i += 2)
  {
      stp = stp->previous->previous;

      if (stp->key == st->key)
          return true; // Draw at first repetition
  }

  return false;
}
#endif

#ifndef NANOHA
/// Position::flip() flips position with the white and black sides reversed. This
/// is only useful for debugging e.g. for finding evaluation symmetry bugs.

void Position::flip() {

  string f, token;
  std::stringstream ss(fen());

  for (Rank r = RANK_8; r >= RANK_1; --r) // Piece placement
  {
      std::getline(ss, token, r > RANK_1 ? '/' : ' ');
      f.insert(0, token + (f.empty() ? " " : "/"));
  }

  ss >> token; // Active color
  f += (token == "w" ? "B " : "W "); // Will be lowercased later

  ss >> token; // Castling availability
  f += token + " ";

  std::transform(f.begin(), f.end(), f.begin(),
                 [](char c) { return char(islower(c) ? toupper(c) : tolower(c)); });

  ss >> token; // En passant square
  f += (token == "-" ? token : token.replace(1, 1, token[1] == '3' ? "6" : "3"));

  std::getline(ss, token); // Half and full moves
  f += token;

  set(f, is_chess960(), this_thread());

  assert(pos_is_ok());
}
#endif

/// Position::pos_is_ok() performs some consistency checks for the position object.
/// This is meant to be helpful when debugging.
#ifdef NANOHA
bool Position::pos_is_ok(int* failedStep) const {
	// What features of the position should be verified?
	const bool debugAll = false;
	const bool debugKingCount = debugAll || false;
	const bool debugKingCapture = debugAll || false;
	const bool debugKey = debugAll || false;
	//  const bool debugIncrementalEval = debugAll || false;
	const bool debugPieceCounts = debugAll || false;

	if (failedStep) *failedStep = 1;

	// Side to move OK?
	if (side_to_move() != WHITE && side_to_move() != BLACK)
		return false;

	// Are the king squares in the position correct?
	if (failedStep) (*failedStep)++;
	if (sq_king<WHITE>() != 0 && piece_on((Square)sq_king<WHITE>()) != GOU) {
		std::cerr << "kposW=0x" << std::hex << int(sq_king<WHITE>()) << ", Piece=0x" << int(piece_on((Square)sq_king<WHITE>())) << std::endl;
		print_csa();
		return false;
	}

	if (failedStep) (*failedStep)++;
	if (sq_king<BLACK>() != 0 && piece_on((Square)sq_king<BLACK>()) != SOU) {
		std::cerr << "kposB=0x" << std::hex << int(sq_king<BLACK>()) << ", Piece=0x" << int(piece_on((Square)sq_king<BLACK>())) << std::endl;
		print_csa();
		return false;
	}

	// Do both sides have exactly one king?
	if (failedStep) (*failedStep)++;
	if (debugKingCount)
	{
		int kingCount[2] = { 0, 0 };
		for (Square s = SQ_A1; s <= SQ_I9; s++)
			if (type_of(piece_on(s)) == OU)
				kingCount[color_of(piece_on(s))]++;

		if (kingCount[0] != 1 || kingCount[1] != 1)
			return false;
	}

	// Can the side to move capture the opponent's king?
	if (failedStep) (*failedStep)++;
	if (debugKingCapture)
	{
		// TODO: 玉に相手駒の利きがあるか？
		//      Color us = side_to_move();
		//      Color them = flip(us);
		//      Square ksq = king_square(them);
		//      if (attackers_to(ksq) & pieces_of_color(us))
		//          return false;
	}

	// Is there more than 2 checkers?
	if (failedStep) (*failedStep)++;
	// TODO:玉に3駒以上の利きがあったら不正な状態
	//  if (debugCheckerCount && count_1s<CNT32>(st->checkersBB) > 2)
	//      return false;

	// Hash key OK?
	if (failedStep) (*failedStep)++;
	if (debugKey && st->key != compute_key())
		return false;

	// Incremental eval OK?
	if (failedStep) (*failedStep)++;
	// TODO:
	//  if (debugIncrementalEval && st->value != compute_value())
	//      return false;

	// TODO:行き所のない駒(1段目の歩、香、桂、2段目の桂)

	// Piece counts OK?
	if (failedStep) (*failedStep)++;
	if (debugPieceCounts) {
	}
	// TODO:盤面情報と駒番号の情報のチェック.
	if (failedStep) *failedStep = 0;
	return true;

}
#else
bool Position::pos_is_ok(int* failedStep) const {

  const bool Fast = true; // Quick (default) or full check?

  enum { Default, King, Bitboards, State, Lists, Castling };

  for (int step = Default; step <= (Fast ? Default : Castling); step++)
  {
      if (failedStep)
          *failedStep = step;

      if (step == Default)
          if (   (sideToMove != WHITE && sideToMove != BLACK)
              || piece_on(square<KING>(WHITE)) != W_KING
              || piece_on(square<KING>(BLACK)) != B_KING
              || (   ep_square() != SQ_NONE
                  && relative_rank(sideToMove, ep_square()) != RANK_6))
              return false;

      if (step == King)
          if (   std::count(board, board + SQUARE_NB, W_KING) != 1
              || std::count(board, board + SQUARE_NB, B_KING) != 1
              || attackers_to(square<KING>(~sideToMove)) & pieces(sideToMove))
              return false;

      if (step == Bitboards)
      {
          if (  (pieces(WHITE) & pieces(BLACK))
              ||(pieces(WHITE) | pieces(BLACK)) != pieces())
              return false;

          for (PieceType p1 = PAWN; p1 <= KING; ++p1)
              for (PieceType p2 = PAWN; p2 <= KING; ++p2)
                  if (p1 != p2 && (pieces(p1) & pieces(p2)))
                      return false;
      }

      if (step == State)
      {
          StateInfo si = *st;
          set_state(&si);
          if (std::memcmp(&si, st, sizeof(StateInfo)))
              return false;
      }

      if (step == Lists)
          for (Color c = WHITE; c <= BLACK; ++c)
              for (PieceType pt = PAWN; pt <= KING; ++pt)
              {
                  if (pieceCount[c][pt] != popcount<Full>(pieces(c, pt)))
                      return false;

                  for (int i = 0; i < pieceCount[c][pt];  ++i)
                      if (   board[pieceList[c][pt][i]] != make_piece(c, pt)
                          || index[pieceList[c][pt][i]] != i)
                          return false;
              }

      if (step == Castling)
          for (Color c = WHITE; c <= BLACK; ++c)
              for (CastlingSide s = KING_SIDE; s <= QUEEN_SIDE; s = CastlingSide(s + 1))
              {
                  if (!can_castle(c | s))
                      continue;

                  if (   piece_on(castlingRookSquare[c | s]) != make_piece(c, ROOK)
                      || castlingRightsMask[castlingRookSquare[c | s]] != (c | s)
                      ||(castlingRightsMask[square<KING>(c)] & (c | s)) != (c | s))
                      return false;
              }
  }

  return true;
}
#endif

#if defined(NANOHA)
void Position::print_csa(Move move) const {

	if (move != MOVE_NONE)
	{
		std::cout << "\nMove is: " << move_to_csa(move) << std::endl;
	}
	else {
		std::cout << "\nMove is: NONE" << std::endl;
	}
	// 盤面
	for (Rank rank = RANK_1; rank <= RANK_9; rank++)
	{
		std::cout << "P" << int(rank);
		for (File file = FILE_9; file >= FILE_1; file--)
		{
			Square sq = make_square(file, rank);
			Piece piece = piece_on(sq);
			std::cout << pieceCSA.from_piece(piece);
		}
		std::cout << std::endl;
	}
	// 持ち駒
	unsigned int n;
	std::cout << "P+";
	{
		const Hand &h = hand[BLACK];
		if ((n = h.getFU()) > 0) while (n--) { std::cout << "00FU"; }
		if ((n = h.getKY()) > 0) while (n--) { std::cout << "00KY"; }
		if ((n = h.getKE()) > 0) while (n--) { std::cout << "00KE"; }
		if ((n = h.getGI()) > 0) while (n--) { std::cout << "00GI"; }
		if ((n = h.getKI()) > 0) while (n--) { std::cout << "00KI"; }
		if ((n = h.getKA()) > 0) while (n--) { std::cout << "00KA"; }
		if ((n = h.getHI()) > 0) while (n--) { std::cout << "00HI"; }
	}
	std::cout << std::endl << "P-";
	{
		const Hand &h = hand[WHITE];
		if ((n = h.getFU()) > 0) while (n--) { std::cout << "00FU"; }
		if ((n = h.getKY()) > 0) while (n--) { std::cout << "00KY"; }
		if ((n = h.getKE()) > 0) while (n--) { std::cout << "00KE"; }
		if ((n = h.getGI()) > 0) while (n--) { std::cout << "00GI"; }
		if ((n = h.getKI()) > 0) while (n--) { std::cout << "00KI"; }
		if ((n = h.getKA()) > 0) while (n--) { std::cout << "00KA"; }
		if ((n = h.getHI()) > 0) while (n--) { std::cout << "00HI"; }
	}
	std::cout << std::endl << (sideToMove == BLACK ? '+' : '-') << std::endl;
	std::cout << "SFEN is: " << fen() << "\nKey is: 0x" << std::hex << st->key << std::dec << std::endl;
}
#endif
