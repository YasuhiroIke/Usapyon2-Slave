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

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <cstddef>  // For offsetof()
#include <string>

#ifndef NANOHA 
#include "bitboard.h"
#endif
#include "types.h"

class Position;
class Thread;

#ifndef NANOHA
namespace PSQT {

  extern Score psq[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];

  void init();
}
#endif

#ifndef NANOHA
/// CheckInfo struct is initialized at constructor time and keeps info used to
/// detect if a move gives check.

struct CheckInfo {

  explicit CheckInfo(const Position&);

  Bitboard dcCandidates;
  Bitboard pinned;
  Bitboard checkSquares[PIECE_TYPE_NB];
  Square   ksq;
};
#endif

/// StateInfo struct stores information needed to restore a Position object to
/// its previous state when we retract a move. Whenever a move is made on the
/// board (by calling Position::do_move), a StateInfo object must be passed.

struct StateInfo {
#if defined(NANOHA)
	int gamePly;
	int pliesFromNull;

	Piece captured;
	uint32_t hand;
	uint32_t effect;
	Key key;
#else
  // Copied when making a move
  Key    pawnKey;
  Key    materialKey;
  Value  nonPawnMaterial[COLOR_NB];
  int    castlingRights;
  int    rule50;
  int    pliesFromNull;
  Score  psq;
  Square epSquare;

  // Not copied when making a move
  Key        key;
  Bitboard   checkersBB;
  PieceType  capturedType;
#endif
  StateInfo* previous;
};

#if defined(NANOHA)
// 初期化関数
extern void init_application_once();	// 実行ファイル起動時に行う初期化.
#endif

/// Position class stores information regarding the board representation as
/// pieces, side to move, hash keys, castling info, etc. Important methods are
/// do_move() and undo_move(), used by the search to update node info when
/// traversing the search tree.

class Position {

public:
  static void init();
#ifdef C11_impemented
  Position() = default; // To define the global object RootPos
  Position(const Position&) = delete;
#else
  Position() {}
private:
	Position(const Position& old) {
		memcpy(this,&old,sizeof(Position));
	};
public:
#endif
  Position(const Position& pos, Thread* th) { *this = pos; thisThread = th; }
#ifdef NANOHA
  Position(const std::string& fen, int threadID);
  Position(const std::string& f, Thread* th) { set(f, th); }
#else
  Position(const std::string& fen, bool isChess960, int threadID);
  Position(const std::string& f, bool c960, Thread* th) { set(f, c960, th); }
#endif
  Position& operator=(const Position&); // To assign RootPos from UCI

  // FEN string input/output
#ifdef NANOHA
  void set(const std::string& fenStr, Thread* th);
#else
  void set(const std::string& fenStr, bool isChess960, Thread* th);
#endif
  const std::string fen() const;
#ifdef NANOHA
	void print_csa(Move m = MOVE_NONE) const;
	void print(Move m) const;
	void print() const {print(MOVE_NULL);}
	bool operator == (const Position &a) const;
	bool operator != (const Position &a) const;
#else
	void print(Move m = MOVE_NONE) const;
#endif


#ifndef NANOHA
  // Position representation
  Bitboard pieces() const;
  Bitboard pieces(PieceType pt) const;
  Bitboard pieces(PieceType pt1, PieceType pt2) const;
  Bitboard pieces(Color c) const;
  Bitboard pieces(Color c, PieceType pt) const;
  Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
#endif
  Piece piece_on(Square s) const;
#ifndef NANOHA
  Square ep_square() const;
#endif
  bool empty(Square s) const;
#if defined(NANOHA)
  Value type_value_of_piece_on(Square s) const;
  Value promote_value_of_piece_on(Square s) const;
  int pin_on(Square s) const;
#else
  template<PieceType Pt> int count(Color c) const;
  template<PieceType Pt> const Square* squares(Color c) const;
  template<PieceType Pt> Square square(Color c) const;
#endif

#ifndef NANOHA
  // Castling
  int can_castle(Color c) const;
  int can_castle(CastlingRight cr) const;
  bool castling_impeded(CastlingRight cr) const;
  Square castling_rook_square(CastlingRight cr) const;
#endif

#ifndef NANOHA
  // Checking
  Bitboard checkers() const;
  Bitboard discovered_check_candidates() const;
  Bitboard pinned_pieces(Color c) const;

  // Attacks to/from a given square
  Bitboard attackers_to(Square s) const;
  Bitboard attackers_to(Square s, Bitboard occupied) const;
  Bitboard attacks_from(Piece pc, Square s) const;
  template<PieceType> Bitboard attacks_from(Square s) const;
  template<PieceType> Bitboard attacks_from(Square s, Color c) const;
#else
  	bool in_check() const;
	bool at_checking() const;	// 王手をかけている状態か？
#endif

  // Properties of moves
#ifdef NANOHA
	bool is_double_pawn(const Color us, int to) const;		// 二歩か？
	bool is_pawn_drop_mate(const Color us, int to) const;	// 打ち歩詰めか？
	bool move_gives_check(Move m) const;
	bool move_attacks_square(Move m, Square s) const;
	bool pl_move_is_legal(const Move m) const;
	bool is_pseudo_legal(const Move m) const {return pl_move_is_legal(m);}
  bool capture(Move m) const;
  bool capture_or_promotion(Move m) const;
  PieceType captured_piece_type() const;
  Piece moved_piece(Move m) const;
  bool legal(Move m) const { return pl_move_is_legal(m); }
#else
  bool legal(Move m, Bitboard pinned) const;
  bool pseudo_legal(const Move m) const;
  bool capture(Move m) const;
  bool capture_or_promotion(Move m) const;
  bool gives_check(Move m, const CheckInfo& ci) const;
  bool advanced_pawn_push(Move m) const;
  PieceType captured_piece_type() const;
#endif


#ifndef NANOHA
  // Piece specific
  bool pawn_passed(Color c, Square s) const;
  bool opposite_bishops() const;
#endif

#ifdef NANOHA
  void do_move(Move m, StateInfo& st,int count=1);
  void do_drop(Move m);
  void undo_move(Move m);
  void undo_drop(Move m);
  void do_null_move(StateInfo& st);
  void undo_null_move();
  // 手を進めずにハッシュ計算のみ行う
#ifndef USAPYON2
  uint64_t calc_hash_no_move(const Move m) const;
#endif
#else
  // Doing and undoing moves
  void do_move(Move m, StateInfo& st, bool givesCheck);
  void undo_move(Move m);
  void do_null_move(StateInfo& st);
  void undo_null_move();
#endif


#if defined(NANOHA)
	// 手生成で使うための関数
	// 指定位置から指定方向に何かある位置まで探す(WALL or 先手駒 or 後手駒の位置になる)
	int SkipOverEMP(int pos, const int dir) const;
	// 利きの更新
	void add_effect(const int z);					// 位置zの駒の利きを反映する
	void del_effect(const int z, const Piece k);	
	// 利きの演算
	template<Color>
	void add_effect_straight(const int z, const int dir, const uint32_t bit);
	template<Color>
	void del_effect_straight(const int z, const int dir, const uint32_t bit);
#define AddKikiDirS	add_effect_straight<BLACK>
#define AddKikiDirG	add_effect_straight<WHITE>
#define DelKikiDirS	del_effect_straight<BLACK>
#define DelKikiDirG	del_effect_straight<WHITE>
	// ピン情報更新
	template <Color> void add_pin_info(const int dir);
	template <Color> void del_pin_info(const int dir);
#define AddPinInfS	add_pin_info<BLACK>
#define AddPinInfG	add_pin_info<WHITE>
#define DelPinInfS	del_pin_info<BLACK>
#define DelPinInfG	del_pin_info<WHITE>

public:
	// 手生成
	template<Color> MoveStack* add_straight(MoveStack* mlist, const int from, const int dir) const;
	template<Color> MoveStack* add_move(MoveStack* mlist, const int from, const int dir) const;
#define add_straightB	add_straight<BLACK>
#define add_straightW	add_straight<WHITE>
#define add_moveB	add_move<BLACK>
#define add_moveW	add_move<WHITE>
	MoveStack* gen_move_to(const Color us, MoveStack* mlist, int to) const;		// toに動く手の生成
	MoveStack* gen_drop_to(const Color us, MoveStack* mlist, int to) const;		// toに駒を打つ手の生成
	template <Color> MoveStack* gen_drop(MoveStack* mlist) const;			// 駒を打つ手の生成
	MoveStack* gen_move_king(const Color us, MoveStack* mlist, int pindir = 0) const;			//玉の動く手の生成
	MoveStack* gen_king_noncapture(const Color us, MoveStack* mlist, int pindir = 0) const;			//玉の動く手の生成
	MoveStack* gen_move_from(const Color us, MoveStack* mlist, int from, int pindir = 0) const;		//fromから動く手の生成

	template <Color> MoveStack* generate_capture(MoveStack* mlist) const;
	template <Color> MoveStack* generate_non_capture(MoveStack* mlist) const;
	template <Color> MoveStack* generate_evasion(MoveStack* mlist) const;
	template <Color> MoveStack* generate_non_evasion(MoveStack* mlist) const;
	template <Color> MoveStack* generate_legal(MoveStack* mlist) const;

	// 王手関連
	template <Color> MoveStack* gen_check_long(MoveStack* mlist) const;
	template <Color> MoveStack* gen_check_short(MoveStack* mlist) const;
	template <Color> MoveStack* gen_check_drop(MoveStack* mlist, bool &bUchifudume) const;
	template <Color> MoveStack* generate_check(MoveStack* mlist, bool &bUchifudume) const;

	// 3手詰め用の手生成
	template <Color> MoveStack* gen_check_drop3(MoveStack* mlist, bool &bUchifudume) const;
	template <Color> MoveStack* generate_check3(MoveStack* mlist, bool &bUchifudume) const;			// 王手生成
	// 王手回避手の生成(3手詰め残り2手用)
	MoveStack *generate_evasion_rest2(const Color us, MoveStack *mBuf, effect_t effect, int &Ai);
	MoveStack *generate_evasion_rest2_MoveAi(const Color us, MoveStack *mBuf, effect_t effect);
	MoveStack *generate_evasion_rest2_DropAi(const Color us, MoveStack *mBuf, effect_t effect, int &check_pos);

	// 局面の評価
	static void init_evaluate();
#if defined(EVAL_APERY)
	int make_list_apery(int list0[], int list1[], int nlist) const;
#else
	int make_list(int * pscore, int list0[], int list1[] ) const;
#endif
	int evaluate(const Color us) const;

	// 稲庭判定(bInaniwa にセットするため const でない)
	bool IsInaniwa(const Color us);

	// 機能：勝ち宣言できるかどうか判定する
	bool IsKachi(const Color us) const;

	// 王手かどうか？
	bool is_check_move(const Color us, Move move) const;

	// 高速1手詰め
	static void initMate1ply();
	template <Color us> uint32_t infoRound8King() const;
	// 駒打ちにより利きを遮るか？
	template <Color us> uint32_t chkInterceptDrop(const uint32_t info, const int kpos, const int i, const int kind) const;
	// 駒移動により利きを遮るか？
	template <Color us> uint32_t chkInterceptMove(const uint32_t info, const int kpos, const int i, const int from, const int kind) const;
	// 駒打による一手詰の判定
	template <Color us> uint32_t CheckMate1plyDrop(const uint32_t info, Move &m) const;
	int CanAttack(const int kind, const int from1, const int from2, const int to) const;
	// 移動による一手詰の判定
	template <Color us> uint32_t CheckMate1plyMove(const uint32_t info, Move &m, const int check = 0) const;
	template <Color us> int Mate1ply(Move &m, uint32_t &info);

	// 3手詰め
	int Mate3(const Color us, Move &m);
//	int EvasionRest2(const Color us, MoveStack *antichecks, unsigned int &PP, unsigned int &DP, int &dn);
	int EvasionRest2(const Color us, MoveStack *antichecks);

	template<Color>
	effect_t exist_effect(int pos) const;				// 利き
	template<Color us>
	int sq_king() const {return (us == BLACK) ? knpos[1] : knpos[2];}

	// 局面をHuffman符号化する
	int EncodeHuffman(unsigned char buf[32]) const;
#endif

  // Static exchange evaluation
  Value see(Move m) const;
  Value see_sign(Move m) const;

  // Accessing hash keys
  Key key() const;
  Key key_after(Move m) const;
  Key exclusion_key() const;
#ifndef NANOHA
  Key material_key() const;
  Key pawn_key() const;
#endif

  // Other properties of the position
  Color side_to_move() const;
#ifndef NANOHA
  Phase game_phase() const;
#endif
  int game_ply() const;
#ifndef NANOHA
  bool is_chess960() const;
#endif
  Thread* this_thread() const;
#if defined(NANOHA)
	int64_t tnodes_searched() const;
	void set_tnodes_searched(int64_t n);
#if defined(CHK_PERFORM)
	unsigned long mate3_searched() const;
	void set_mate3_searched(unsigned long  n);
	void inc_mate3_searched(unsigned long  n=1);
#endif // defined(CHK_PERFORM)
#endif
  uint64_t nodes_searched() const;
  void set_nodes_searched(uint64_t n);
#ifdef NANOHA
  bool is_draw(int &ret) const;
#else
  bool is_draw() const;
#endif

#ifndef NANOHA
  int rule50_count() const;
#endif

#ifndef NANOHA
  Score psq_score() const;
  Value non_pawn_material(Color c) const;
#endif

  // Position consistency check, for debugging
  bool pos_is_ok(int* failedStep = nullptr) const;
  void flip();
#ifdef NANOHA
  inline Color flip(Color side) const {
	  return (Color)(side ^ 1);
  }
#endif
#if defined(NANOHA)
	uint32_t handValue_of_side() const {return hand[sideToMove].h; }
	template<Color us> uint32_t handValue() const {return hand[us].h; }
	int get_material() const { return material; }
#endif

#if defined(NANOHA)
	static unsigned char relate_pos(int z1, int z2) {return DirTbl[z1][z2];}	// z1とz2の位置関係.
#endif

private:
  // Initialization helpers (used while setting up a position)
  void clear();
#ifndef NANOHA
  void set_castling_right(Color c, Square rfrom);
#endif
  void set_state(StateInfo* si) const;

public:
	bool move_is_legal(const Move m) const;

private:

#if defined(NANOHA)
	Key compute_key() const;
	int compute_material() const;
	void init_position(const unsigned char board_ori[9][9], const int Mochigoma_ori[]);
	void make_pin_info();
	void init_effect();
#endif

#ifndef NANOHA
  // Other helpers
  Bitboard check_blockers(Color c, Color kingColor) const;
  void put_piece(Color c, PieceType pt, Square s);
  void remove_piece(Color c, PieceType pt, Square s);
  void move_piece(Color c, PieceType pt, Square from, Square to);
  template<bool Do>
  void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

  // Data members
  Piece board[SQUARE_NB];
  Bitboard byTypeBB[PIECE_TYPE_NB];
  Bitboard byColorBB[COLOR_NB];
  int pieceCount[COLOR_NB][PIECE_TYPE_NB];
  Square pieceList[COLOR_NB][PIECE_TYPE_NB][16];
  int index[SQUARE_NB];
  int castlingRightsMask[SQUARE_NB];
  Square castlingRookSquare[CASTLING_RIGHT_NB];
  Bitboard castlingPath[CASTLING_RIGHT_NB];
#endif
  StateInfo startState;
  uint64_t nodes;
  int gamePly;
  Color sideToMove;
  Thread* thisThread;
  StateInfo* st;
#if defined(NANOHA)
	int64_t tnodes;
	unsigned long count_Mate1plyDrop;		// 駒打ちで詰んだ回数
	unsigned long count_Mate1plyMove;		// 駒移動で詰んだ回数
	unsigned long count_Mate3ply;			// Mate3()で詰んだ回数
#endif

#ifndef NANOHA
  bool chess960;
#endif

#if defined(NANOHA)
	Piece banpadding[16*2];		// Padding
	Piece ban[16*12];			// 盤情報 (駒種類)
	PieceNumber_t komano[16*12];		// 盤情報 (駒番号)
#define MAX_KOMANO	40
	effect_t effect[2][16*12];				// 利き
#define effectB	effect[BLACK]
#define effectW	effect[WHITE]

#define IsCheckS()	EXIST_EFFECT(effectW[kingS])	/* 先手玉に王手がかかっているか? */
#define IsCheckG()	EXIST_EFFECT(effectB[kingG])	/* 後手玉に王手がかかっているか? */
	int pin[16*10];					// ピン(先手と後手両用)
	Hand hand[2];					// 持駒
#define handS	hand[BLACK]
#define handG	hand[WHITE]
	PieceKind_t knkind[MAX_PIECENUMBER+1];	// knkind[num] : 駒番号numの駒種類(EMP(0x00) ～ GRY(0x1F))
	uint8_t knpos[MAX_PIECENUMBER+1];		// knpos[num]  : 駒番号numの盤上の座標(0:未使用、1:先手持駒、2:後手持駒、0x11-0x99:盤上)
									//    駒番号
#define KNS_SOU	1
#define KNE_SOU	1
								//        1    : 先手玉
#define KNS_GOU	2
#define KNE_GOU	2
								//        2    : 後手玉
#define KNS_HI	3
#define KNE_HI	4
								//        3- 4 : 飛
#define KNS_KA	5
#define KNE_KA	6
								//        5- 6 : 角
#define KNS_KI	7
#define KNE_KI	10
								//        7-10 : 金
#define KNS_GI	11
#define KNE_GI	14
								//       11-14 : 銀
#define KNS_KE	15
#define KNE_KE	18
								//       15-18 : 桂
#define KNS_KY	19
#define KNE_KY	22
								//       19-22 : 香
#define KNS_FU	23
#define KNE_FU	40
								//       23-40 : 歩
#define kingS	sq_king<BLACK>()
#define kingG	sq_king<WHITE>()
#define hiPos	(&knpos[ 3])
#define kaPos	(&knpos[ 5])
#define kyPos	(&knpos[19])
#define IsHand(x)	((x) <  0x11)
#define OnBoard(x)	((x) >= 0x11)
	int material;
	bool bInaniwa;

#endif

#if defined(NANOHA)
//  static Key zobrist[2][RY+1][0x100];
	static Key zobrist[GRY+1][0x100];
	static Key zobSideToMove;		// 手番を区別する
	static Key zobExclusion;		// NULL MOVEかどうか区別する
#ifdef USAPYON2
	static Key zobHand[GRY + 1][32];
#endif
	static unsigned char DirTbl[0xA0][0x100];	// 方向用[from][to]

	// 王手生成用テーブル
	static const struct ST_OuteMove2 {
		int from;
		struct {
			int to;
			int narazu;
			int nari;
		} to[6];
	} OuteMove2[32];
	static uint32_t TblMate1plydrop[0x10000];	// 駒打ちで詰む判断をするテーブル.

	friend void init_application_once();	// 実行ファイル起動時に行う初期化.
	friend class SearchMateDFPN;
	friend class Book;
#endif

};

extern std::ostream& operator<<(std::ostream& os, const Position& pos);

inline Color Position::side_to_move() const {
  return sideToMove;
}

inline bool Position::empty(Square s) const {
#ifdef NANOHA
	return ban[s] == EMP;
#else
	return board[s] == NO_PIECE;
#endif
}

#if defined(NANOHA)
inline Value Position::promote_value_of_piece_on(Square s) const {
	return Value(NanohaTbl::KomaValuePro[type_of(piece_on(s))]);
}

inline int Position::pin_on(Square s) const {
	return pin[s];
}
#endif

inline Piece Position::piece_on(Square s) const {
#ifdef NANOHA  
	return ban[s];
#else
	return board[s];
#endif
}

inline Piece Position::moved_piece(Move m) const {
	return move_piece(m);
}

#ifndef NANOHA

inline Bitboard Position::pieces() const {
  return byTypeBB[ALL_PIECES];
}

inline Bitboard Position::pieces(PieceType pt) const {
  return byTypeBB[pt];
}

inline Bitboard Position::pieces(PieceType pt1, PieceType pt2) const {
  return byTypeBB[pt1] | byTypeBB[pt2];
}

inline Bitboard Position::pieces(Color c) const {
  return byColorBB[c];
}

inline Bitboard Position::pieces(Color c, PieceType pt) const {
  return byColorBB[c] & byTypeBB[pt];
}

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2) const {
  return byColorBB[c] & (byTypeBB[pt1] | byTypeBB[pt2]);
}

template<PieceType Pt> inline int Position::count(Color c) const {
  return pieceCount[c][Pt];
}

template<PieceType Pt> inline const Square* Position::squares(Color c) const {
  return pieceList[c][Pt];
}

template<PieceType Pt> inline Square Position::square(Color c) const {
  assert(pieceCount[c][Pt] == 1);
  return pieceList[c][Pt][0];
}

inline Square Position::ep_square() const {
  return st->epSquare;
}

inline int Position::can_castle(CastlingRight cr) const {
  return st->castlingRights & cr;
}

inline int Position::can_castle(Color c) const {
  return st->castlingRights & ((WHITE_OO | WHITE_OOO) << (2 * c));
}

inline bool Position::castling_impeded(CastlingRight cr) const {
  return byTypeBB[ALL_PIECES] & castlingPath[cr];
}

inline Square Position::castling_rook_square(CastlingRight cr) const {
  return castlingRookSquare[cr];
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s) const {
  return  Pt == BISHOP || Pt == ROOK ? attacks_bb<Pt>(s, byTypeBB[ALL_PIECES])
        : Pt == QUEEN  ? attacks_from<ROOK>(s) | attacks_from<BISHOP>(s)
        : StepAttacksBB[Pt][s];
}

template<>
inline Bitboard Position::attacks_from<PAWN>(Square s, Color c) const {
  return StepAttacksBB[make_piece(c, PAWN)][s];
}

inline Bitboard Position::attacks_from(Piece pc, Square s) const {
  return attacks_bb(pc, s, byTypeBB[ALL_PIECES]);
}

inline Bitboard Position::attackers_to(Square s) const {
  return attackers_to(s, byTypeBB[ALL_PIECES]);
}

inline Bitboard Position::checkers() const {
  return st->checkersBB;
}

inline Bitboard Position::discovered_check_candidates() const {
  return check_blockers(sideToMove, ~sideToMove);
}

inline Bitboard Position::pinned_pieces(Color c) const {
  return check_blockers(c, c);
}

inline bool Position::pawn_passed(Color c, Square s) const {
  return !(pieces(~c, PAWN) & passed_pawn_mask(c, s));
}

inline bool Position::advanced_pawn_push(Move m) const {
  return   type_of(moved_piece(m)) == PAWN
        && relative_rank(sideToMove, from_sq(m)) > RANK_4;
}
#endif

inline bool Position::in_check() const {
#if defined(NANOHA)
	int pos = (sideToMove==BLACK) ? sq_king<BLACK>(): sq_king<WHITE>();
	if (pos == 0) return false;
	Color them = flip(sideToMove);
	return EXIST_EFFECT(effect[them][pos]);
#else
	return st->checkersBB != 0;
#endif
}

#if defined(NANOHA)
inline bool Position::at_checking() const {
	int pos = (sideToMove == BLACK) ? sq_king<WHITE>() : sq_king<BLACK>();
	if (pos == 0) return false;
	return EXIST_EFFECT(effect[sideToMove][pos]);
}
#endif

inline Key Position::key() const {
  return st->key;
}

#ifndef NANOHA
inline Key Position::pawn_key() const {
  return st->pawnKey;
}

inline Key Position::material_key() const {
  return st->materialKey;
}

inline Score Position::psq_score() const {
  return st->psq;
}

inline Value Position::non_pawn_material(Color c) const {
  return st->nonPawnMaterial[c];
}
#endif

inline int Position::game_ply() const {
  return gamePly;
}

#ifndef NANOHA
inline int Position::rule50_count() const {
  return st->rule50;
}
#endif

inline uint64_t Position::nodes_searched() const {
  return nodes;
}

inline void Position::set_nodes_searched(uint64_t n) {
  nodes = n;
}

#if defined(NANOHA)
inline int64_t Position::tnodes_searched() const {
	return tnodes;
}

inline void Position::set_tnodes_searched(int64_t n) {
	tnodes = n;
}

#if defined(CHK_PERFORM)
inline unsigned long Position::mate3_searched() const {
	return count_Mate3ply;
}
inline void Position::set_mate3_searched(unsigned long  n) {
	count_Mate3ply = n;
}
inline void Position::inc_mate3_searched(unsigned long  n) {
	count_Mate3ply += n;
}
#endif // defined(CHK_PERFORM)

#endif

#ifndef NANOHA
inline bool Position::opposite_bishops() const {
  return   pieceCount[WHITE][BISHOP] == 1
        && pieceCount[BLACK][BISHOP] == 1
        && opposite_colors(square<BISHOP>(WHITE), square<BISHOP>(BLACK));
}

inline bool Position::is_chess960() const {
  return chess960;
}
#endif

inline bool Position::capture_or_promotion(Move m) const {
#if defined(NANOHA)
	return move_captured(m) != EMP || is_promotion(m);
#else

  assert(is_ok(m));
  return type_of(m) != NORMAL ? type_of(m) != CASTLING : !empty(to_sq(m));
#endif
}

inline bool Position::capture(Move m) const {
#if defined(NANOHA)
	return !empty(move_to(m));
#else

  // Castling is encoded as "king captures the rook"
  assert(is_ok(m));
  return (!empty(to_sq(m)) && type_of(m) != CASTLING) || type_of(m) == ENPASSANT;
#endif
}

inline PieceType Position::captured_piece_type() const {
#if defined(NANOHA)
	return type_of(st->captured);
#else
  return st->capturedType;
#endif
}

inline Thread* Position::this_thread() const {
  return thisThread;
}

#ifndef NANOHA
inline void Position::put_piece(Color c, PieceType pt, Square s) {

  board[s] = make_piece(c, pt);
  byTypeBB[ALL_PIECES] |= s;
  byTypeBB[pt] |= s;
  byColorBB[c] |= s;
  index[s] = pieceCount[c][pt]++;
  pieceList[c][pt][index[s]] = s;
  pieceCount[c][ALL_PIECES]++;
}

inline void Position::remove_piece(Color c, PieceType pt, Square s) {

  // WARNING: This is not a reversible operation. If we remove a piece in
  // do_move() and then replace it in undo_move() we will put it at the end of
  // the list and not in its original place, it means index[] and pieceList[]
  // are not guaranteed to be invariant to a do_move() + undo_move() sequence.
  byTypeBB[ALL_PIECES] ^= s;
  byTypeBB[pt] ^= s;
  byColorBB[c] ^= s;
  /* board[s] = NO_PIECE;  Not needed, overwritten by the capturing one */
  Square lastSquare = pieceList[c][pt][--pieceCount[c][pt]];
  index[lastSquare] = index[s];
  pieceList[c][pt][index[lastSquare]] = lastSquare;
  pieceList[c][pt][pieceCount[c][pt]] = SQ_NONE;
  pieceCount[c][ALL_PIECES]--;
}

inline void Position::move_piece(Color c, PieceType pt, Square from, Square to) {

  // index[from] is not updated and becomes stale. This works as long as index[]
  // is accessed just by known occupied squares.
  Bitboard from_to_bb = SquareBB[from] ^ SquareBB[to];
  byTypeBB[ALL_PIECES] ^= from_to_bb;
  byTypeBB[pt] ^= from_to_bb;
  byColorBB[c] ^= from_to_bb;
  board[from] = NO_PIECE;
  board[to] = make_piece(c, pt);
  index[to] = index[from];
  pieceList[c][pt][index[to]] = to;
}
#endif

#if defined(NANOHA)
// 以下、なのは固有制御
template<Color us>
effect_t Position::exist_effect(int pos) const {
	return effect[us][pos];
}

// 指定位置から指定方向に何かある位置まで探す(WALL or 先手駒 or 後手駒の位置になる)
inline int Position::SkipOverEMP(int pos, const int dir) const {
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	return pos + dir;
}

// 二歩チェック(true:posの筋に歩がある＝二歩になる、false:posの筋に歩がない)
inline bool Position::is_double_pawn(const Color us, const int pos) const
{
	const Piece *p = &(ban[(pos & ~0x0F)+1]);
	Piece pawn = (us == BLACK) ? SFU : GFU;
	for (int i = 0; i < 9; i++) {
		if (*p++ == pawn) {
			return true;
		}
	}
	return false;
}

// 利き関連
template<Color turn>
inline void Position::add_effect_straight(const int z, const int dir, const uint32_t bit)
{
	int zz = z;
	do {
		zz += dir;
		effect[turn][zz] |= bit;
	} while(ban[zz] == EMP);

	// 利きは相手玉を一つだけ貫く
	const int enemyKing = (turn == BLACK) ? GOU : SOU;
	if (ban[zz] == enemyKing) {
		zz += dir;
		if (ban[zz] != WALL) {
			effect[turn][zz] |= bit;
		}
	}
}
template<Color turn>
inline void Position::del_effect_straight(const int z, const int dir, const uint32_t bit)
{
	int zz = z;
	do {
		zz += dir; effect[turn][zz] &= bit;
	} while(ban[zz] == EMP);

	// 利きは相手玉を一つだけ貫く
	const int enemyKing = (turn == BLACK) ? GOU : SOU;
	if (ban[zz] == enemyKing) {
		zz += dir;
		if (ban[zz] != WALL) {
			effect[turn][zz] &= bit;
		}
	}
}

// ピン情報更新
template<Color turn>
inline void Position::add_pin_info(const int dir) {
	int z;
	const Color rturn = (turn == BLACK) ? WHITE : BLACK;
	z = (turn == BLACK) ? SkipOverEMP(kingS, -dir) : SkipOverEMP(kingG, -dir);
	if (ban[z] != WALL) {
		if ((turn == BLACK && (ban[z] & GOTE) == 0)
		 || (turn == WHITE && (ban[z] & GOTE) != 0)) {
			effect_t eft = (turn == BLACK) ? EFFECT_KING_S(z) : EFFECT_KING_G(z);
			if (eft & (effect[rturn][z] >> EFFECT_LONG_SHIFT)) pin[z] = dir;
		}
	}
}
template<Color turn>
void Position::del_pin_info(const int dir) {
	int z;
	z = (turn == BLACK) ? SkipOverEMP(kingS, -dir) : SkipOverEMP(kingG, -dir);
	if (ban[z] != WALL) {
		if ((turn == BLACK && (ban[z] & GOTE) == 0)
		 || (turn == WHITE && (ban[z] & GOTE) != 0)) {
			pin[z] = 0;
		}
	}
}
#endif


#endif // #ifndef POSITION_H_INCLUDED
