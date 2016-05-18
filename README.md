# Usapyon2-Slave

「うさぴょん２」ではクラスタ並列化構成を予定しているのですが、その内のスレーブ部分になります。

<strike>なお、マスター部分は2016/03/21現在、「影」位しかありません。</strike>


マスター部（及びSlaveとつなぐ部分）は結局5月5日の選手権当日（3回戦目から実戦投入）に一応出来たのですが、最後の対局で「読み太」にTimeUpで負けたので、まだバグが残っているようです。公開は当面控えておきます。＜実験を相当重ねないといけないので、永遠に公開できないかも…。

スレーブ側なので、定跡も要らないし時間制御も要らないし…などと考えていたのですが、実際の強さを知るには、floodgate上で
ベンチマークを取らないとなー…と思い直し、USIエンジンとしてそれなりにちゃんと実装したものとなっております。

元にしているソースコードは、「なのはmini 0.2.2.1」、「StockFish 7」になっております。
また、評価バイナリは、Aperyの「大樹の枝バージョン」を使用しております。

usapyon2.exeを実行する際には、Aperyの「大樹の枝バージョン」のxxx_synthesized.binを同じディレクトリに置いておく必要があります。

また、定跡を使用する場合には、book_40.jsk/book_40_2.jskというファイルも必要です。

<strike>binディレクトリにbin.zipを置きましたので、そちらを解凍してusapyon2.exeと同じディレクトリに置けばＯＫです。</strike>

定跡ファイルも以下の場所からダウンロード可能にしました。

上記、転送容量の制限にひっかかってしまったようですので、<a href="http://usapyon.game.coocan.jp/usapyon2/index.html">うさぴょん２</a>のページからダウンロードして下さい。

「なのはmini」の作者、川端一之様、「Apery」の作者、平岡拓也様、
「StockFish7」の作者、Tord Romstad様、Marco Costalba様, Joona Kiiski様,Gary Linscott様に深く感謝をしております。


また、２ｃｈの「コンピュータ将棋スレッド」での「名無し名人」様方にも深く感謝をしております。


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
