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

#include <cassert>
#include <fstream>
#include <iostream>
#include "movegen.h"
#include "position.h"

#define USE_M3HASH	// Mate3()結果をハッシュする.

#if !defined(NDEBUG)
#define DEBUG_MATE2
#endif

#if defined(DEBUG_MATE2)
Move pre_check;
#endif

#if defined(USE_M3HASH)
#define MATE3_MASK              0x07ffffU	/* 8 * 64K = 512K entry */

// パフォーマンス計測用.
namespace M3Perform{
uint64_t called;
uint64_t hashhit;
uint64_t override;
}

namespace {
struct Mate3_Hash {
	uint64_t word1, word2;				// word1:key, word2:0-31=black_hand, word2:32-63=move
} mate3_hash_tbl[ MATE3_MASK + 1 ];

bool probe_m3hash(const Position& pos, Move &m)
{
	uint64_t word1, word2;
	const uint64_t key = pos.key();
	const uint32_t h = pos.handValue<BLACK>();
	const uint32_t index = static_cast<uint32_t>(key) ^ h ^ (h >> 15);
	const Mate3_Hash now = mate3_hash_tbl[index & MATE3_MASK];
	word1 = now.word1 ^ now.word2;
	word2 = now.word2;

	if (word1 != key || static_cast<uint32_t>(word2) != pos.handValue<BLACK>()) {
		return false;
	}
	m = static_cast<Move>(word2 >> 32);
	return true;
}

void store_m3hash(const Position& pos, const Move m)
{
	Mate3_Hash now;
	const uint64_t key = pos.key();
	const uint32_t h = pos.handValue<BLACK>();
	now.word2 = static_cast<uint64_t>(h) | (static_cast<uint64_t>(m) << 32);
	now.word1 = key ^ now.word2;

	const uint32_t index = static_cast<uint32_t>(key) ^ h ^ (h >> 15);
	if (mate3_hash_tbl[index & MATE3_MASK].word1 != 0) M3Perform::override++;
	mate3_hash_tbl[index & MATE3_MASK] = now;
}

}
#endif

//
//  玉との位置関係
//
// +---+---+---+---+---+
// |+30|+14| -2|-18|-34|
// +---+---+---+---+---+
// |+31|+15| -1|-17|-33|
// +---+---+---+---+---+
// |+32|+16| 玉|-16|-32|
// +---+---+---+---+---+
// |+33|+17| +1|-15|-31|
// +---+---+---+---+---+
// |+34|+18| +2|-14|-30|
// +---+---+---+---+---+
// |+35|+19| +3|-13|-29|
// +---+---+---+---+---+
// |+36|+20| +4|-12|-28|
// +---+---+---+---+---+
//
//
// このテーブルでは歩、桂、銀、金、と、成香、成桂、成銀のみを扱う.
// 香、角、飛、馬、竜は別で処理する.
const struct Position::ST_OuteMove2 Position::OuteMove2[32] = {
	// +36の位置の駒から王手.
	{+36, {
		{+18,	// 桂のみ
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	//  +4の位置の駒から王手.
	{ +4, {
		{+18,	// 桂のみ
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{-14,	// 桂のみ
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// -28の位置の駒から王手.
	{-28, {
		{-14,	// 桂のみ
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// +35の位置の駒から王手.
	{+35, {
		{+17,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// +19の位置の駒から王手.
	{+19, {
		{ +1,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	//  +3の位置の駒から王手.
	{ +3, {
		{+17,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{-15,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// -13の位置の駒から王手.
	{-13, {
		{ +1,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// -29の位置の駒から王手.
	{-29, {
		{-15,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// +34の位置の駒から王手.
	{+34, {
		{+17,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{+16,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// +18の位置の駒から王手.
	{+18, {
		{+17,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(1<<RY),
			(1<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{ +1,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	//  +2の位置の駒から王手.
	{ +2, {
		{+17,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{-15,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{+16,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{-16,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ +1,
			(1<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(0<<RY),
			(1<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 11:-14の位置の駒から王手.
	{-14, {
		{-15,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(1<<RY),
			(1<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{ +1,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 12:-30の位置の駒から王手.
	{-30, {
		{-15,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{-16,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 13:+33の位置の駒から王手.
	{+33, {
		{+17,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{+16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 14:+17の位置の駒から王手.
	{+17, {
		{+16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(1<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	//  +1の位置の駒から王手.	⇒なし
	// 15:-15の位置の駒から王手.
	{-15, {
		{-16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(1<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,	// 桂成のみ
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(1<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 16:-31の位置の駒から王手.
	{-31, {
		{-15,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{-16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 17:+32の位置の駒から王手.
	{+32, {
		{+17,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{+16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{+15,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 18:+16の位置の駒から王手.
	{+16, {
		{+15,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ +1,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 19:-16の位置の駒から王手.
	{-16, {
		{-17,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ +1,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 20:-32の位置の駒から王手.
	{-32, {
		{-15,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{-16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{-17,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 21:+31の位置の駒から王手.
	{+31, {
		{+16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{+15,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 22:+15の位置の駒から王手.
	{+15, {
		{+16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 23: -1の位置の駒から王手.
	{ -1, {
		{+16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{-16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 24:-17の位置の駒から王手.
	{-17, {
		{-16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(0<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 25:-33の位置の駒から王手.
	{-33, {
		{-16,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{-17,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 26:+30の位置の駒から王手.
	{+30, {
		{+15,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 27:+14の位置の駒から王手.
	{+14, {
		{+15,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 28: -2の位置の駒から王手.
	{ -2, {
		{+15,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{-17,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(1<<KI)|(0<<KA)|(0<<HI)| (0x0F << TO) |(1<<UM)|(0<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 29:-18の位置の駒から王手.
	{-18, {
		{-17,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{ -1,
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(1<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 30:-34の位置の駒から王手.
	{-34, {
		{-17,
			(0<<FU)|(0<<KY)|(0<<KE)|(1<<GI)|(0<<KI)|(0<<KA)|(0<<HI)| (0x00 << TO) |(0<<UM)|(1<<RY),
			(0<<FU)|(0<<KY)|(0<<KE)|(0<<GI)        |(0<<KA)|(0<<HI)},
		{0,0,0},
	}},

	// 終端.
	{ 0, {
		{0,0,0},
	}},
};

// 盤上の跳びのある駒での王手を生成する.
template<Color us>
MoveStack* Position::gen_check_long(MoveStack* mlist) const
{
	// 手番側から見て、相手の玉の位置
	const int enemyKing = (us == BLACK) ? kingG : kingS;
	if (enemyKing == 0) return mlist;

	const int ksuji = enemyKing & 0xF0;
	const int kdan  = enemyKing & 0x0F;
	int suji, dan;
	int dir;
	int to;

	// 内容
	// 飛・龍による王手
		// 1. 飛・龍と玉の間の駒を動かす or 取る
		// 2. 飛竜が空いている飛の道にのる
		// 3. 飛が成ること及び竜による王手

	// 角・馬による王手
		// 1. 角・馬と玉の間の駒を動かす or 取る
		// 2. 角・馬が空いている道にのることによる王手
		// 3. 角が成ること及び馬による王手

	// 香車による王手
		// 1. 香車と玉の間の駒を動かす or 取る
		// 2. 香が成ることによる王手


	// 飛・龍による王手
	for (int hi = 0; hi < 2; hi++) {
		const int sq = hiPos[hi];
		// 持駒は処理しない
		if (!OnBoard(sq)) continue;
		// 相手の駒は処理しない
		if (color_of(ban[sq]) != us) continue;

		suji = sq & 0xF0;
		dan  = sq & 0x0F;
		if (suji == ksuji || dan == kdan) {
			// 1. 飛・龍と玉の間の駒を動かす or 取る (同じ段 or 同じ筋にいて、間の駒が1つ)
			if (dan > kdan) {
				dir = 1;
			} else if (dan < kdan) {
				dir = -1;
			} else if (suji > ksuji) {
				dir = 16;
			} else {
				dir = -16;
			}
			to = enemyKing + dir;
			while (ban[to] == EMP) {
				to = to + dir;
			}
			if (sq == SkipOverEMP(to, dir)) {
				if (color_of(ban[to]) != us) {
					// toにいる駒は敵
					if ((pin[sq]==0 || pin[sq]==dir || pin[sq]==-dir)) {
						if ((ban[sq] & PROMOTED) == 0 && (can_promotion<us>(to) || can_promotion<us>(sq))) {
							// 成る手
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1, MOVE_CHECK_LONG);
							// 不成り
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG | MOVE_CHECK_NARAZU);
						} else {
							// 不成り
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG);
						}
					}
				} else {
					// toにいる駒は味方
					mlist = gen_move_from(us, mlist, to, dir);
				}
			}
		} else {
			// 2. 飛竜が空いている飛の道にのる(同じ段や同じ筋にいない)
			int xmin = Min(ksuji, suji);
			int xmax = Max(ksuji, suji);
			int ymin = Min(kdan, dan);
			int ymax = Max(kdan, dan);
			int x, y;
			bool moveH, moveV;
			moveH = moveV = true;
			for (x = xmin + 16; x < xmax; x += 16) {
				if (ban[x +  dan] != EMP) moveH = false;	// 飛の横方向に駒があれば、飛が横に動く王手はない.
				if (ban[x + kdan] != EMP) moveV = false;	// 玉の横方向に駒があれば、飛が縦に動く王手はない.
			}
			for (y = ymin + 1; y < ymax; y++) {
				if (ban[ksuji + y] != EMP) moveH = false;	// 玉の縦方向に駒があれば、飛が横に動く王手はない.
				if (ban[suji  + y] != EMP) moveV = false;	// 飛の縦方向に駒があれば、飛が縦に動く王手はない.
			}
			// 飛車が横に動く王手
			if (moveH) {
				if ((ban[ksuji + dan] == EMP || color_of(ban[ksuji + dan]) != us) 
				  && (pin[sq] == 0 || pin[sq] == 16 || pin[sq] == -16)) {
					if ((ban[sq] & PROMOTED) == 0 && can_promotion<us>(sq)) {
						(mlist++)->move = cons_move(sq, ksuji + dan, ban[sq], ban[ksuji + dan], 1, MOVE_CHECK_LONG);
						(mlist++)->move = cons_move(sq, ksuji + dan, ban[sq], ban[ksuji + dan], 0, MOVE_CHECK_LONG | MOVE_CHECK_NARAZU);
					} else {
						(mlist++)->move = cons_move(sq, ksuji + dan, ban[sq], ban[ksuji + dan], 0, MOVE_CHECK_LONG);
					}
				}
			}
			// 飛車が縦に動く王手
			if (moveV) {
				if ((ban[suji + kdan] == EMP || color_of(ban[suji + kdan]) != us) 
				  && (pin[sq] == 0 || pin[sq] == 1 || pin[sq] == -1)) {
					if ((ban[sq] & PROMOTED) == 0 && (can_promotion<us>(sq) || can_promotion<us>(enemyKing))) {
						(mlist++)->move = cons_move(sq, suji + kdan, ban[sq], ban[suji + kdan], 1, MOVE_CHECK_LONG);
						(mlist++)->move = cons_move(sq, suji + kdan, ban[sq], ban[suji + kdan], 0, MOVE_CHECK_LONG | MOVE_CHECK_NARAZU);
					} else {
						(mlist++)->move = cons_move(sq, suji + kdan, ban[sq], ban[suji + kdan], 0, MOVE_CHECK_LONG);
					}
				}
			}
			// 竜が斜めに動く手を生成する
			if (ban[sq] & PROMOTED)
			for (int dx = -0x10; dx <= 0x10; dx += 0x20) {
				for (int dy = -1; dy <= 1; dy += 2) {
					if (pin[sq] != 0 && pin[sq] != dx + dy && pin[sq] != -dx - dy) continue;
					if (ban[sq+dx+dy] == WALL || (ban[sq+dx+dy] != EMP && color_of(ban[sq + dx + dy]) == us)) continue;	// 味方の駒
					if (kdan == dan + dy) {
						//斜めに動いて横に王手
						int sx = Min(ksuji, suji + dx);
						int lx = Max(ksuji, suji + dx);
						moveV = true;
						for (x = sx + 16; x < lx; x += 16) {
							if (ban[x + kdan] != EMP) moveV = false;
						}
						if (moveV) {
							(mlist++)->move = cons_move(sq, sq + dx + dy, ban[sq], ban[sq + dx + dy], 0, MOVE_CHECK_LONG);
						}
					}
					if (ksuji == suji + dx) {
						//斜めに動いて縦に王手
						int sy = Min(kdan, dan + dy);
						int ly = Max(kdan, dan + dy);
						moveH = true;
						for (y = sy + 1; y < ly; y++) {
							if (ban[y + ksuji] != EMP) moveH = false;
						}
						if (moveH) {
							(mlist++)->move = cons_move(sq, sq + dx + dy, ban[sq], ban[sq + dx + dy], 0, MOVE_CHECK_LONG);
						}
					}
				}
			}

			// 3. 飛が成ること及び竜による王手
			{
				static const int dirs[]={16,   1, -16, -1};
				static const int Positions[]={-17, -15,  17, 15};

				int i, j;
				for (i = 0; i < 4; i++) {
					to = enemyKing + Positions[i];
					if (ban[to] != EMP && (ban[to] == WALL || color_of(ban[to]) == us)) {	// 味方の駒
						continue;
					}
					for (j = 0; j < 4; j++) {
						dir = dirs[j];
						if (sq == SkipOverEMP(to, dir)) {
							if ((ban[sq] & PROMOTED) && (pin[sq] == 0 || pin[sq] == dir || pin[sq] == -dir)) {
								(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0);
							} else if ((ban[sq] & PROMOTED) == 0 && (pin[sq] == 0 || pin[sq] == dir || pin[sq] == -dir)
							  && ((can_promotion<us>(sq) || can_promotion<us>(to)))
								 ) {
								(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1);
							}
						}
					}
				}
			}
		}
	}

	// 角・馬による王手
	for (int ka = 0; ka < 2; ka++) {
		const int sq = kaPos[ka];
		// 持駒は処理しない
		if (!OnBoard(sq)) continue;
		// 相手の駒は処理しない
		if (color_of(ban[sq]) != us) continue;

		suji = sq & 0xF0;
		dan  = sq & 0x0F;

		// 1. 角・馬と玉の間の駒を動かす or 取る
		static const int ka_dir[] = {+15, +17};
		{
			int i;
			for (i = 0; i < static_cast<int>(sizeof(ka_dir)/sizeof(ka_dir[0])); i++) {
				dir = ka_dir[i];
				if ((sq - enemyKing) % dir == 0) {
					// 角・馬の dir 線上に玉がいる
					if (sq < enemyKing) dir = -dir;
					to = enemyKing + dir;
					while (ban[to] == EMP) {
						to += dir;
					}
					if (ban[to] != WALL) {
						if (sq == SkipOverEMP(to, dir)) {
							// 角・馬と玉の間に1つ駒がある
							if (color_of(ban[to]) != us) {
								// toにいる駒は敵
								if (pin[sq] == 0 || pin[sq] == dir || pin[sq] == -dir) {
									if ((ban[sq] & PROMOTED) == 0
										&& (can_promotion<us>(to) || can_promotion<us>(sq))) {
										// 成る手
										(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1, MOVE_CHECK_LONG);
										(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG | MOVE_CHECK_NARAZU);
									} else {
										(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG);
									}
								}
							} else {
								// toにいる駒は味方
								mlist = gen_move_from(us, mlist, to, dir);
							}
						}
					}
				}
			}
		}

		// 未：2. 角・馬が空いている道にのることによる王手
		int kx = enemyKing >> 4;
		int ky = enemyKing & 0x0f;
		int bx = suji >> 4;
		int by = dan;
		int k1, k2, b1, b2;
		k1 = ky + kx;
		k2 = ky - kx;
		b1 = by + bx;
		b2 = by - bx;
		// 一直線ならばここでは生成しない
		if (k1 != b1 && k2 != b2) {
			// 角・馬と玉が直線上にない
			if ((k1 & 1) == (b1 & 1)) {
				// 角の筋に玉がいる
				int x1, y1;
				x1 = (k1 - b2) / 2;
				y1 = (b2 + k1) / 2;
				dir = 17;
				if ((x1 >= 1) && (x1 <= 9) && (y1 >= 1) && (y1 <= 9)
					&& ((pin[sq] == 0) || (pin[sq] == dir) || (pin[sq] == -dir))
					&& (ban[to = ((x1 << 4)+y1)] == EMP || color_of(ban[to]) != us)) {
					bool moveF = true;
					int s = Min(sq, to);
					int l = Max(sq, to);
					int j;
					for (j = s + dir; j < l; j += dir) {
						if (ban[j] != EMP) moveF = false;
					}
					s = Min(to, enemyKing);
					l = Max(to, enemyKing);
					dir = 15;
					for (j = s + dir; j < l; j += dir) {
						if (ban[j] != EMP) moveF = false;
					}
					if (moveF) {
						if (((ban[sq] & PROMOTED) == 0)
						  && (can_promotion<us>(sq) || can_promotion<us>(to))) {
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1, MOVE_CHECK_LONG);
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG | MOVE_CHECK_NARAZU);
						} else {
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG);
						}
					}
				}
				x1 = (b1 - k2) / 2;
				y1 = (k2 + b1) / 2;
				dir = 15;
				if ((x1 >= 1) && (x1 <= 9) && (y1 >= 1) && (y1 <= 9)
					&& ((pin[sq] == 0) || (pin[sq] == dir) || (pin[sq] == -dir))
					&& (ban[to = ((x1 << 4)+y1)] == EMP || color_of(ban[to]) != us)) {
					bool moveF = true;
					int s = Min(sq, to);
					int l = Max(sq, to);
					int j;
					for (j = s + dir; j < l; j += dir) {
						if (ban[j] != EMP) moveF = false;
					}
					s = Min(to, enemyKing);
					l = Max(to, enemyKing);
					dir = 17;
					for (j = s + dir; j < l; j += dir) {
						if (ban[j] != EMP) moveF = false;
					}
					if (moveF) {
						if (((ban[sq] & PROMOTED) == 0)
						  && (can_promotion<us>(sq) || can_promotion<us>(to))) {
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1, MOVE_CHECK_LONG);
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG | MOVE_CHECK_NARAZU);
						} else {
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG);
						}
					}
				}
			} else {
				// 角の筋に玉がいない
				// ⇒馬が縦横に動く手を生成する
				if (ban[sq] & PROMOTED) {
					static const int dirs[4] = {-16, -1, 1, 16};
					static const int p[4] = {-1, -1, +1, +1};	// y+x の増減
					static const int m[4] = {+1, -1, +1, -1};	// y-x の増減
					for (int dirID = 0; dirID < 4; dirID++) {
						if (pin[sq] != 0 && pin[sq] != dirs[dirID] && pin[sq] != -dirs[dirID]) continue;
						if (ban[sq + dirs[dirID]] == WALL || (ban[sq + dirs[dirID]] != EMP && color_of(ban[sq + dirs[dirID]]) == us)) continue;		// 味方の駒
						if (k1 == b1 + p[dirID]) {
							//動いてp方向に王手
							int s = Min(enemyKing, sq + dirs[dirID]);
							int l = Max(enemyKing, sq + dirs[dirID]);
							bool moveF = true;
							for (int j = s + 15; j < l; j += 15) {
								if (ban[j] != EMP) moveF = false;
							}
							if (moveF) {
								(mlist++)->move = cons_move(sq, sq + dirs[dirID], ban[sq], ban[sq + dirs[dirID]], 0, MOVE_CHECK_LONG);
							}
						}
						if (k2 == b2 + m[dirID]) {
							//動いてm方向に王手
							int s = Min(enemyKing, sq + dirs[dirID]);
							int l = Max(enemyKing, sq + dirs[dirID]);
							bool moveF = true;
							for (int j = s + 17; j < l; j += 17) {
								if (ban[j] != EMP) moveF = false;
							}
							if (moveF) {
								(mlist++)->move = cons_move(sq, sq + dirs[dirID], ban[sq], ban[sq + dirs[dirID]], 0, MOVE_CHECK_LONG);
							}
						}
					}
				}
			}
		}

		// 未：3. 角が成ること及び馬による王手
		{
			static const int dirs[] = {-17, -15,  17, 15};
			static const int Positions[]  = { 16,   1, -16, -1};
		
			int i, j;
			for (i = 0; i < 4; i++) {
				to = enemyKing + Positions[i];
				if (ban[to] != EMP && (ban[to] == WALL || color_of(ban[to]) == us)) {		// 味方の駒
					continue;
				}
				for (j = 0; j < 4; j++) {
					dir = dirs[j];
					if (sq == SkipOverEMP(to, dir)) {
						if ((ban[sq] & PROMOTED)
							&& (pin[sq] == 0 || pin[sq] == dir || pin[sq] == -dir)) {
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0);
						} else if ((ban[sq] & PROMOTED) == 0
							&& (pin[sq] == 0 || pin[sq] == dir || pin[sq] == -dir)
							&& (can_promotion<us>(sq) || can_promotion<us>(to))
							) {
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1);
						}
					}
				}
			}
		}
	}

	// 香車による王手
	for (int ky = 0; ky < 4; ky++) {
		const int sq = kyPos[ky];
		// 持駒は処理しない
		if (!OnBoard(sq)) continue;

		// 相手の駒は処理しない
		if (color_of(ban[sq]) != us) continue;
		// 成香は処理しない
		if (ban[sq] & PROMOTED) continue;

		suji = sq & 0xF0;
		dan  = sq & 0x0F;

		if (us == BLACK && dan > kdan) {
			dir = 1;
		} else if (us != BLACK && dan < kdan) {
			dir = -1;
		} else {
			continue;
		}

		// 1. 香車と玉の間の駒を動かす or 取る
		if (suji == ksuji) {
///			to = enemyKing + dir;
///			while (ban[to] == EMP) {
///				to = to + dir;
///			}
			to = SkipOverEMP(enemyKing, dir);
			if (sq == SkipOverEMP(to, dir)) {
				if (color_of(ban[to]) != us) {
					// toにいる駒は敵
					if ((pin[sq] == 0 || pin[sq] == dir || pin[sq] == -dir)) {
						// 成っても王手=目の前に王がいる＆成れるなら成る手も生成しましょう
						if (to - dir == enemyKing) {
							if (can_promotion<us>(to)) {
								// 成る手
								(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1);
							}
							if ((us == BLACK && (to & 0x0F) == 2)
								|| (us != BLACK && (to & 0x0F) == 8)) {
								// 不成り
								(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG | MOVE_CHECK_NARAZU);
							} else {
								// 不成り
								(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG);
							}
						} else {
							// 不成り
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 0, MOVE_CHECK_LONG);
						}
					}
				} else {
					// toにいる駒は味方
					mlist = gen_move_from(us, mlist, to, dir);
				}
			}
		} else {
			// 2. 香が成ることによる王手
			to = 0;
			if (suji == ksuji + 16) {
				to = enemyKing + 16;
			} else if (suji == ksuji - 16) {
				to = enemyKing - 16;
			}
			if (to != 0) {
				int from = to + dir;
				if (ban[to] != EMP && color_of(ban[to]) == us) {	// 味方の駒
					to = from;
					from += dir;
				} else if (ban[from] != EMP && color_of(ban[from]) != us) {	// 敵の駒
					to = from;
					from += dir;
				}
				while (ban[from] == EMP) {
					from += dir;
				}
				if (from == sq && (pin[sq] == 0 || pin[sq] == dir || pin[sq] == -dir)) {
					if ((us == BLACK) && ((to & 0x0f) <= 3)) {
						if (ban[to] == EMP || color_of(ban[to]) != us) {
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1);
						}
						if ((to & 0x0F) == kdan && kdan <= 2 && (sq & 0x0F) != kdan + 1) {
							(mlist++)->move = cons_move(sq, to + dir, ban[sq], ban[to+dir], 1);
						}
					}
					if ((us != BLACK) && ((to & 0x0f) >= 7)) {
						if (ban[to] == EMP || color_of(ban[to]) != us) {
							(mlist++)->move = cons_move(sq, to, ban[sq], ban[to], 1);
						}
						if ((to & 0x0F) == kdan && kdan >= 8 && (sq & 0x0F) != kdan - 1) {
							(mlist++)->move = cons_move(sq, to + dir, ban[sq], ban[to+dir], 1);
						}
					}
				}
			}
		}
	}
	return mlist;
}

// 盤上の駒を動かす手で王手となるものを生成する.Ver.2
template<Color us>
MoveStack* Position::gen_check_short(MoveStack* mlist) const
{
	int enemyKing; 				// 手番側から見て、相手の玉の位置
	if (us == BLACK) {
		enemyKing = kingG;
	} else {
		enemyKing = kingS;
	}

	//
	//  玉との位置関係
	//
	// +---+---+---+---+---+
	// |+30|+14| -2|-18|-34|
	// +---+---+---+---+---+
	// |+31|+15| -1|-17|-33|
	// +---+---+---+---+---+
	// |+32|+16| 玉|-16|-32|
	// +---+---+---+---+---+
	// |+33|+17| +1|-15|-31|
	// +---+---+---+---+---+
	// |+34|+18| +2|-14|-30|
	// +---+---+---+---+---+
	// |+35|+19| +3|-13|-29|
	// +---+---+---+---+---+
	// |+36|+20| +4|-12|-28|
	// +---+---+---+---+---+
	//
	//


	int i;
	int j;

	if (us == BLACK) {
		// 先手
		i = -1;
		while (OuteMove2[++i].from) {
			const int from = enemyKing + OuteMove2[i].from;
			if (ban[from] == WALL) continue;
			const int koma = ban[from];
			if (koma != EMP && IsSente(koma)) {
				j = 0;
				while (OuteMove2[i].to[j].to) {
					int to = enemyKing + OuteMove2[i].to[j].to;
					if (ban[to] != WALL && (ban[to] == EMP || IsGote(ban[to]))) {
						if (pin[from] == 0 || pin[from] == to-from || pin[from] == from-to) {
							if (((OuteMove2[i].to[j].nari & (1 << (koma & 0x0F))) != 0) && ((to & 0x0f) <= 3 || (from & 0x0f) <= 3)) {
								(mlist++)->move = cons_move(from, to, ban[from], ban[to], 1);
							}
							if ((OuteMove2[i].to[j].narazu & (1 << (koma & 0x0F))) != 0) {
								(mlist++)->move = cons_move(from, to, ban[from], ban[to], 0);
							}
						}
					}
					j++;
				}
			}
		}
	} else {
		// 後手
		i = -1;
		while (OuteMove2[++i].from) {
			const int from = enemyKing - OuteMove2[i].from;
			if (ban[from] == WALL) continue;
			const int koma = ban[from];
			if (koma != EMP && IsGote(koma)) {
				j = 0;
				while (OuteMove2[i].to[j].to) {
					int to = enemyKing - OuteMove2[i].to[j].to;
					if (ban[to] != WALL && (ban[to] == EMP || IsSente(ban[to]))) {
						if (pin[from] == 0 || pin[from] == to-from || pin[from] == from-to) {
							if (((OuteMove2[i].to[j].nari & (1 << (koma & 0x0F))) != 0) && ((to & 0x0f) >= 7 || (from & 0x0f) >= 7)) {
								(mlist++)->move = cons_move(from, to, ban[from], ban[to], 1);
							}
							if ((OuteMove2[i].to[j].narazu & (1 << (koma & 0x0F))) != 0) {
								(mlist++)->move = cons_move(from, to, ban[from], ban[to], 0);
							}
						}
					}
					j++;
				}
			}
		}
	}
	return mlist;
}

// 駒を打つ手で王手となるものを生成する.
template <Color us>
MoveStack* Position::gen_check_drop(MoveStack* mlist, bool& bUchifudume) const
{
	int enemyKing = (us == BLACK) ? kingG : kingS;	// 手番側から見て、相手の玉の位置
	int suji, dan;

	const int ksuji = enemyKing & 0xF0;
	const int kdan  = enemyKing & 0x0F;

	//
	//  玉との位置関係
	//
	//  +28 +12 -4 -20 -36
	//  +29 +13 -3 -19 -35
	//  +30 +14 -2 -18 -34
	//  +31 +15 -1 -17 -33
	//  +32 +16 玉 -16 -32
	//  +33 +17 +1 -15 -31
	//  +34 +18 +2 -14 -30
	//  +35 +19 +3 -15 -29
	//  +36 +20 +4 -13 -28
	//
	//
	Piece piece;
	// 金を打つ
	if (((us == BLACK) ? handS.getKI() : handG.getKI()) > 0) {
		if (us == BLACK) {
			piece = SKI;
			if (ban[enemyKing +  1] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing +  1, piece, EMP);
			}
			if (ban[enemyKing + 17] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 17, piece, EMP);
			}
			if (ban[enemyKing - 15] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 15, piece, EMP);
			}
			if (ban[enemyKing + 16] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 16, piece, EMP);
			}
			if (ban[enemyKing - 16] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 16, piece, EMP);
			}
			if (ban[enemyKing -  1] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing -  1, piece, EMP);
			}
		} else {
			piece = GKI;
			if (ban[enemyKing -  1] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing  - 1, piece, EMP);
			}
			if (ban[enemyKing - 17] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 17, piece, EMP);
			}
			if (ban[enemyKing + 15] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 15, piece, EMP);
			}
			if (ban[enemyKing - 16] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 16, piece, EMP);
			}
			if (ban[enemyKing + 16] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 16, piece, EMP);
			}
			if (ban[enemyKing +  1] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing  + 1, piece, EMP);
			}
		}
	}
	// 銀を打つ
	if (((us == BLACK) ? handS.getGI() : handG.getGI()) > 0) {
		if (us == BLACK) {
			piece = SGI;
			if (ban[enemyKing +  1] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing +  1, piece, EMP);
			}
			if (ban[enemyKing - 17] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 17, piece, EMP);
			}
			if (ban[enemyKing - 15] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 15, piece, EMP);
			}
			if (ban[enemyKing + 15] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 15, piece, EMP);
			}
			if (ban[enemyKing + 17] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 17, piece, EMP);
			}
		}
		if (us != BLACK) {
			piece = GGI;
			if (ban[enemyKing -  1] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing -  1, piece, EMP);
			}
			if (ban[enemyKing + 17] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 17, piece, EMP);
			}
			if (ban[enemyKing + 15] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 15, piece, EMP);
			}
			if (ban[enemyKing - 15] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 15, piece, EMP);
			}
			if (ban[enemyKing - 17] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 17, piece, EMP);
			}
		}
	}
	if (us == BLACK) {
		// 先手番のとき
		// 桂を打つ
		if (handS.getKE() > 0 && kdan <= 7) {
			piece = SKE;
			if (ban[enemyKing - 14] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 14, piece, EMP);
			}
			if (ban[enemyKing + 18] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 18, piece, EMP);
			}
		}
		// 香を打つ
		suji = ksuji;
		dan = kdan + 1;
		if (handS.getKY() > 0 && ban[dan+suji] == EMP) {
			for (; dan <= 9; dan++) {
				if (ban[dan+suji]==EMP) {
					(mlist++)->move = cons_move(0, suji+dan, SKY, EMP);
				} else {
					break;
				}
			}
		}
		// 歩を打つ
		suji = ksuji;
		dan = kdan + 1;
		if (handS.getFU() > 0 && ban[dan+suji] == EMP) {
			// 二歩チェック
			int nifu = is_double_pawn(us, suji);
			if (nifu == 0) {
				// 打ち歩詰めもチェック
				dan = kdan + 1;
				if (!is_pawn_drop_mate(us, dan+suji)) {
					(mlist++)->move = cons_move(0, suji+dan, SFU, EMP);
				} else {
					bUchifudume = true;
				}
			}
		}
	}
	if (us != BLACK) {
		// 後手番のとき
		// 桂を打つ
		if (handG.getKE() > 0 && kdan >= 3) {
			piece = GKE;
			if (ban[enemyKing + 14] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing + 14, piece, EMP);
			}
			if (ban[enemyKing - 18] == EMP) {
				(mlist++)->move = cons_move(0, enemyKing - 18, piece, EMP);
			}
		}
		// 香を打つ
		suji = ksuji;
		dan = kdan - 1;
		if (handG.getKY() > 0 && ban[dan+suji] == EMP) {
			for (; dan >= 1; dan--) {
				if (ban[dan+suji]==EMP) {
					(mlist++)->move = cons_move(0, suji+dan, GKY, EMP);
				} else {
					break;
				}
			}
		}
		// 歩を打つ
		suji = ksuji;
		dan = kdan - 1;
		if (handG.getFU() > 0 && ban[dan+suji] == EMP) {
			// 二歩チェック
			int nifu = is_double_pawn(us, suji);
			if (nifu == 0) {
				// 打ち歩詰めもチェック
				dan = kdan - 1;
				if (!is_pawn_drop_mate(us, dan+suji)) {
					(mlist++)->move = cons_move(0, suji+dan, GFU, EMP);
				} else {
					bUchifudume = true;
				}
			}
		}
	}

	// 角と飛車は玉周辺から打つようにする
	// 角を打つ
	if (((us == BLACK) ? handS.getKA() : handG.getKA()) > 0) {
		static const int dir1 = -17;
		static const int dir2 = -15;
		static const int dir3 =  15;
		static const int dir4 =  17;
		int pos1, pos2, pos3, pos4;
		pos1 = pos2 = pos3 = pos4 = enemyKing;
		bool bl_1 = true;
		bool bl_2 = true;
		bool bl_3 = true;
		bool bl_4 = true;
		piece = (us == BLACK) ? SKA : GKA;
		while (bl_1 || bl_2 || bl_3 || bl_4) {
			if (bl_1 && ban[pos1 += dir1] == EMP) {
				(mlist++)->move = cons_move(0, pos1, piece, EMP);
			} else {
				bl_1 = false;
			}
			if (bl_2 && ban[pos2 += dir2] == EMP) {
				(mlist++)->move = cons_move(0, pos2, piece, EMP);
			} else {
				bl_2 = false;
			}
			if (bl_3 && ban[pos3 += dir3] == EMP) {
				(mlist++)->move = cons_move(0, pos3, piece, EMP);
			} else {
				bl_3 = false;
			}
			if (bl_4 && ban[pos4 += dir4] == EMP) {
				(mlist++)->move = cons_move(0, pos4, piece, EMP);
			} else {
				bl_4 = false;
			}
		}
	}
	// 飛車を打つ
	if (((us == BLACK) ? handS.getHI() : handG.getHI()) > 0) {
		static const int dir1 = -16;
		static const int dir2 =  -1;
		static const int dir3 =   1;
		static const int dir4 =  16;
		int pos1, pos2, pos3, pos4;
		pos1 = pos2 = pos3 = pos4 = enemyKing;
		bool bl_1 = true;
		bool bl_2 = true;
		bool bl_3 = true;
		bool bl_4 = true;
		piece = (us == BLACK) ? SHI : GHI;
		while (bl_1 || bl_2 || bl_3 || bl_4) {
			if (bl_1 && ban[pos1 += dir1] == EMP) {
				(mlist++)->move = cons_move(0, pos1, piece, EMP);
			} else {
				bl_1 = false;
			}
			if (bl_2 && ban[pos2 += dir2] == EMP) {
				(mlist++)->move = cons_move(0, pos2, piece, EMP);
			} else {
				bl_2 = false;
			}
			if (bl_3 && ban[pos3 += dir3] == EMP) {
				(mlist++)->move = cons_move(0, pos3, piece, EMP);
			} else {
				bl_3 = false;
			}
			if (bl_4 && ban[pos4 += dir4] == EMP) {
				(mlist++)->move = cons_move(0, pos4, piece, EMP);
			} else {
				bl_4 = false;
			}
		}
	}
	return mlist;
}

// 駒を打つ手で王手となるものを生成する(残り3手用).
template<Color us>
MoveStack* Position::gen_check_drop3(MoveStack* mlist, bool& bUchifudume) const
{
	int enemyKing = (us == BLACK) ? kingG : kingS;	// 手番側から見て、相手の玉の位置
	int suji, dan;

	const int ksuji = enemyKing & 0xF0;
	const int kdan  = enemyKing & 0x0F;

	//
	//  玉との位置関係
	//
	//  +28 +12 -4 -20 -36
	//  +29 +13 -3 -19 -35
	//  +30 +14 -2 -18 -34
	//  +31 +15 -1 -17 -33
	//  +32 +16 玉 -16 -32
	//  +33 +17 +1 -15 -31
	//  +34 +18 +2 -14 -30
	//  +35 +19 +3 -15 -29
	//  +36 +20 +4 -13 -28
	//
	//
	Piece piece;
	// 金を打つ
#define ADD_DROP(x)  if (ban[enemyKing + (x)] == EMP) (mlist++)->move = cons_move(0, enemyKing + (x), piece, EMP)
	if (((us == BLACK) ? handS.getKI() : handG.getKI()) > 0) {
		if (us == BLACK) {
			piece = SKI;
			ADD_DROP(  1);
			ADD_DROP( 17);
			ADD_DROP(-15);
			ADD_DROP( 16);
			ADD_DROP(-16);
			ADD_DROP( -1);
		} else {
			piece = GKI;
			ADD_DROP( -1);
			ADD_DROP(-17);
			ADD_DROP( 15);
			ADD_DROP(-16);
			ADD_DROP( 16);
			ADD_DROP(  1);
		}
	}
	// 銀を打つ
	if (((us == BLACK) ? handS.getGI() : handG.getGI()) > 0) {
		if (us == BLACK) {
			piece = SGI;
			ADD_DROP(  1);
			ADD_DROP(-17);
			ADD_DROP(-15);
			ADD_DROP( 15);
			ADD_DROP( 17);
		} else {
			piece = GGI;
			ADD_DROP( -1);
			ADD_DROP( 17);
			ADD_DROP( 15);
			ADD_DROP(-15);
			ADD_DROP(-17);
		}
	}
	if (us == BLACK) {
		//桂を打つ
		if (handS.getKE() > 0 && kdan <= 7) {
			piece = SKE;
			ADD_DROP(-14);
			ADD_DROP( 18);
		}
		// 香を打つ
		suji = ksuji;
		dan = kdan + 1;
		if (handS.getKY() > 0 && ban[dan+suji] == EMP) {
			int n = 0;
			piece = SKY;
			for (; dan <= 9 && n < 3; dan++, n++) {
				if (ban[dan+suji]==EMP) {
					(mlist++)->move = cons_move(0, suji+dan, piece, EMP);
				} else {
					break;
				}
			}
		}
		// 歩を打つ
		suji = ksuji;
		dan = kdan + 1;
		if (handS.getFU() > 0 && ban[dan+suji] == EMP) {
			// 二歩チェック
			int nifu = is_double_pawn(us, suji);
			if (nifu == 0) {
				// 打ち歩詰めもチェック
				dan = kdan + 1;
				if (!is_pawn_drop_mate(us, dan+suji)) {
					(mlist++)->move = cons_move(0, suji+dan, SFU, EMP);
				} else {
					bUchifudume = true;
				}
			}
		}
	} else {
		//桂を打つ
		if (handG.getKE() > 0 && kdan >= 3) {
			piece = GKE;
			ADD_DROP( 14);
			ADD_DROP(-18);
		}
		// 香を打つ
		suji = ksuji;
		dan = kdan - 1;
		if (handG.getKY() > 0 && ban[dan+suji] == EMP) {
			int n = 0;
			for (; dan >= 1 && n < 3; dan--, n++) {
				if (ban[dan+suji]==EMP) {
					(mlist++)->move = cons_move(0, suji+dan, GKY, EMP);
				} else {
					break;
				}
			}
		}
		// 歩を打つ
		suji = ksuji;
		dan = kdan - 1;
		if (handG.getFU() > 0 && ban[dan+suji] == EMP) {
			// 二歩チェック
			int nifu = is_double_pawn(us, suji);
			if (nifu == 0) {
				// 打ち歩詰めもチェック
				dan = kdan - 1;
				if (!is_pawn_drop_mate(us, dan+suji)) {
					(mlist++)->move = cons_move(0, suji+dan, GFU, EMP);
				} else {
					bUchifudume = true;
				}
			}
		}
	}

	// 角を打つ
	if (((us == BLACK) ? handS.getKA() : handG.getKA()) > 0) {
		static const int dir1 = -17;
		static const int dir2 = -15;
		static const int dir3 =  15;
		static const int dir4 =  17;
		int pos1, pos2, pos3, pos4;
		pos1 = pos2 = pos3 = pos4 = enemyKing;
		int n = 0;
		piece = (us == BLACK) ? SKA : GKA;
		while (n < 3) {
			if (ban[pos1 += dir1] == EMP) {
				(mlist++)->move = cons_move(0, pos1, piece, EMP);
				n++;
			} else {
				break;
			}
		}
		n = 0;
		while (n < 3) {
			if (ban[pos2 += dir2] == EMP) {
				(mlist++)->move = cons_move(0, pos2, piece, EMP);
				n++;
			} else {
				break;
			}
		}
		n = 0;
		while (n < 3) {
			if (ban[pos3 += dir3] == EMP) {
				(mlist++)->move = cons_move(0, pos3, piece, EMP);
				n++;
			} else {
				break;
			}
		}
		n = 0;
		while (n < 3) {
			if (ban[pos4 += dir4] == EMP) {
				(mlist++)->move = cons_move(0, pos4, piece, EMP);
				n++;
			} else {
				break;
			}
		}
	}
	// 飛車を打つ
	if (((us == BLACK) ? handS.getHI() : handG.getHI()) > 0) {
		static const int dir1 = -16;
		static const int dir2 =  -1;
		static const int dir3 =   1;
		static const int dir4 =  16;
		int pos1, pos2, pos3, pos4;
		pos1 = pos2 = pos3 = pos4 = enemyKing;
		int n;
		n = 0;
		piece = (us == BLACK) ? SHI : GHI;
		while (n < 3) {
			if (ban[pos1 += dir1] == EMP) {
				(mlist++)->move = cons_move(0, pos1, piece, EMP);
				n++;
			} else {
				break;
			}
		}
		n = 0;
		while (n < 3) {
			if (ban[pos2 += dir2] == EMP) {
				(mlist++)->move = cons_move(0, pos2, piece, EMP);
				n++;
			} else {
				break;
			}
		}
		n = 0;
		while (n < 3) {
			if (ban[pos3 += dir3] == EMP) {
				(mlist++)->move = cons_move(0, pos3, piece, EMP);
				n++;
			} else {
				break;
			}
		}
		n = 0;
		while (n < 3) {
			if (ban[pos4 += dir4] == EMP) {
				(mlist++)->move = cons_move(0, pos4, piece, EMP);
				n++;
			} else {
				break;
			}
		}
	}
#undef ADD_DROP
	return mlist;
}

template<Color us>
MoveStack* Position::generate_check(MoveStack* mlist, bool& bUchifudume) const
{
	int enemyKing = (us == BLACK) ? kingG : kingS;	// 手番側から見て、相手の玉の位置
	if (enemyKing == 0) return 0;

	// 手を生成.
	MoveStack* cur = mlist;
	MoveStack* last;

	// 自玉に王手がかかっているときは王手を回避＆＆王手に成るような手を列挙する.
	if ((us == BLACK && kingS != 0 && IsCheckS()) 
	   || (us != BLACK && kingG != 0 && IsCheckG())) {
		last = (us == BLACK) ? generate_evasion<BLACK>(mlist) :  generate_evasion<WHITE>(mlist);

		while (cur != last) {
			if (is_check_move(us, cur->move)) {
				(mlist++)->move = cur->move;
			}
			cur++;
		}

		return mlist;
	}

	// 盤上の駒を動かす手で王手となるものを生成する.
	last = gen_check_long<us>(mlist);
	last = gen_check_short<us>(last);

	// 重複をなくす(重複は盤上の駒を動かす手に限られる)
	if (last != mlist) {
		int j;
		int teNum = 1;
		MoveStack* top = mlist;
		for (cur = &top[1]; cur != last; cur++) {
			bool flag = false;
			for (j = 0; j < teNum; j++) {
				if (top[j].move == cur->move) {
					flag = true;
					break;
				}
			}
			if (flag == false) {
				top[teNum].move = cur->move;
				teNum++;
			}
		}
		mlist += teNum;
	}

	// 駒を打つ手で王手となるものを生成する.
	mlist = gen_check_drop<us>(mlist, bUchifudume);

	return mlist;
}

template<Color us>
MoveStack* Position::generate_check3(MoveStack* mlist, bool& bUchifudume) const
{
	int enemyKing = (us == BLACK) ? kingG : kingS;	// 手番側から見て、相手の玉の位置
	if (enemyKing == 0) return 0;

	// 手を生成.
	MoveStack* cur = mlist;
	MoveStack* last;

	// 自玉に王手がかかっているときは王手を回避＆＆王手に成るような手を列挙する.
	if ((us == BLACK && kingS != 0 && IsCheckS())
	  || (us != BLACK && kingG != 0 && IsCheckG())) {
		last = (us == BLACK) ? generate_evasion<BLACK>(mlist) :  generate_evasion<WHITE>(mlist);

		while (cur != last) {
			if (is_check_move(us, cur->move)) {
				(mlist++)->move = cur->move;;
			}
			cur++;
		}

		return mlist;
	}

	// 盤上の駒を動かす手で王手となるものを生成する.
	last = gen_check_long<us>(mlist);
	last = gen_check_short<us>(last);

	// 重複をなくす(重複は盤上の駒を動かす手に限られる)
	if (last != mlist) {
		int j;
		int teNum = 1;
		MoveStack* top = mlist;
		for (cur = &top[1]; cur != last; cur++) {
			bool flag = false;
			for (j = 0; j < teNum; j++) {
				if (top[j].move == cur->move) {
					flag = true;
					break;
				}
			}
			if (flag == false) {
				top[teNum].move = cur->move;
				teNum++;
			}
		}
		mlist += teNum;
	}

	// 駒を打つ手で王手となるものを生成する.
	mlist = gen_check_drop<us>(mlist, bUchifudume);

	return mlist;
}

// 王手を受ける手の生成(残り2手)
MoveStack *Position::generate_evasion_rest2(const Color us, MoveStack *mBuf, effect_t kiki, int &Ai)
{
	int kingPos;
	MoveStack *mlist = mBuf;

	kiki &= (EFFECT_LONG_MASK | EFFECT_SHORT_MASK);

	if (kiki == 0) {
		output_info("Error!:%s玉に王手がかかっていない！\n", (us == BLACK) ? "先手" : "後手");
		this->print_csa();
MYABORT();
		return mlist;
	}
	if ((kiki & (kiki-1)) != 0) {
		//両王手は玉を動かすしかない
		mlist = gen_move_king(us, mlist);
	} else {
		kingPos = (us == BLACK) ? kingS : kingG;
		unsigned long id;
		int check;
		if (kiki & EFFECT_SHORT_MASK) {
			// 跳びのない利きによる王手 → 回避手段：王手している駒を取る、玉を動かす
			_BitScanForward(&id, kiki);
			check = kingPos - NanohaTbl::Direction[id];
			//王手駒を取る
			mlist = gen_move_to(us, mlist, check);
			//玉を動かす
			mlist = gen_move_king(us, mlist);
		} else {
			// 跳び利きによる王手 → 回避手段：王手している駒を取る、玉を動かす、合駒
			_BitScanForward(&id, kiki);
			id -= EFFECT_LONG_SHIFT;
			check = SkipOverEMP(kingPos, -NanohaTbl::Direction[id]);
			//王手駒を取る
			mlist = gen_move_to(us, mlist, check);
			//玉を動かす
			mlist = gen_move_king(us, mlist);
			//合駒をする手を生成する
			if (kingPos - NanohaTbl::Direction[id] != check) {
				Ai = 1;
			}
		}
	}
	return mlist;
}

// 王手を受ける手の生成(残り2手;移動合い)
MoveStack *Position::generate_evasion_rest2_MoveAi(const Color us, MoveStack *mBuf, effect_t kiki)
{
	int kingPos;
	MoveStack *mlist = mBuf;

	kiki &= (EFFECT_LONG_MASK | EFFECT_SHORT_MASK);

	if (kiki == 0) {
		output_info("Error!:%s玉に王手がかかっていない！\n", (us == BLACK) ? "先手" : "後手");
		this->print_csa();
MYABORT();
		return mlist;
	}
	if ((kiki & (kiki-1)) != 0) {
		//両王手は玉を動かすしかない ⇒ 合いの生成なので、ここには来ないはず
MYABORT();
	} else {
		kingPos = (us == BLACK) ? kingS : kingG;
		unsigned long id;
		int check;
		if (kiki & EFFECT_SHORT_MASK) {
			// 跳びのない利きによる王手 ⇒ 合いの生成なので、ここには来ないはず
MYABORT();
		} else {
			// 跳び利きによる王手 → 合駒
			_BitScanForward(&id, kiki);
			id -= EFFECT_LONG_SHIFT;
			check = SkipOverEMP(kingPos, -NanohaTbl::Direction[id]);
			//合駒をする手を生成する
			if (kingPos - NanohaTbl::Direction[id] != check) {
				int i;
				for (i = kingPos - NanohaTbl::Direction[id]; ban[i] == EMP; i -= NanohaTbl::Direction[id]) {
					mlist = gen_move_to(us, mlist, i);	//移動合
//					mlist = gen_drop_to(us, mlist, i);	//駒を打つ合
				}
			}
		}
	}
	return mlist;
}

// 王手を受ける手の生成(残り2手;移動合い)
MoveStack *Position::generate_evasion_rest2_DropAi(const Color us, MoveStack *mBuf, effect_t kiki, int &check_pos)
{
	int kingPos;

	kiki &= (EFFECT_LONG_MASK | EFFECT_SHORT_MASK);
	MoveStack *mlist = mBuf;

	if (kiki == 0) {
		output_info("Error!:%s玉に王手がかかっていない！\n", us == BLACK ? "先手" : "後手");
		this->print_csa();
MYABORT();
		return mlist;
	}

	if ((kiki & (kiki-1)) != 0) {
		//両王手は玉を動かすしかない ⇒ 合いの生成なので、ここには来ないはず
MYABORT();
	} else {
		kingPos = (us == BLACK) ? kingS : kingG;
		unsigned long id;
		int check;
		if (kiki & EFFECT_SHORT_MASK) {
			// 跳びのない利きによる王手 ⇒ 合いの生成なので、ここには来ないはず
MYABORT();
		} else {
			// 跳び利きによる王手 → 合駒
			_BitScanForward(&id, kiki);
			id -= EFFECT_LONG_SHIFT;
			check = SkipOverEMP(kingPos, -NanohaTbl::Direction[id]);
			//合駒をする手を生成する
			check_pos = check;
			if (kingPos - NanohaTbl::Direction[id] != check) {
				int i;
				for (i = kingPos - NanohaTbl::Direction[id]; ban[i] == EMP; i -= NanohaTbl::Direction[id]) {
//					MoveTo(SorG, mNum, mBuf, i);	//移動合
//					int teNum2 = mNum;
//					DropTo(SorG, teNum2, mBuf, i);	//駒を打つ合
//					if (teNum2 > mNum) mNum++;
					mlist = gen_drop_to(us, mlist, i);	//駒を打つ合
				}
			}
		}
	}
	return mlist;
}

//
// 詰むかどうかを調べる。
// 引数：Color us				手番(BLACK：先手、WHITE：後手)
//		 int maxDepth			探索する最大深さ
//		 Move &m					詰ます手を返す
//		 unsigned long limit	制限時間(ms)
// 戻り値：int					詰むかどうか(VALUE_MATE:詰む、-VALUE_MATE：詰まない、VALUE_ZERO：不明)
//
int Position::Mate3(const Color us, Move &m)
{
#if defined(USE_M3HASH)
	M3Perform::called++;
#endif
	assert(us == side_to_move());
	// 1手詰めを確認
	{
		uint32_t refInfo;
		int val = (us == BLACK) ? Mate1ply<BLACK>(m, refInfo) :  Mate1ply<WHITE>(m, refInfo);
		if (val == VALUE_MATE) return val;
	}

#if defined(USE_M3HASH)
	if (probe_m3hash(*this, m)) {
		M3Perform::hashhit++;
		return m == MOVE_NONE ? -VALUE_MATE : VALUE_MATE;
	}
#endif

	MoveStack moves[256]; 	// 深さ3程度なら十分な大きさ
	MoveStack *cur, *last;
	bool bUchifudume = false;

	last = (us == BLACK) ? generate_check3<BLACK>(moves, bUchifudume)
	                     : generate_check3<WHITE>(moves, bUchifudume);
	if (last == moves) {
		return -VALUE_MATE; 	//詰まない
	}

	int valmax = -VALUE_MATE;
	for (cur = moves; cur != last; cur++) {
		StateInfo newSt;
		Move move = cur->move;
		// 残り３手では打ち歩詰めを回避する必要はないため、不成は読まない
		if ((move & MOVE_CHECK_NARAZU)) continue;
		do_move(move, newSt);
		int val;
#if defined(DEBUG_MATE2)
		pre_check = move;
#endif
		val = EvasionRest2(flip(us), last);
		undo_move(move);

		if (val > valmax) valmax = val;
		if (valmax == VALUE_MATE) {
			m = move;
#if defined(USE_M3HASH)
			store_m3hash(*this, move);
#endif
			return VALUE_MATE; //詰んだ
		}
	}

#if defined(USE_M3HASH)
	store_m3hash(*this, MOVE_NONE);
#endif
	return valmax;
}

//
// 玉方末端(残り2手)で詰むかどうかを調べる。
// 引数：Color us				手番(BLACK：先手、WHITE：後手)
//		 Move &m					詰ます手を返す
// 戻り値：int					詰むかどうか(VALUE_MATE:詰む、VAL_FUDUMI：詰まない、VAL_FUMEI：不明)
//
//int Position::EvasionRest2(const Color us, MoveStack *antichecks, unsigned int &PP, unsigned int &DP, int &pn, int &dn, bool &isUchifudume)
int Position::EvasionRest2(const Color us, MoveStack *antichecks)
{
	if (!in_check()) {
		output_info("Error!:%s玉に王手がかかっていない！\n", us == BLACK ? "先手" : "後手");
#if defined(DEBUG_MATE2)
		this->print_csa(pre_check);
		undo_move(pre_check);
		this->print_csa();

		MYABORT();
#endif
		return -VALUE_MATE;
	}

	MoveStack *cur, *last;
	// 打つ合駒以外の王手回避手生成
	int Ai = 0;
	last = (us == BLACK) ? generate_evasion_rest2(BLACK, antichecks, exist_effect<WHITE>(kingS), Ai)
	                     : generate_evasion_rest2(WHITE, antichecks, exist_effect<BLACK>(kingG), Ai);

	if (last == antichecks && Ai == 0) {
		// 受けがない ⇒ 詰み
		return VALUE_MATE;
	}

	int valmin = VALUE_MATE;
	StateInfo newSt;
	uint32_t refInfo;
	Move m;
	for (cur = antichecks; cur != last; cur++) {
		Move move = cur->move;
		do_move(move, newSt);
		int val = (us == BLACK) ? Mate1ply<WHITE>(m, refInfo) :  Mate1ply<BLACK>(m, refInfo);
		undo_move(move);

		if (val < valmin) {
			valmin = val;
		}
		// 詰まなかった
		if (valmin != VALUE_MATE) {
			return valmin;
		}
	}
	if (Ai == 0) return valmin;

	// 移動合いの手で詰むか確認する
	last = (us == BLACK) ? generate_evasion_rest2_MoveAi(BLACK, antichecks, exist_effect<WHITE>(kingS))
	                     : generate_evasion_rest2_MoveAi(WHITE, antichecks, exist_effect<BLACK>(kingG));

	for (cur = antichecks; cur != last; cur++) {
		Move move = cur->move;
		do_move(move, newSt);
		int val = (us == BLACK) ? Mate1ply<WHITE>(m, refInfo) : Mate1ply<BLACK>(m, refInfo);
		undo_move(move);
		if (val < valmin) {
			valmin = val;
		}

		// 詰まなかった
		if (valmin != VALUE_MATE) {
			return valmin;
		}
	}

	// 打つ合駒で詰むか確認する
	int check = 0;	// 王手をかけている駒の位置
	last = (us == BLACK) ? generate_evasion_rest2_DropAi(BLACK, antichecks, exist_effect<WHITE>(kingS), check)
	                     : generate_evasion_rest2_DropAi(WHITE, antichecks, exist_effect<BLACK>(kingG), check);

	for (cur = antichecks; cur != last; cur++) {
		Move move = cur->move;

		// 末端の詰み処理
		do_move(move, newSt);
		int val = (us == BLACK) ? Mate1ply<WHITE>(m, refInfo) : Mate1ply<BLACK>(m, refInfo);
		undo_move(move);

		// 詰まなかった
		if (val != VALUE_MATE) {
			return val;
		}
	}
	// どうやっても詰んでしまう
	return VALUE_MATE;
}

// ベンチマークの時のみ呼ばれる.
void analize_mate3()
{
#if defined(USE_M3HASH)
	std::cerr << "\n==============================="
	          << "\n Mate3() called  : " << M3Perform::called;
	if (M3Perform::called > 0) {
		std::cerr << "\n hash hit(count) : " << M3Perform::hashhit
		          << "\n hash hit(%)     : " << (double)M3Perform::hashhit  / M3Perform::called * 100.0
		          << "\n override(count) : " << M3Perform::override
		          << "\n override(%)     : " << (double)M3Perform::override / M3Perform::called * 100.0;
		          
	}
	int count = 0;
	int mate = 0;
	for (int i = 0; i < MATE3_MASK+1; i++) {
		if (mate3_hash_tbl[i].word1 != 0 || mate3_hash_tbl[i].word2 != 0) {
			count++;
			if (static_cast<Move>(mate3_hash_tbl[i].word2 >> 32) != MOVE_NONE) mate++;
		}
	}
	if (count > 0) {
		std::cerr << "\n used(%)         : " << (double)count * 100.0  / (MATE3_MASK+1)
		          << "\n      mate(%)    : " << (double)mate * 100.0 / count
		          << std::endl;
	}
#endif
}

template MoveStack* Position::generate_check<BLACK>(MoveStack* mlist, bool& bUchifudume) const;
template MoveStack* Position::generate_check<WHITE>(MoveStack* mlist, bool& bUchifudume) const;
