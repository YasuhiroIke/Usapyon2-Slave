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



#if !defined(MOVE_H_INCLUDED)
#define MOVE_H_INCLUDED

#include <string>


#if defined(NANOHA)
/*
xxxxxxxx xxxxxxxx xxxxxxxx 11111111  8bit(bit 0- 7) 移動先
xxxxxxxx xxxxxxxx 11111111 xxxxxxxx  8bit(bit 8-15) 移動元 or 駒の種類
xxxxxxxx xxxxxxx1 xxxxxxxx xxxxxxxx  1bit(bit16)    成フラグ
xxxxxxxx xx11111x xxxxxxxx xxxxxxxx  5bit(bit17-21) 動かす駒
xxxxx111 11xxxxxx xxxxxxxx xxxxxxxx  5bit(bit22-26) 捕獲する駒
11111xxx xxxxxxxx xxxxxxxx xxxxxxxx  5bit(bit27-31) 手の種類
*/
enum Move {
	MOVE_NONE = 0,
	MOVE_NULL = 0xFFF00001		// from != toとなるように定義
};
#endif

#include "misc.h"
#include "types.h"

// types.hに既存
// Maximum number of allowed moves per position
/*
#if defined(NANOHA)
const int MAX_MOVES = 768;	// 合法手の最大値593以上で切りのよさそうな数
#else
const int MAX_MOVES = 256;
#endif
*/

#if defined(NANOHA)
#define To2Move(to)             (static_cast<unsigned int>(to)   <<  0)
#define TO_MASK                 0xFFU
#define From2Move(from)         (static_cast<unsigned int>(from) <<  8)
#define FLAG_PROMO               (1U                  << 16)

#define Piece2Move(piece)       (static_cast<unsigned int>(piece) << 17)
#define Cap2Move(piece)         (static_cast<unsigned int>(piece) << 22)

#define U2To(move)              (((move) >>  0) & 0x00FFU)
#define U2From(move)            (((move) >>  8) & 0x00FFU)
#define U2PieceMove(move)       (((move) >> 17) & 0x001FU)

#define UToCap(u)               (((u)     >> 22) & 0x001FU)

#define MOVE_CHECK_LONG		(0x01U << 27)	// 跳び駒による王手
#define MOVE_CHECK_NARAZU	(0x02U << 27)	// 飛、角の不成

#else
/// A move needs 16 bits to be stored
///
/// bit  0- 5: destination square (from 0 to 63)
/// bit  6-11: origin square (from 0 to 63)
/// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
/// bit 14-15: special move flag: promotion (1), en passant (2), castle (3)
///
/// Special cases are MOVE_NONE and MOVE_NULL. We can sneak these in
/// because in any normal move destination square is always different
/// from origin square while MOVE_NONE and MOVE_NULL have the same
/// origin and destination square, 0 and 1 respectively.

enum Move {
	MOVE_NONE = 0,
	MOVE_NULL = 65
};
#endif


struct MoveStack {
	Move move;
	int score;
	inline operator Move() {
		return move;
	}
	inline void operator=(Move m) { move = m; }
};


inline bool operator<(const MoveStack& f, const MoveStack& s) { return f.score < s.score; }

// An helper insertion sort implementation, works with pointers and iterators
template<typename T, typename K>
inline void sort(K firstMove, K lastMove)
{
	T value;
	K cur, p, d;

	if (firstMove != lastMove)
		for (cur = firstMove + 1; cur != lastMove; cur++)
		{
			p = d = cur;
			value = *p--;
			if (*p < value)
			{
				do *d = *p;
				while (--d != firstMove && *--p < value);
				*d = value;
			}
		}
}

inline Square move_from(Move m) {
#if defined(NANOHA)
	return Square(U2From(static_cast<unsigned int>(m)));
#else
	return Square((m >> 6) & 0x3F);
#endif
}

inline Square move_to(Move m) {
#if defined(NANOHA)
	return Square(U2To(static_cast<unsigned int>(m)));
#else
	return Square(m & 0x3F);
#endif
}

#if defined(NANOHA)
inline Piece move_piece(Move m) {
	return Piece(U2PieceMove(static_cast<unsigned int>(m)));	// 手番を含めた、駒の種類(先手歩～後手龍)
}
inline PieceType move_ptype(Move m) {
	return PieceType((int(m) >> 17) & 0x0F);	// 駒の種類(歩～龍)
}
inline bool move_is_drop(Move m) {
	return (int(m) & 0xFF00) < 0x1100;
}

inline bool is_special(Move) {
	return false;
}

inline Piece move_captured(Move m) {
	return Piece(UToCap(static_cast<unsigned int>(m)));
}

void move_fprint(FILE *fp, Move m, int rotate = 0);
inline void move_print(Move m, int rotate = 0) {
	move_fprint(stdout, m, rotate);
}

class Position;
extern Move cons_move(Color us, unsigned char f, unsigned char t, const Position &k);

inline Move cons_move(const int from, const int to, const Piece piece, const Piece capture, const int promote=0, const unsigned int K=0)
{
	unsigned int tmp;
	tmp  = From2Move(from);
	tmp |= To2Move(to);
	tmp |= Piece2Move(piece);
	tmp |= Cap2Move(capture);
	if (promote) tmp |= FLAG_PROMO;
	tmp |= K;
	return Move(tmp);
}

#else
inline bool is_special(Move m) {
	return m & (3 << 14);
}
#endif

inline bool is_promotion(Move m) {
#if defined(NANOHA)
	return (static_cast<unsigned int>(m) & FLAG_PROMO)!=0;
#else
	return (m & (3 << 14)) == (1 << 14);
#endif
}

#if defined(NANOHA)
inline int is_enpassant(Move) {
	return false;
}
#else
inline int is_enpassant(Move m) {
	return (m & (3 << 14)) == (2 << 14);
}
#endif

#if defined(NANOHA)
inline int is_castle(Move) {
	return false;
}
#else
inline int is_castle(Move m) {
	return (m & (3 << 14)) == (3 << 14);
}
#endif

#if defined(NANOHA)
inline bool move_is_pawn_drop(Move m){
	return move_is_drop(m) && move_ptype(m) == FU;
}
#endif

#if !defined(NANOHA)
inline PieceType promotion_piece_type(Move m) {
	return PieceType(((m >> 12) & 3) + 2);
}

inline Move make_move(Square from, Square to) {
	return Move(to | (from << 6));
}

inline Move make_promotion_move(Square from, Square to, PieceType promotion) {
	return Move(to | (from << 6) | (1 << 14) | ((promotion - 2) << 12)) ;
}

inline Move make_enpassant_move(Square from, Square to) {
	return Move(to | (from << 6) | (2 << 14));
}

inline Move make_castle_move(Square from, Square to) {
	return Move(to | (from << 6) | (3 << 14));
}
#endif

inline bool is_ok(Move m) {
	return move_from(m) != move_to(m); // Catches also MOVE_NONE
}

class Position;

#if defined(NANOHA)
extern const std::string move_to_uci(Move m);
extern const std::string move_to_csa(Move m);
extern const std::string move_to_kif(Move m);
#else
extern const std::string move_to_uci(Move m, bool chess960);
#endif
extern Move move_from_uci(const Position& pos, const std::string& str);
extern const std::string move_to_san(Position& pos, Move m);

#endif // !defined(MOVE_H_INCLUDED)
