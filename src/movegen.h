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



#if !defined(MOVEGEN_H_INCLUDED)
#define MOVEGEN_H_INCLUDED

#include "move.h"

enum MoveType {
	MV_CAPTURE,             // ãÓÇéÊÇÈéË
	MV_NON_CAPTURE,         // ãÓÇéÊÇÁÇ»Ç¢éË
	MV_CHECK,               // â§éË
	MV_NON_CAPTURE_CHECK,   // ãÓÇéÊÇÁÇ»Ç¢â§éË
	MV_EVASION,             // â§éËâÒîéË
	MV_NON_EVASION,         // â§éËÇ™Ç©Ç©Ç¡ÇƒÇ¢Ç»Ç¢éûÇÃçáñ@éËê∂ê¨
	MV_LEGAL                // çáñ@éËê∂ê¨
};

template<MoveType>
MoveStack* generate(const Position& pos, MoveStack* mlist);

/// The MoveList struct is a simple wrapper around generate(), sometimes comes
/// handy to use this class instead of the low level generate() function.
template<MoveType T>
struct MoveList {

	explicit MoveList(const Position& pos) : cur(mlist), last(generate<T>(pos, mlist)) {}
	MoveStack begin() { cur = mlist; return cur; }
	void operator++() { cur++; }
	bool end() const { return cur == last; }
	Move move() const { return cur->move; }
	int size() const { return int(last - mlist); }
	bool contains(Move move) const {
		int i = 0;
		for (i = 0; &mlist[i] != last; i++) {
			if ( (((uint32_t)move) & 0x7FFF)  == (((uint32_t)mlist[i].move) & 0x7FFF) ) {
				return true;
			}
		}
		return false;
	}
#if 0
	template<GenType T>
	struct MoveList {
		explicit MoveList(const Position& pos) : last(generate<T>(pos, moveList)) {}
		const ExtMove* begin() const { return moveList; }
		const ExtMove* end() const { return last; }
		size_t size() const { return last - moveList; }
		bool contains(Move move) const {
			for (const auto& m : *this) if (m == move) return true;
			return false;
		}
#endif

private:
	MoveStack mlist[MAX_MOVES];
	MoveStack *cur, *last;
};

#endif // !defined(MOVEGEN_H_INCLUDED)
