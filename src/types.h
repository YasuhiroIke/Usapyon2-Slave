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

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

/// When compiling with provided Makefile (e.g. for Linux and OSX), configuration
/// is done automatically. To get started type 'make help'.
///
/// When Makefile is not used (e.g. with Microsoft Visual Studio) some switches
/// need to be set manually:
///
/// -DNDEBUG      | Disable debugging mode. Always use this for release.
///
/// -DNO_PREFETCH | Disable use of prefetch asm-instruction. You may need this to
///               | run on some very old machines.
///
/// -DUSE_POPCNT  | Add runtime support for use of popcnt asm-instruction. Works
///               | only in 64-bit mode and requires hardware with popcnt support.
///
/// -DUSE_PEXT    | Add runtime support for use of pext asm-instruction. Works
///               | only in 64-bit mode and requires hardware with pext support.

#include <cassert>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#ifdef NANOHA
#include <cstdio>
#endif


#if defined(_MSC_VER)
// Disable some silly and noisy warning from MSVC compiler
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'

#define snprintf	_snprintf

#endif

#include <stdint.h>

/// Predefined macros hell:
///
/// __GNUC__           Compiler is gcc, Clang or Intel on Linux
/// __INTEL_COMPILER   Compiler is Intel
/// _MSC_VER           Compiler is MSVC or Intel on Windows
/// _WIN32             Building on Windows (any)
/// _WIN64             Building on Windows 64 bit

#if defined(_WIN64) && defined(_MSC_VER) // No Makefile used
#  include <intrin.h> // MSVC popcnt and bsfq instrinsics
#  define IS_64BIT
#  define USE_BSFQ
#endif

#if defined(USE_POPCNT) && defined(__INTEL_COMPILER) && defined(_MSC_VER)
#  include <nmmintrin.h> // Intel header for _mm_popcnt_u64() intrinsic
#endif

#if !defined(NO_PREFETCH) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#  include <xmmintrin.h> // Intel and Microsoft header for _mm_prefetch()
#endif

#if defined(USE_PEXT)
#  include <immintrin.h> // Header for _pext_u64() intrinsic
#  define pext(b, m) _pext_u64(b, m)
#else
#  define pext(b, m) (0)
#endif

#ifdef USE_POPCNT
const bool HasPopCnt = true;
#else
const bool HasPopCnt = false;
#endif

#ifdef USE_PEXT
const bool HasPext = true;
#else
const bool HasPext = false;
#endif

#ifdef IS_64BIT
const bool Is64Bit = true;
#else
const bool Is64Bit = false;
#endif

typedef uint64_t Key;

#ifndef NANOHA
typedef uint64_t Bitboard;
#endif

#ifdef NANOHA
const int MAX_MOVES = 768;
#else
const int MAX_MOVES = 256;
#endif
const int MAX_PLY   = 128;

#ifndef PRI64
#define PRI64	"%lld"
#endif

#ifndef PRIx64
#define PRIx64	"%llx"
#endif

#define Min(x, y) (((x) < (y)) ? (x) : (y))
#define Max(x, y) (((x) < (y)) ? (y) : (x))

#ifndef NANOHA
/// A move needs 16 bits to be stored
///
/// bit  0- 5: destination square (from 0 to 63)
/// bit  6-11: origin square (from 0 to 63)
/// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
/// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
/// NOTE: EN-PASSANT bit is set only when a pawn can be captured
///
/// Special cases are MOVE_NONE and MOVE_NULL. We can sneak these in because in
/// any normal move destination square is always different from origin square
/// while MOVE_NONE and MOVE_NULL have the same origin and destination square.

enum Move {
  MOVE_NONE,
  MOVE_NULL = 65
};

enum MoveType {
  NORMAL,
  PROMOTION = 1 << 14,
  ENPASSANT = 2 << 14,
  CASTLING  = 3 << 14
};

enum Color {
  WHITE, BLACK, NO_COLOR, COLOR_NB = 2
};

enum CastlingSide {
  KING_SIDE, QUEEN_SIDE, CASTLING_SIDE_NB = 2
};

enum CastlingRight {
  NO_CASTLING,
  WHITE_OO,
  WHITE_OOO = WHITE_OO << 1,
  BLACK_OO  = WHITE_OO << 2,
  BLACK_OOO = WHITE_OO << 3,
  ANY_CASTLING = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO,
  CASTLING_RIGHT_NB = 16
};

template<Color C, CastlingSide S> struct MakeCastling {
  static const CastlingRight
  right = C == WHITE ? S == QUEEN_SIDE ? WHITE_OOO : WHITE_OO
                     : S == QUEEN_SIDE ? BLACK_OOO : BLACK_OO;
};

enum Phase {
  PHASE_ENDGAME,
  PHASE_MIDGAME = 128,
  MG = 0, EG = 1, PHASE_NB = 2
};

enum ScaleFactor {
  SCALE_FACTOR_DRAW    = 0,
  SCALE_FACTOR_ONEPAWN = 48,
  SCALE_FACTOR_NORMAL  = 64,
  SCALE_FACTOR_MAX     = 128,
  SCALE_FACTOR_NONE    = 255
};

enum Bound {
  BOUND_NONE,
  BOUND_UPPER,
  BOUND_LOWER,
  BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

enum Value : int {
  VALUE_ZERO      = 0,
  VALUE_DRAW      = 0,
  VALUE_KNOWN_WIN = 10000,
  VALUE_MATE      = 32000,
  VALUE_INFINITE  = 32001,
  VALUE_NONE      = 32002,

  VALUE_MATE_IN_MAX_PLY  =  VALUE_MATE - 2 * MAX_PLY,
  VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + 2 * MAX_PLY,

  PawnValueMg   = 198,   PawnValueEg   = 258,
  KnightValueMg = 817,   KnightValueEg = 846,
  BishopValueMg = 836,   BishopValueEg = 857,
  RookValueMg   = 1270,  RookValueEg   = 1281,
  QueenValueMg  = 2521,  QueenValueEg  = 2558,

  MidgameLimit  = 15581, EndgameLimit  = 3998
};

enum PieceType {
  NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
  ALL_PIECES = 0,
  PIECE_TYPE_NB = 8
};

enum Piece {
  NO_PIECE,
  W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
  B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
  PIECE_NB = 16
};

#else

enum Phase {
  PHASE_ENDGAME,
  PHASE_MIDGAME = 128,
  MG = 0, EG = 1, PHASE_NB = 2
};

enum ScaleFactor {
  SCALE_FACTOR_DRAW    = 0,
  SCALE_FACTOR_ONEPAWN = 48,
  SCALE_FACTOR_NORMAL  = 64,
  SCALE_FACTOR_MAX     = 128,
  SCALE_FACTOR_NONE    = 255
};

enum Bound {
  BOUND_NONE,
  BOUND_UPPER,
  BOUND_LOWER,
  BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

enum Value : int {
  VALUE_ZERO      = 0,
  VALUE_DRAW      = 0,
  VALUE_KNOWN_WIN = 10000,
  VALUE_MATE      = 32000,
  VALUE_INFINITE  = 32001,
  VALUE_NONE      = 32002,

#ifdef NANOHA
	VALUE_MATE_IN_MAX_PLY = 32000-256,
	VALUE_MATED_IN_MAX_PLY = -32000 + 256,
#else
  VALUE_MATE_IN_MAX_PLY  =  VALUE_MATE - 2 * MAX_PLY,
  VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + 2 * MAX_PLY,
#endif
};

enum Player {SENTE = 0, GOTE = 0x10};

// 以下の判定は x が空(EMP)や壁(WALL)でないことを要求する(空や壁の場合は正しく判定できない)
#define IsGote(x)			((x) & GOTE)
#define IsSente(x)			(!IsGote(x))
#define IsNotOwn(x,SorG)	(((x) & GOTE) != SorG)

enum PieceType {
	PIECE_TYPE_NONE = 0,
	FU = 0x01,
	KY = 0x02,
	KE = 0x03,
	GI = 0x04,
	KI = 0x05,
	KA = 0x06,
	HI = 0x07,
	OU = 0x08,

	PROMOTED = 0x08,
	NAMAMASK = 0x07,

	TO = FU | PROMOTED,
	NY = KY | PROMOTED,
	NK = KE | PROMOTED,
	NG = GI | PROMOTED,
	UM = KA | PROMOTED,
	RY = HI | PROMOTED,
};

enum Piece {
	EMP = 0x00,
	WALL = 0x80,

	SFU = FU | SENTE,
	SKY = KY | SENTE,
	SKE = KE | SENTE,
	SGI = GI | SENTE,
	SKI = KI | SENTE,
	SKA = KA | SENTE,
	SHI = HI | SENTE,
	SOU = OU | SENTE,
	STO = TO | SENTE,
	SNY = NY | SENTE,
	SNK = NK | SENTE,
	SNG = NG | SENTE,
	SUM = UM | SENTE,
	SRY = RY | SENTE,

	GFU = FU | GOTE,
	GKY = KY | GOTE,
	GKE = KE | GOTE,
	GGI = GI | GOTE,
	GKI = KI | GOTE,
	GKA = KA | GOTE,
	GHI = HI | GOTE,
	GOU = OU | GOTE,
	GTO = TO | GOTE,
	GNY = NY | GOTE,
	GNK = NK | GOTE,
	GNG = NG | GOTE,
	GUM = UM | GOTE,
	GRY = RY | GOTE,
	PIECE_NONE = 32,
	PIECE_NB = 32,
	PIECE_TYPE_NB = 32
};

enum Color {
	BLACK, WHITE, COLOR_NONE,COLOR_NB=2
};

typedef uint8_t PieceKind_t;       // 駒種類
typedef uint8_t PieceNumber_t;     // 駒番号
#define MAX_PIECENUMBER	40

// 利きの定義
typedef uint32_t effect_t;

#define EFFECT_SHORT_MASK		0x00000FFF
#define EFFECT_LONG_MASK		0x00FF0000
#define EFFECT_LONG_SHIFT		16
#define EXIST_LONG_EFFECT(x)	((x) & EFFECT_LONG_MASK)
#define EXIST_EFFECT(x)			(x)

#define EFFECT_UP		(1u <<  0)
#define EFFECT_DOWN		(1u <<  1)
#define EFFECT_RIGHT	(1u <<  2)
#define EFFECT_LEFT		(1u <<  3)
#define EFFECT_UR		(1u <<  4)
#define EFFECT_UL		(1u <<  5)
#define EFFECT_DR		(1u <<  6)
#define EFFECT_DL		(1u <<  7)
#define EFFECT_KEUR		(1u <<  8)
#define EFFECT_KEUL		(1u <<  9)
#define EFFECT_KEDR		(1u << 10)
#define EFFECT_KEDL		(1u << 11)

#define EFFECT_KING_S(pos)	relate_pos(pos,kingS)
#define EFFECT_KING_G(pos)	relate_pos(pos,kingG)

// 方向の定義
	//
	//  DIR_KEUL           DIR_KEUR
	//  DIR_UL    DIR_UP   DIR_UR
	//  DIR_LEFT    ●     DIR_RIGHT
	//  DIR_DL   DIR_DOWN  DIR_DR
	//  DIR_KEDL           DIR_KEDR
	//

#define DIR_UP		(-1)
#define DIR_DOWN	(+1)
#define DIR_RIGHT	(-16)
#define DIR_LEFT	(+16)
#define DIR_UR		(DIR_UP   + DIR_RIGHT)
#define DIR_UL		(DIR_UP   + DIR_LEFT)
#define DIR_DR		(DIR_DOWN + DIR_RIGHT)
#define DIR_DL		(DIR_DOWN + DIR_LEFT)
#define DIR_KEUR	(DIR_UP   + DIR_UP   + DIR_RIGHT)
#define DIR_KEUL	(DIR_UP   + DIR_UP   + DIR_LEFT)
#define DIR_KEDR	(DIR_DOWN + DIR_DOWN + DIR_RIGHT)
#define DIR_KEDL	(DIR_DOWN + DIR_DOWN + DIR_LEFT)

#define DIR00		DIR_UP
#define DIR01		DIR_DOWN
#define DIR02		DIR_RIGHT
#define DIR03		DIR_LEFT
#define DIR04		DIR_UR
#define DIR05		DIR_UL
#define DIR06		DIR_DR
#define DIR07		DIR_DL
#define DIR08		DIR_KEUR
#define DIR09		DIR_KEUL
#define DIR10		DIR_KEDR
#define DIR11		DIR_KEDL
#define KIKI00		EFFECT_UP
#define KIKI01		EFFECT_DOWN
#define KIKI02		EFFECT_RIGHT
#define KIKI03		EFFECT_LEFT
#define KIKI04		EFFECT_UR
#define KIKI05		EFFECT_UL
#define KIKI06		EFFECT_DR
#define KIKI07		EFFECT_DL
#define KIKI08		EFFECT_KEUR
#define KIKI09		EFFECT_KEUL
#define KIKI10		EFFECT_KEDR
#define KIKI11		EFFECT_KEDL

// 成れるかどうか？
template<Color color>
inline bool can_promotion(const int pos)
{
	return (color == BLACK) ? ((pos & 0x0F) <= 3) : ((pos & 0x0F) >= 7);
}

// 行きどころのない駒を打ってはいけない
//   歩と香：先手は1段目、後手は9段目がNG
template<Color color>
inline bool is_drop_pawn(const int pos)
{
	if (color == BLACK && (pos & 0x0F) < 2) return false;
	if (color == WHITE && (pos & 0x0F) > 8) return false;
	return true;
}

//   桂：先手は1、2段目、後手は8、9段目がNG
template<Color color>
inline bool is_drop_knight(const int pos)
{
	if (color == BLACK && (pos & 0x0F) < 3) return false;
	if (color == WHITE && (pos & 0x0F) > 7) return false;
	return true;
}

// 持駒
struct Hand {
	uint32_t h;
	static const uint32_t tbl[HI+1];
/*
	xxxxxxxx xxxxxxxx xxxxxxxx xxx11111  歩
	xxxxxxxx xxxxxxxx xxxxx111 xxxxxxxx  香
	xxxxxxxx xxxxxxxx x111xxxx xxxxxxxx  桂
	xxxxxxxx xxxxx111 xxxxxxxx xxxxxxxx  銀
	xxxxxxxx x111xxxx xxxxxxxx xxxxxxxx  金
	xxxxxx11 xxxxxxxx xxxxxxxx xxxxxxxx  角
	xx11xxxx xxxxxxxx xxxxxxxx xxxxxxxx  飛
	このように定義すると16進数で表示したときにどの駒を何枚持っているかわかりやすい。
	例）飛、金、桂を各1枚、歩を8枚持っているときは 0x10101008 となる
 */
#define HAND_FU_SHIFT	 0
#define HAND_KY_SHIFT	 8
#define HAND_KE_SHIFT	12
#define HAND_GI_SHIFT	16
#define HAND_KI_SHIFT	20
#define HAND_KA_SHIFT	24
#define HAND_HI_SHIFT	28

#define HAND_FU_MASK	(0x1Fu << HAND_FU_SHIFT)
#define HAND_KY_MASK	(0x07u << HAND_KY_SHIFT)
#define HAND_KE_MASK	(0x07u << HAND_KE_SHIFT)
#define HAND_GI_MASK	(0x07u << HAND_GI_SHIFT)
#define HAND_KI_MASK	(0x07u << HAND_KI_SHIFT)
#define HAND_KA_MASK	(0x03u << HAND_KA_SHIFT)
#define HAND_HI_MASK	(0x03u << HAND_HI_SHIFT)
#define HAND_FU_INC		(0x01u << HAND_FU_SHIFT)
#define HAND_KY_INC		(0x01u << HAND_KY_SHIFT)
#define HAND_KE_INC		(0x01u << HAND_KE_SHIFT)
#define HAND_GI_INC		(0x01u << HAND_GI_SHIFT)
#define HAND_KI_INC		(0x01u << HAND_KI_SHIFT)
#define HAND_KA_INC		(0x01u << HAND_KA_SHIFT)
#define HAND_HI_INC		(0x01u << HAND_HI_SHIFT)
#define HAND_DOM_MASK	(~(HAND_FU_MASK | HAND_KY_MASK | HAND_KE_MASK | HAND_GI_MASK | HAND_KI_MASK | HAND_KA_MASK | HAND_HI_MASK))
#define IS_DOM_HAND(h1, h2)	((((h1) - (h2)) & HAND_DOM_MASK) == 0)
	// 各駒の枚数を取得する
	inline uint32_t getFU() const {return ((h & HAND_FU_MASK) >> HAND_FU_SHIFT);}
	inline uint32_t getKY() const {return ((h & HAND_KY_MASK) >> HAND_KY_SHIFT);}
	inline uint32_t getKE() const {return ((h & HAND_KE_MASK) >> HAND_KE_SHIFT);}
	inline uint32_t getGI() const {return ((h & HAND_GI_MASK) >> HAND_GI_SHIFT);}
	inline uint32_t getKI() const {return ((h & HAND_KI_MASK) >> HAND_KI_SHIFT);}
	inline uint32_t getKA() const {return ((h & HAND_KA_MASK) >> HAND_KA_SHIFT);}
	inline uint32_t getHI() const {return ((h & HAND_HI_MASK) >> HAND_HI_SHIFT);}
#ifdef USAPYON2
	static const int MASKS[HI + 1];
	static const int SHIFTS[HI + 1];
	inline uint32_t getFromKind(int kind) const{ return ((h&MASKS[kind]) >> SHIFTS[kind]); }
#endif // USAPYON2

	// 駒を持っているかどうか(存在を確認するだけならシフト演算は不要)
	inline uint32_t existFU() const {return (h & HAND_FU_MASK);}
	inline uint32_t existKY() const {return (h & HAND_KY_MASK);}
	inline uint32_t existKE() const {return (h & HAND_KE_MASK);}
	inline uint32_t existGI() const {return (h & HAND_GI_MASK);}
	inline uint32_t existKI() const {return (h & HAND_KI_MASK);}
	inline uint32_t existKA() const {return (h & HAND_KA_MASK);}
	inline uint32_t existHI() const {return (h & HAND_HI_MASK);}
	inline int exist(int kind) const {kind &= ~GOTE; if (IS_DOM_HAND(h, tbl[kind])) return 1; return 0;}
	inline void set(const int hand[]) {
		h = uint32_t((hand[HI] << HAND_HI_SHIFT) + (hand[KA] << HAND_KA_SHIFT)
		  + (hand[KI] << HAND_KI_SHIFT) + (hand[GI] << HAND_GI_SHIFT)
		  + (hand[KE] << HAND_KE_SHIFT) + (hand[KY] << HAND_KY_SHIFT)
		  + (hand[FU] << HAND_FU_SHIFT));
	}
	inline void inc(const int kind) {h += tbl[kind];}
	inline void dec(const int kind) {h -= tbl[kind];}
	Hand& operator = (uint32_t a) {h = a; return *this;}
	bool Empty() const {return h == 0;}
};
#endif

enum Depth {

  ONE_PLY = 1,

  DEPTH_ZERO          =  0,
  DEPTH_QS_CHECKS     =  0,
  DEPTH_QS_NO_CHECKS  = -1,
  DEPTH_QS_RECAPTURES = -5,

  DEPTH_NONE = -6,
  DEPTH_MAX  = MAX_PLY
};

#if defined(NANOHA)
enum Square {
	SQ_A1 = 0x11, SQ_A2, SQ_A3, SQ_A4, SQ_A5, SQ_A6, SQ_A7, SQ_A8, SQ_A9,
	SQ_B1 = 0x21, SQ_B2, SQ_B3, SQ_B4, SQ_B5, SQ_B6, SQ_B7, SQ_B8, SQ_B9,
	SQ_C1 = 0x31, SQ_C2, SQ_C3, SQ_C4, SQ_C5, SQ_C6, SQ_C7, SQ_C8, SQ_C9,
	SQ_D1 = 0x41, SQ_D2, SQ_D3, SQ_D4, SQ_D5, SQ_D6, SQ_D7, SQ_D8, SQ_D9,
	SQ_E1 = 0x51, SQ_E2, SQ_E3, SQ_E4, SQ_E5, SQ_E6, SQ_E7, SQ_E8, SQ_E9,
	SQ_F1 = 0x61, SQ_F2, SQ_F3, SQ_F4, SQ_F5, SQ_F6, SQ_F7, SQ_F8, SQ_F9,
	SQ_G1 = 0x71, SQ_G2, SQ_G3, SQ_G4, SQ_G5, SQ_G6, SQ_G7, SQ_G8, SQ_G9,
	SQ_H1 = 0x81, SQ_H2, SQ_H3, SQ_H4, SQ_H5, SQ_H6, SQ_H7, SQ_H8, SQ_H9,
	SQ_I1 = 0x91, SQ_I2, SQ_I3, SQ_I4, SQ_I5, SQ_I6, SQ_I7, SQ_I8, SQ_I9,
	SQ_NONE, SQUARE_NB = 0xA0
};
#else
enum Square {
  SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
  SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
  SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
  SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
  SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
  SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
  SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
  SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
  SQ_NONE,

  SQUARE_NB = 64,

  DELTA_N =  8,
  DELTA_E =  1,
  DELTA_S = -8,
  DELTA_W = -1,

  DELTA_NN = DELTA_N + DELTA_N,
  DELTA_NE = DELTA_N + DELTA_E,
  DELTA_SE = DELTA_S + DELTA_E,
  DELTA_SS = DELTA_S + DELTA_S,
  DELTA_SW = DELTA_S + DELTA_W,
  DELTA_NW = DELTA_N + DELTA_W
};
#endif

enum File {
#ifdef NANOHA
  FILE_0, FILE_1, FILE_2, FILE_3, FILE_4, FILE_5, FILE_6, FILE_7, FILE_8, FILE_9
#else
  FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB
#endif
};

enum Rank {
#ifdef NANOHA
  RANK_0, RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9
#else
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB
#endif
};


/// Score enum stores a middlegame and an endgame value in a single integer
/// (enum). The least significant 16 bits are used to store the endgame value
/// and the upper 16 bits are used to store the middlegame value.
enum Score : int { SCORE_ZERO };

inline Score make_score(int mg, int eg) {
  return Score((mg << 16) + eg);
}

/// Extracting the signed lower and upper 16 bits is not so trivial because
/// according to the standard a simple cast to short is implementation defined
/// and so is a right shift of a signed integer.
inline Value mg_value(Score s) {

  union { uint16_t u; int16_t s; } mg = { uint16_t(unsigned(s + 0x8000) >> 16) };
  return Value(mg.s);
}

inline Value eg_value(Score s) {

  union { uint16_t u; int16_t s; } eg = { uint16_t(unsigned(s)) };
  return Value(eg.s);
}

#define ENABLE_BASE_OPERATORS_ON(T)                             \
inline T operator+(T d1, T d2) { return T(int(d1) + int(d2)); } \
inline T operator-(T d1, T d2) { return T(int(d1) - int(d2)); } \
inline T operator*(int i, T d) { return T(i * int(d)); }        \
inline T operator*(T d, int i) { return T(int(d) * i); }        \
inline T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, T d2) { return d1 = d1 + d2; }      \
inline T& operator-=(T& d1, T d2) { return d1 = d1 - d2; }      \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }

#define ENABLE_FULL_OPERATORS_ON(T)                             \
ENABLE_BASE_OPERATORS_ON(T)                                     \
inline T& operator++(T& d) { return d = T(int(d) + 1); }        \
inline T& operator--(T& d) { return d = T(int(d) - 1); }        \
inline T operator/(T d, int i) { return T(int(d) / i); }        \
inline int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Value)
ENABLE_FULL_OPERATORS_ON(PieceType)
ENABLE_FULL_OPERATORS_ON(Piece)
ENABLE_FULL_OPERATORS_ON(Color)
ENABLE_FULL_OPERATORS_ON(Depth)
ENABLE_FULL_OPERATORS_ON(Square)
ENABLE_FULL_OPERATORS_ON(File)
ENABLE_FULL_OPERATORS_ON(Rank)

ENABLE_BASE_OPERATORS_ON(Score)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

/// Additional operators to add integers to a Value
inline Value operator+(Value v, int i) { return Value(int(v) + i); }
inline Value operator-(Value v, int i) { return Value(int(v) - i); }
inline Value& operator+=(Value& v, int i) { return v = v + i; }
inline Value& operator-=(Value& v, int i) { return v = v - i; }

/// Only declared but not defined. We don't want to multiply two scores due to
/// a very high risk of overflow. So user should explicitly convert to integer.
inline Score operator*(Score s1, Score s2);

/// Division of a Score must be handled separately for each term
inline Score operator/(Score s, int i) {
  return make_score(mg_value(s) / i, eg_value(s) / i);
}

#include "move.h"

#if defined(NANOHA)
const Value PawnValueMidgame   = Value(87+87);

extern const Value PieceValueMidgame[32];
extern const Value PieceValueEndgame[32];

#else
extern Value PieceValue[PHASE_NB][PIECE_NONE];
#endif

#ifndef NANOHA
inline Color operator~(Color c) {
  return Color(c ^ BLACK);
}

inline Square operator~(Square s) {
  return Square(s ^ SQ_A8); // Vertical flip SQ_A1 -> SQ_A8
}
#endif

#ifndef NANOHA
inline CastlingRight operator|(Color c, CastlingSide s) {
  return CastlingRight(WHITE_OO << ((s == QUEEN_SIDE) + 2 * c));
}
#endif

inline Value mate_in(int ply) {
  return VALUE_MATE - ply;
}

inline Value mated_in(int ply) {
  return -VALUE_MATE + ply;
}

#ifdef NANOHA
inline Square make_square(File f, Rank r) {
  return Square((r << 4) | f);
}
#else
inline Square make_square(File f, Rank r) {
  return Square((r << 3) | f);
}
#endif

inline Piece make_piece(Color c, PieceType pt) {
#if defined(NANOHA)
	return (c == BLACK) ? Piece(int(pt)) : Piece(GOTE | int(pt));
#else
  return Piece((c << 3) | pt);
#endif
}

inline PieceType type_of(Piece pc)  {
#if defined(NANOHA)
	return PieceType(int(pc) & 0x0F);
#else
	return PieceType(pc & 7);
#endif
}

inline Color color_of(Piece pc) {
#if defined(NANOHA)
	assert(pc != EMP && pc!=WALL);
	return (int(pc) & GOTE) ? WHITE : BLACK;
#else
	assert(pc != NO_PIECE);
	return Color(pc >> 3);
#endif
}

inline bool is_ok(Square s) {
#if defined(NANOHA)
	return s >= SQ_A1 && s <= SQ_I9;
#else
	return s >= SQ_A1 && s <= SQ_H8;
#endif
}

inline File file_of(Square s) {
#if defined(NANOHA)
	return File(int(s) >> 4);
#else
	return File(s & 7);
#endif
}

inline Rank rank_of(Square s) {
#if defined(NANOHA)
	return Rank(int(s) & 0x0F);
#else
	return Rank(s >> 3);
#endif
}

#ifndef NANOHA
inline Square relative_square(Color c, Square s) {
  return Square(s ^ (c * 56));
}

inline Rank relative_rank(Color c, Rank r) {
  return Rank(r ^ (c * 7));
}

inline Rank relative_rank(Color c, Square s) {
  return relative_rank(c, rank_of(s));
}
#endif

#ifndef NANOHA
inline bool opposite_colors(Square s1, Square s2) {
  int s = int(s1) ^ int(s2);
  return ((s >> 3) ^ s) & 1;
}

inline Square pawn_push(Color c) {
  return c == WHITE ? DELTA_N : DELTA_S;
}
#endif

inline Square from_sq(Move m) {
#ifdef NANOHA
	return Square(m>>8 & 0xff);
#else
	return Square((m >> 6) & 0x3F);
#endif
}

inline Square to_sq(Move m) {
#ifdef NANOHA
	return Square(m & 0xff);
#else
  return Square(m & 0x3F);
#endif
}

#ifndef NANOHA
inline MoveType type_of(Move m) {
	return MoveType(m & (3 << 14));
}

inline PieceType promotion_type(Move m) {
  return PieceType(((m >> 12) & 3) + KNIGHT);
}

inline Move make_move(Square from, Square to) {
  return Move(to | (from << 6));
}

template<MoveType T>
inline Move make(Square from, Square to, PieceType pt = KNIGHT) {
  return Move(to | (from << 6) | T | ((pt - KNIGHT) << 12));
}
#else
#if 0
inline MoveType type_of(Move m) {
	return MoveType((m >>27) & 0x1f);
}
#endif
#endif

#if defined(NANOHA)

#define MAX_CHECKS		128		// 王手の最大数
#define MAX_EVASION		128		// 王手回避の最大数

namespace NanohaTbl {
	extern const short z2sq[];
	extern const int Direction[];
	extern const int KomaValue[32];		// 駒の価値
	extern const int KomaValueEx[32];	// 取られたとき(捕獲されたとき)の価値
	extern const int KomaValuePro[32];	// 成る価値
	extern const int Piece2Index[32];	// 駒の種類に変換する({と、杏、圭、全}を金と同一視)
}

// なのはの座標系(0x11～0x99)を(0～80)に圧縮する
inline int conv_z2sq(int z){
	return NanohaTbl::z2sq[z];
}
inline int conv_sq2z(int sq)
{
	int x, y;
	y = sq / 9;
	x = sq % 9;
	return (x+1)*0x10+(y+1);
}

#if defined(_MSC_VER)

#elif defined(__GNUC__)
inline unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask)
{
	if (Mask == 0) return 0;
	*Index = static_cast<unsigned long>(__builtin_ctz(Mask));
	return 1;
}

#define __assume(x)		// MS のコンパイラのコード生成のヒントだが、gccでは無効なため

#endif	// defined(__GNUC__)

inline unsigned int PopCnt32(unsigned int value)
{
	unsigned int bits = 0;
	while (value) {value &= value-1; bits++;}
	return bits;
}

// 出力系
int output_info(const char *fmt, ...);		// 情報出力(printf()代わり)
int foutput_log(FILE *fp, const char *fmt, ...);		// 情報出力(printf()代わり)
//int output_debug(const char *fmt, ...);		// for デバッグ

#define ENABLE_MYASSERT			// MYASSERT()を有効にする
#if defined(ENABLE_MYASSERT)
extern int debug_level;
// x が真のときに何もしない
// *static_cast<int*>(0)=0でメモリ保護例外が発生し、ただちに止まるためデバッグしやすい.
#define MYASSERT(x)	\
			do {		\
				if (!(x)) {	\
					fprintf(stderr, "Assert:%s:%d:[%s]\n", __FILE__, __LINE__, #x);	\
					*static_cast<int*>(0)=0;	\
				}	\
			} while(0)
#define MYABORT()	\
			do {		\
				fprintf(stderr, "Abort:%s:%d\n", __FILE__, __LINE__);	\
				*static_cast<int*>(0)=0;	\
			} while(0)
#define SET_DEBUG(x)	debug_level=(x)
#define DEBUG_LEVEL		debug_level
#else
#define MYASSERT(x)	
#define MYABORT()	
#define SET_DEBUG(x)
#define DEBUG_LEVEL		0
#endif

#if defined(CHK_PERFORM)
#define COUNT_PERFORM(x)	((x)++)
#else
#define COUNT_PERFORM(x)
#endif // defined(CHK_PERFORM)

#endif

#endif // #ifndef TYPES_H_INCLUDED
