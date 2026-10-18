# ファイル別の申し送り

先に作業した担当が見つけた、他のファイルで対応が必要な点。
そのファイルを担当するときに読み、対応したら項目を消す。

## 日本語を返すようになった関数(呼び出し側で二重に包まない・英語前提で加工しない)
- 名前: `monster::name()` 系、`item_def::name()`、`player::name()`(「あなた」)、
  `hand_name()`/`foot_name()`/`arm_name()`(英語版は `*_name_en`)
- 理由・説明の文: `evasion_margin_adverb()`, `get_player_aux_names()`, `describe_contamination()`,
  `ignore_faith_reason()`, `god_spell_warn_string()`, `god_title()`, `ru_sac_text()`,
  `yred_cannot_light_torch_reason()`, `dithmenos_cannot_*_reason()`, `zin_recite_text(-1)`,
  `can_go_berserk`/`invis_allowed`/`flight_allowed`/`no_tele_reason` の理由、
  `resist_margin_phrase()`(助詞から始まる)、`held_status()`(連体形)、
  `multimonster_name_string()`, `describe_monsters_condensed()`, `skill_names()`,
  `player_title()`(冠詞なし)、`duration_end_message()`/`duration_expire_message()`,
  `fill_status_info()`(英語版 `fill_status_info_en()`)、transform の
  `get_description`/`transform_message`/`get_untransform_message`/`melding_description`/
  `get_long_name`/`get_uc_attack_name`、`unarmed_attack_name()`, `mutation_desc()`,
  `bane_name()`(dbkey なし)、`bane_desc()`, `terse_mutation_list()`,
  `mut_upgrade_summary()`(英語版 `mut_upgrade_summary_en()`)、`cant_transform_reason()`,
  `no_selectables_message()`, `spell_uselessness_reason()` / `casting_uselessness_reason()` /
  `desc_cannot_memorise_reason()`(完結した文)、`spell_schools_string()`, `shop_name()`,
  デッキの説明系、`desc_wl_success_chance()` などのターゲット説明、`describe_item_rarity()`、
  `damage_rating()`, `describe_hit_chance()`/`describe_to_hit()`, `get_cloud_desc()` の見出し、
  `prep_branch_level_name()`(前置詞なし「迷宮の3階」)、`get_monster_status_descriptors()`、
  `get_monster_equipment_desc()`(DESC_NONE は述語形)、`i_feel_safe()` の理由
  (「近くに〜がいる」の句。句読点なし)
- 表示用に新設した日本語版(元の関数は検索キー・保存・比較のため英語のまま):
  `ability_name_j()`, `feature_description_at_j()`/`feature_description_j()`/
  `raw_feature_description_j()`(directn.h)、`get_monster_status_descriptor_pairs()`、
  `level_id::describe_j()`, `spelltype_long_name_j()`/`spelltype_short_name_j()`(spl-util.h)、
  `get_ghost_description_j()`(describe.h)、`rand_store_names()`(shopping.h)、
  hiscores の `*_j()`(`hiscore_line_j`, `death_description_j` など)と `death_name_j()`
  (英語で保存された名前を表示用に訳す。hiscores.h)
- Lua: 比較用の値は英語(`name`, `status`, `item.name()`, `you.caught()` など)。表示用に
  `crawl.jtrans(key, ...)`, `crawl.tagged_jtrans(tag, key)`, `crawl.jtrans_has_key(key)`,
  `item.name_j()`, `m:name_j()`, `mons.name_j` / `full_name_j` / `mname_j` / `mfull_name_j`,
  `dgn.feature_desc_j` / `feature_desc_at_j` がある。
- 二重の包み(害はないが不要): movement.cc / status.cc の `jtransc(held_status())`、
  player-equip.cc / evoke.cc の `jtransc(you.hand_name(…))`、ability.cc `_desc_sac_mut` の
  外側の `jtrans`、item-use.cc:433 の `jtransc(no_selectables_message(...))`

## 決めたこと
- 訳語の正はデータ辞書(jtrans/data-*.txt)。揺れの監査結果は ja-work/term-audit.md。
  腐食(腐蝕は使わない)、脱出ハッチ、コウモリ(蝙蝠は使わない)に統一した。
  分岐名は「迷宮」、地の文の the Dungeon は「ダンジョン」。
- 罠の名前 `trap_name()` / `full_trap_name()` は比較・Lua 用に英語のまま。表示は
  `tagged_jtrans("[feature]", full_trap_name(t))`。
- ギズモの名前は日本語(「・」区切り)でセーブにも残る。
- スコアファイル・ゴーストファイルに保存される称号は日本語のままでよい(このフォークは
  日本語専用のため)。`god_title_en()` などは作らない。
- 毒・麻痺などの原因(DISABLED_BY_KEY, POISONER_KEY)と死因に残る名前は英語で保存し、
  表示側(hiscores の `death_name_j()`)で訳す。プレイヤー自身は "you"。

## des(dat/des)と dlua(dat/dlua)
- 方式: des は英語のまま dlua に渡し、dlua が受け取る側で `crawl.jtrans` する。訳は
  jtrans/des.txt(des の文と `[feature]` の地形名)と jtrans/dlua.txt(timed_msg の verb /
  noisemaker / ranges、timed_marker の entity、trove の toll_desc をタグ付きで)。
  des に verb・noisemaker・ranges・entity の新しい値を足すときは dlua.txt にも足す。
- des の MONS の `name:`(bag_of_meat など独自のモンスター名)と、KFEAT の店名
  (name:Gozag type:Platinum suffix:Reserve など。shopping.cc 側)は未対応。
- dat 以下の .lua / .des に日本語(非 ASCII)を書いてよい。BOM なしのファイルは
  unicode.cc の FileLineInput が UTF-8 として正しければそのまま読む(第 6 陣で修正。
  それ以前は日本語版 Windows で CP932 として読まれ、Lua の構文エラーになった)。
- Lua の構文確認は `crawl.exe -builddb -rc <rc>`(DB と迷宮定義を作って終了。エラーは標準出力)。

## DB テキスト(dat/database/ja, dat/descript/ja)
第 7 陣で全訳済み(`python -I ja-tools/dbtools.py status` で未訳は quotes.txt の 182 件だけ)。
- quotes.txt の 182 件は著作権の関係で英語のまま(一覧と理由は ja-work/quotes-untranslated.txt)。
- 台詞の注意: mon-speak.cc の `_invalid_msg` は、プレイヤーが相手でないときに「あなた」を含む
  ja 台詞を弾く。mon-cast.cc は台詞の「身振り」「指さ」「指差」で身振りかを判定する。
  `@to_foe@` / `@at_foe@` の直後は「に」「を」「に向かって」のどれか(mon-util.cc の消去規則)。
  内部キー(`_recite_what_` など)の形は ja/monspeak.txt の訳に合わせる。
- jtrans/shout.txt のテンプレート訳は ja/shout.txt ができたので使われなくなった(消してよい)。
- mon-info.cc の `_mname_j`: 全角を含む名前は『』を付けずに返す(「オークの司祭ボグダン」)。
- monname.txt を訳したので、使徒・祖霊の名前とコグリンの武器の銘は日本語でセーブ・
  morgue・スコアに残る(日本語専用のフォークなのでこれでよい)。
- spl-miscast.cc の "'s body" 除去は英文向けのまま(体のないモンスターでも「〜の体」が残る)。
- 英語版の誤り(訳では直した): graffiti の `@graffiti_author_any@`(下線抜け)、decorlines の
  `Jiyva peaceful fountain_blue` の重複、hints のタグの誤り。monstatus と mutations にも重複キー。
- docs のヘルプは訳して crawl-ref/docs/ja/ に置いた(下の「ヘルプ文書」)。

## newgame.cc
- `_highlight_pattern()` は英語の種族名を `show_help('%')` に渡している。適性表を訳すときは
  日本語のパターンが必要。

## tileweb.cc(Web 版。優先度低)
- `action->quiver_verb()` が英語(訳は quiver.txt の `[quiver verb]<動詞>`)。
- :1273 の状態の DB 検索名と " (expiring)" の判定は `fill_status_info_en()` で。
- :1141 の称号、:1323 の素手攻撃名、:1457-1463 の menu_colour(`item_name_en`)。

## initfile.cc
- 最初の rc 読み込みは DB の初期化より前なので、その時点のオプションのエラー文は英語。

## アイテム関係の名前関数の表示
- `sub_type_string`, `ego_type_string`, `missile_brand_name`, `potion_type_name`
  などは英語のまま。表示する所では辞書キー(`jtrans("potion of " + 名前)`、
  `tagged_jtrans("[item adj]", "of " + エゴ)` など)で包む。
- `item_base_name()` は英語のまま(item-name.cc の英語の経路が使う)。
- 装備スロット名は transform.txt の `[equip slot]Weapon` などを使える。

## 未対応のまま残した細部
- melee-attack: Gyre / Gimble の名前の設定、`hands_act("burn")`、`floodify` の物質名。
- mutation-data.h の分類名 any / good / bad / slime / corrupt(ほぼウィザード用)。
- abyss.cc の Xom の判定 `who != "a distortion unwield"` と player-equip.cc が渡す
  "unwielding a weapon of distortion" の不一致(本家側。触らない)。
- teleport.cc の `jtrans(" " + conj_verb(verb) + "!")` は実行時に組み立てるキー
  (hops! / leaps! / blinks! を手で辞書に入れた)。
- menu.txt にタグなしの短いキー("exit" "select" "page up" "page down")がある。
  衝突したらタグ付きに替える。
- beam.txt に使わなくなったキー("Really fire at %s%s?%s"、"Really fire in %s direction%s?%s")。
- C++ に残る英文は、ほぼデバッグ・内部エラー・死因など意図して残したもの。包み忘れは
  `python -I ja-tools/find_unwrapped.py`(通常と `-v`)で洗い出せる。
- output.cc のタッチ UI(Android)の固定幅ラベル "XL Next" / "Place" / "Noise" は英語のまま。
- wiz-item.cc:907 に本家のバグ(`if (entry.slot = SLOT_GIZMO)`。触っていない)。

## 全体
- 死因の文字列(ouch/hurt に渡す aux、"miscasting ..." など)とマイルストーンは
  スコアファイルに残り比較にも使うので包んでいない。表示は hiscores の `*_j()` で訳す。
- `wrap` は `#include "database.h"` を自ファイルのヘッダより前に入れることがある。
  その場合はアルファベット順の位置に直す。`wrap` は `#ifdef DEBUG_*` 内の
  mprf などのデバッグ出力も包むので戻す。`yesno("…" TOURNEY "…")` のような
  リテラル連結も壊すことがある。
- 書式文字列: 日本語の語順に合わせて引数の数・順番を変えた呼び出しがある
  (キーの英文と %s の数が合わない)。訳を直すときは呼び出し側の引数を確認する
  (`jtools.py fmt`)。
- 句読点を全角にしたので、英文に句読点を足してから辞書を引いている所は、
  引いた後で足すようにする。先頭の ", " を `erase(0, 2)` で削る処理に「、」を混ぜない。
- タイル版の文字領域(TextRegion)は全角文字を 2 マス(2 マス目は `WIDE_CHAR_FILLER`)で持つ。
  TextRegion の cbuf を直接読み書きする処理を足すときはこれに合わせる。ウェブタイル版
  (tileweb-text.cc)は別の仕組みで、手を入れていない(未確認)。
- 結合文字(幅 0)・制御文字は TextRegion で 1 マスのまま(本家の TODO のとおり)。

## ヘルプ文書(crawl-ref/docs/ja/)
- 訳はゲームが読む完成形(折り返し済みテキスト)。command.cc の `_doc_path()` が、日本語のとき
  `docs/ja/<名前>` を先に探す。対象はマニュアル、適性表、クイックスタート、マクロ、オプション、
  タイル版ヘルプ、更新履歴(0.34 系の部分だけ。版の見出し `Stone Soup 0.34.x` と `Highlights` の
  語はゲームが探すので残す)。
- 作業の形: 英語の控え `ja-work/docs-en/`(分担用に `parts/`)、訳 `ja-work/docs-ja/parts/`、規約
  `ja-work/docs-ja/RULES.md`。`python -I ja-tools/assemble_docs.py` で組み合わせて docs/ja/ に書き、
  幅を検査し、適性表を作る。`ja-tools/docwrap.py` は表示幅 79 桁での折り返しと検査。
- 本家の文書が変わったら、ビルドで生成される `crawl-ref/docs/crawl_manual.txt` などと控えの差分を
  見て訳を直す。英語版のマニュアルとクイックスタートは rst/md から生成されるもの(util/gen-all.py)。
- 適性表 `docs/ja/aptitudes.txt` は `ja-tools/gen_apt_ja.py` が英語版の表から作る(前文は
  `crawl-ref/docs/template/ja/apt-preface.txt`、種族名は data-species.txt)。gen-all.py(MSVC の
  ビルド前処理)から呼ぶ。Linux の Makefile には足していない。
- 新規ゲームの画面から適性表を開くときの種族名の強調(newgame.cc の `_highlight_pattern`)は
  訳した種族名で探す。
- 用語: quiver は「セット」(0.19-ja は「射撃準備」。0.34 では呪文・能力も入るので変えた)、inscription は「銘」、mutation は「変異」、skill は
  「スキル」。マニュアルの目次はゲーム内の目次(command.txt)と
  同じ見出し。
- 用語を置き換えたところは、行が短いまま残っている(段落を詰め直していない)。
- 訳さずに残したもの: オプション名・値・rc の例・Lua、正規表現の例の英文(message_colour など
  日本語版では日本語のメッセージに一致させる旨は説明に添えた)、menu_colour の英語のアイテム名
  (英語名で照合する)。
