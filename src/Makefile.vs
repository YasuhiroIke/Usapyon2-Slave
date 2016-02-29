#
# なのはminiにするときはNANOHAMINI=1の前の#を取り、
# なのはnanoにするときはNANOHAMINI=1の前に#を付け、NANOHANANO=1の前の#を取る
# nanoperyにするときはNANOPERY=1の前の#を取る
#
#NANOHAMINI=1
#NANOHANANO=1
NANOPERY=1

!IFDEF NANOHAMINI
EVAL_TYPE=EVAL_MINI
EVAL_OBJ=evaluate.obj
EXE = nanohamini.exe
PGD = nanohamini.pgd
PGOLOG = nanohamini_prof.txt
!ELSEIFDEF NANOHANANO
EVAL_TYPE=EVAL_NANO
EVAL_OBJ=evaluate.obj
EXE = nanohanano.exe
PGD = nanohanano.pgd
PGOLOG = nanohanano_prof.txt
!ELSEIFDEF NANOPERY
EVAL_TYPE=EVAL_APERY
EVAL_OBJ=evaluate_apery.obj
EXE = usapyon2.exe
PGD = usapyon2.pgd
PGOLOG = usapyon2_prof.txt
!ELSE
!ERROR undefined eval_type
!ENDIF

OBJS = mate1ply.obj misc.obj timeman.obj $(EVAL_OBJ) position.obj \
	 tt.obj main.obj move.obj \
	 movegen.obj search.obj uci.obj movepick.obj thread.obj ucioption.obj \
	 benchmark.obj book.obj \
	 shogi.obj mate.obj problem.obj

CC=cl
LD=link



# Compile Options
#
# -DEVAL_MINI      なのはmini(2駒関係(KP+PP)の評価関数)
# -DEVAL_NANO      なのはnano(2駒関係(KPのみ)の評価関数)
# -DEVAL_APERY     nanopery(Aperyの評価関数)
#
# Visual C++オプション
#
# /D_CRT_SECURE_NO_WARNINGS
#                   secureな関数を使っていないときの警告を出さない
# /Zc:forScope      スコープ ループに標準 C++ を適用する
# /Wall             警告をすべて有効にする
# /GS[-]            セキュリティ チェックを有効にする
# /favor:<blend|AMD64|EM64T> 最適化するプロセッサ
# /GL[-]            リンク時のコード生成を行う
# /RTCs             スタック フレーム ランタイム チェック
# /RTCu             初期化されていないローカル変数のチェック

FLAGS = -DNDEBUG -D$(EVAL_TYPE) -DUSAPYON2 -DNANOHA -DCHK_PERFORM -DTWIG \
	-DOLD_LOCKS /favor:AMD64 /EHsc /D_CRT_SECURE_NO_WARNINGS \
	 /GL /Zc:forScope
#CXXFLAGS=$(FLAGS) /MT /W4 /Wall /nologo /Od /GS /RTCsu
CXXFLAGS=$(FLAGS) /MD /W3 /nologo /Ox /Ob2 /GS- /Gm /Zi
LDFLAGS=/NOLOGO /STACK:16777216,32768 /out:$(EXE) /LTCG /DEBUG
PGOLDFLAGS1=/NOLOGO /STACK:16777216,32768 /out:$(EXE) /LTCG:PGI
PGOLDFLAGS2=/NOLOGO /STACK:16777216,32768 /out:$(EXE) /LTCG:PGO


all: $(EXE)

$(EXE) : $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) User32.lib

.cpp.obj :
	$(CC) $(CXXFLAGS) /c $*.cpp

clean :
	del /q *.obj
	del /q *.idb
	del /q *.pdb
	del /q *.pgc
	del /q *.pgd
	del /q *.suo
	del    $(PGOLOG)
	del    $(EXE)

pgo: $(OBJS)
	$(LD) $(PGOLDFLAGS1) $(OBJS) User32.lib
	$(EXE) bench 256 4 10
	pgomgr /merge $(PGD)
	pgomgr /summary $(PGD) > $(PGOLOG)
	$(LD) $(PGOLDFLAGS2) $(OBJS) User32.lib

prof-clean:
	del /q *.pgc
	del /q *.pgd
	del    $(PGOLOG)
	del    $(EXE)
