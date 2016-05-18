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


#if !defined(BOOK_H_INCLUDED)
#define BOOK_H_INCLUDED

#include <map>
#include <cstring>	// for memcmp()

#include "move.h"
#include "misc.h"

class Position;
struct MoveStack;

// 定跡データのキー
struct BookKey {
	unsigned char data[32];

	bool operator < (const BookKey &b) const {
		return memcmp(data, b.data, sizeof(BookKey)) < 0;
	}
};

// 定跡データの中身
struct BookEntry {
	short eval;				// 評価値
	unsigned short hindo;	// 頻度(出現数)
	unsigned short swin;	// 先手勝ち数
	unsigned short gwin;	// 後手勝ち数
	unsigned short dummy[3];	// 予約
	unsigned char move[9][2];
};

// 定跡のクラス
class Book {
private:
	typedef std::map<BookKey, BookEntry> Joseki_type;
	typedef Joseki_type::value_type Joseki_value;
	Joseki_type joseki;
	PRNG prng;
	Book(const Book&);				// warning対策.
	Book& operator = (const Book&);	// warning対策.
public:
	Book();
	~Book();
	// 初期化
	void open(const std::string& fileName);
	void close();
	// 定跡データから、現在の局面kの合法手がその局面でどれくらいの頻度で指されたかを返す。
	void fromJoseki(Position &pos, int &mNum, MoveStack moves[], BookEntry data[]);
	// 現在の局面がどのくらいの頻度で指されたか定跡データを調べる
	int getHindo(const Position &pos);

	// 
	size_t size() const {return joseki.size();}

	Move get_move(Position& pos, bool findBestMove);
};

#ifdef USAPYON2
extern Book *book[2];
#else
extern Book *book;
#endif

#endif // !defined(BOOK_H_INCLUDED)
