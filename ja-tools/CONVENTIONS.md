# C++ ソース日本語化の規約(ja-0.34)

0.34 のソース中の英文を `jtrans()` 系で包み、辞書
`crawl-ref/source/dat/database/ja/jtrans/*.txt` に訳を書く。
旧日本語版 0.19-ja の同じ作業を参考にする(`jtools.py old <file>`)。

## 1 ファイルの作業手順

1. `python -I ja-tools/jtools.py old <file.cc>` で 0.19-ja の差分を見る
   (0.34 で消えた・書き換わったコードも多いので、そのまま当てない)。
2. 0.34 のファイルを読み、プレイヤーに見える英文を包む(下記)。
   必要なら `#include "database.h"`(jtrans 系)と `#include "japanese.h"` を足す。
3. `python -I ja-tools/jtools.py harvest <file.cc>` で旧辞書に同じ英文がある
   ものを `jtrans/<ファイル名>.txt` に自動で取り込む。
4. 残った未訳キーを訳して同じ辞書ファイルに書く。
5. `python -I ja-tools/jtools.py check <file.cc>` で未訳 0 件を確認。

## 包むもの・包まないもの

包む(プレイヤーに見えるもの):
- メッセージ: `mpr`, `mprf`, `mprf_nocap`, `simple_monster_message`,
  `simple_god_message`, `god_speaks`, `canned_msg` 相当の文字列など
- 確認・入力プロンプト(`yesno`, `yes_or_no`, `prompt_for_*` 等)
- メニュー・画面の見出し、項目、ヘルプ文、ステータス表示
- 説明文の組み立て(describe 系)、ノート・死因・ダンプの文

包まない:
- DB の検索キー(`getLongDescription(...)` などに渡す名前)、`DESC_DBNAME`
- セーブに残る識別子、props のキー、オプション名と値、Lua、正規表現
- `==` などで比較される文字列、JSON のキー、ファイル名
- `ASSERT`/`die`/デバッグ専用の出力(`dprf`, `#ifdef DEBUG`)
- ウィザードモード専用(wiz-*.cc)は優先度低。後回しでよい

迷ったら包まずに、ファイル先頭付近のコメントか作業報告に書いておく。

## API

```cpp
mpr(jtrans("You feel better."));
mprf(jtransc("%s hits you!"), mon->name(DESC_THE).c_str());
string s = jtrans("...");           // string が欲しいとき
jtrans_notrim(" (melded)")          // 前後の空白・改行を残したいとき
tagged_jtrans("[verb]", "hits")     // 短い語で文脈を分けたいとき
verb_j("hits"), adj_j(...), adv_j(...)   // [verb] 等のタグ付き省略形
```

- `jtransc(x)` は一時オブジェクトの `c_str()`。その文の中でだけ使う。
- 書式文字列は **%s 等の並びと個数を英文と同じに保つ**。日本語の語順で
  引数の順番を入れ替える必要があるときは、引数を渡す側を組み替える
  (位置指定 `%1$s` は MSVC で使えない)。
- 英語の文法処理(冠詞・複数形・三単現)は日本語では不要。
  `japanese.h` の `apply_description_j` などを使うか、分岐を削る。
- `language = ja` 以外では `jtrans()` は英文をそのまま返す。ただし英語表示を
  保つための分岐は不要(このフォークは日本語専用。英語表示が崩れてもよい)。

## 名前

- モンスター名・アイテム名・呪文名・神の名前などは、名前を作る中核
  (item-name.cc, mon-info.cc, mon-util.cc, spl-util.cc, religion.cc など)で
  日本語にする。呼び出し側で `jtrans(mon->name(...))` のように二重に包まない。
- 名前のあとに助詞を付ける: `"%sは%sを攻撃した。"`
- 代名詞(he/she/it/its)は日本語では省くか、名前で言い換える。

## 文体(0.19-ja に合わせる)

- 地の文は常体・過去形: 「あなたは気分がよくなった。」
- 主語の「あなた」は旧訳どおり明示してよい。
- 句読点は全角「、。！？」。数字・記号・キー表示 `(y/n)` は半角のまま。
- 鉤括弧: 台詞『』は旧訳に合わせる(`jtools.py find` で確認)。
- 用語は旧辞書に合わせる。`python -I ja-tools/jtools.py find <英語>` で探す。
  `ja-work/old-jtrans/` を直接 grep してもよい。

## 表示幅

日本語は 1 文字が半角 2 文字分の幅で、UTF-8 で 3 バイト。
`printf` の `%-20s` などはバイト数で揃えるので崩れる。幅を揃える必要が
ある箇所は `chop_string`、`align_centre`、`align_right`(unicode.h)や
`strwidth` を使う。

## 辞書ファイル

`crawl-ref/source/dat/database/ja/jtrans/<ソースの拡張子なし名>.txt`

```
%%%%
You feel better.

あなたは気分がよくなった。
%%%%
%s hits you!

%sはあなたを攻撃した！
%%%%
```

- キーは英文そのまま(C のエスケープを解いた形)。改行は `\n` と書く。
  前後の空白は無視され、大文字小文字は区別されない。
- 訳は 1 行が基本。値に `__NONE` と書くと空文字列になる。
- `#` で始まる行はコメント。
- 同じキーを複数のファイルに書かない(先に読まれたものが使われる)。
  別の意味で訳し分けたいときはタグ付きキーにする。
