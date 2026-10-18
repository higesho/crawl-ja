# C++ ソース日本語化の規約(ja-0.34)

0.34 のソース中の英文を `jtrans()` 系で包み、辞書
`crawl-ref/source/dat/database/ja/jtrans/*.txt` に訳を書く。
旧日本語版 0.19-ja の同じ作業を参考にする(`jtools.py old <file>`)。

## 1 ファイルの作業手順

1. `python -I ja-tools/jtools.py old <file.cc>` で 0.19-ja の差分を見る
   (0.34 で消えた・書き換わったコードも多いので、そのまま当てない)。
2. `python -I ja-tools/jtools.py wrap <file.cc>` で `mpr("...")` などの
   定型パターンを機械的に包む(`#include "database.h"` も足される)。
   そのあと 0.34 のファイルを読み、残りのプレイヤーに見える英文を手で包む
   (下記)。機械的に包んだ箇所も、語順の組み替えが必要なものは直す。
   必要なら `#include "japanese.h"` を足す。
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

- **モンスター名・アイテム名**は名前を作る関数(`monster::name()`,
  `monster_info` の名前関数, `item_def::name()`)自体が日本語を返すように
  する(担当: mon-info.cc / mon-util.cc / monster.cc / item-name.cc)。
  呼び出し側では `jtrans(mon->name(...))` のように二重に包まない。
  `DESC_DBNAME` は説明文の検索キーなので英語のまま。
- **呪文・技能・種族・職業・神**の名前関数(`spell_title()`, `skill_name()`,
  `species::name()`, `get_job_name()`, `god_name()`)は検索キーなどにも
  使われるので英語のまま残し、**表示する箇所で** `japanese.h` の
  `spell_title_j()`, `skill_name_j()`, `species_name_j()`, `job_name_j()`,
  `god_name_j()`(`_jc` は `c_str()` 版)に置き換える。
- その他の名前(地形・雲・状態・能力・変異など)は、データの英文を
  表示する箇所で `jtrans(name)` / `tagged_jtrans("[タグ]", name)` で包む。
  データ表(`*-data.h`)の訳は辞書 `jtrans/data-*.txt` にまとめて作る。
  旧辞書のタグに合わせる(`[feature]`, `[dur]`, `[ability]`, `[cloud]`,
  `[form]`, `[card]`, `[branch]`, `[zap]` など。`jtools.py find` で確認)。
- `simple_monster_message(mon, jtransc(" is healed!"))` のように、名前の後ろに
  つなぐ文は訳を助詞から始める(「は傷が癒えた！」)。所有格
  (`need_possessive`)や `apostrophise()` は日本語の名前なら自動で「〜の」に
  なるので、訳は「の」の後ろから書く(「魔力が高まった！」)。
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

## データ辞書のキー形式(全員共通)

名前・データ表の訳は `jtrans/data-<種類>.txt` に置く。キーは旧辞書に合わせる。

| 内容 | 出典 | キーの形 | 辞書ファイル |
|---|---|---|---|
| モンスター名 | mon-data.h | `goblin`(タグなし) | data-monsters.txt |
| アイテム名 | item-name.cc 等 | `potion of curing`, `hand axe`(タグなし) | data-items.txt |
| 助数詞 | | `[counter]hand axe`, `[counter class]Weapons` | data-items.txt |
| 固定アーティファクト名 | art-data.h | `Singing Sword`(タグなし) | data-unrands.txt |
| 呪文名 | spl-data.h | `[spell]Magic Dart` | data-spells.txt |
| 技能名 | skills.cc | `[skill]Fighting` | data-skills.txt |
| 称号 | skills.cc, religion.cc | `[title]Fighter` | data-skills.txt |
| 種族名 | species-data.h | `[spec name]Human`, `[spec adj]Human` | data-species.txt |
| 職業名 | job-data.h | `[job]Fighter` | data-jobs.txt |
| 神の名前 | religion.cc | `Trog`, `Trog the Wrathful`(タグなし) | data-gods.txt |
| 支部名 | branch-data.h | `[branch]Dungeon` | data-branches.txt |
| 地形名 | feature-data.h | `[feature]closed door` | data-features.txt |
| ビーム名 | zap-data.h | `[zap]puff of flame` | data-zaps.txt |
| 状態表示 | duration-data.h | 表示文字列そのもの(`[dur]` は短縮表示) | data-durations.txt |
| 変身 | form-data.h | `[form]...` | data-forms.txt |
| 変異 | mutation-data.h | 文章そのもの(タグなし) | data-mutations.txt |

## 並行作業のルール

- 割り当てられたファイル(と、その辞書ファイル `jtrans/<ファイル名>.txt`)
  以外は編集しない。他のファイルの変更が必要なら報告に書く。
- ビルド(MSBuild)はしない。取りまとめ役がまとめてビルドする。
  コンパイルが通るよう、型(`string` と `const char*`)や include に注意する。
- git のコミット・ブランチ操作はしない。
