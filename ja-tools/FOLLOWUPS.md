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
- スコアファイル・ゴーストファイルに保存される称号は日本語のままでよい(このフォークは
  日本語専用のため)。`god_title_en()` などは作らない。
- 毒・麻痺などの原因(DISABLED_BY_KEY, POISONER_KEY)と死因に残る名前は英語で保存し、
  表示側(hiscores の `death_name_j()`)で訳す。プレイヤー自身は "you"。

## des(dat/des)と dlua(dat/dlua)
- vault の扉 marker(`door_description_*`、"veto_reason", "door_verb_already_open")と
  `feat_renames` は英語のまま(directn.cc / movement.cc は訳があれば訳す作り)。
- lm_trove.lua の `it.name()`、lm_timed.lua の `feature_desc_at` と "vanishes" などの文は
  英語の名前・文を表示している。`name_j()`、`feature_desc_at_j()`、`crawl.jtrans` に替える。
  `crawl.bane_name` は日本語を返すので `crawl.jtrans("… %s …", crawl.bane_name(…))` の形で。
- dlua/des の名前比較(lm_trig、test.lua、trog_wizard.des、trog_burn_book.des、trove.des、
  iter.lua)は英語の値が前提(第 5 陣で英語に戻した)。

## DB テキスト(dat/database/ja, dat/descript/ja)
進み具合は `python -I ja-tools/dbtools.py status` で見る。主な未訳・注意点:
- 台詞 DB: "Beogh apostle challenge" など、ja/godspeak.txt のゾムの台詞、ja/monspeak.txt の
  `nobody_recollection fire` など、`recite_closure`(ジンの朗唱の結び)、`eel hand actions` /
  `eel hand solo actions`、`getMiscString(<名前> + " title")`(名前付きモンスターの肩書き)、
  `harlequin_trap_lines`(今は traps.txt で表示側 jtrans)。
- mon-speak.cc の `_invalid_msg` は、プレイヤーが相手でないときに「あなた」を含む ja 台詞を
  弾く。mon-cast.cc は台詞の日本語文を「身振り」「指さ」「指差」で探して身振りか判定する。
  台詞を訳すときは注意。
- ja/shout.txt: imp の台詞、Sphinx riddle(`_riddle_*`)、`__BUGGY` など未訳
  (今は jtrans/shout.txt のテンプレート訳で応急対処。DB を訳したらテンプレート訳と整合を取る)。
- ja/wpnnoise.txt、ja/colourname.txt(宝石を全部集めたときの色名)、misc DB の噴水・食料の文
  (`_walk_on_decor`。`@your_weapon@` / `@your_hands@` には英語名を入れている)、
  `hell_effect_noisy` / `hell_effect_quiet`、呪文失敗時の文(spl-miscast の `_do_msg`)、
  `summon_horrible_things` の ja を確認・作成する。spl-miscast.cc の "'s body" 除去は英文向け。
- descript/ja/mutations.txt にマクレブの印 9 種("Mark of … mutation")の説明がない。
- descript/ja/hints.txt はほぼ未訳。神の説明画面の能力・神罰の段落も未訳がある。
- descript/ja/branches.txt の Lair などに 0.34 で変わった旧訳(「8の階層」など)が残る。
- descript/ja/backgrounds.txt の Artificer「熟練工」・Warper「転移術師」を職業名
  (data-jobs.txt の発動術師・転位術師)に揃える。dat/database/ja/godname.txt は一部のみ。
- docs のヘルプ(マニュアル、適性表、changelog)は英語のまま。

## notes.cc
- `NOTE_INFERNAL_MARK`("Branded self with the %s")と `NOTE_ANCESTOR_TYPE` の定型文が英語の
  ままで、日本語の名前が入る。

## game-options.cc / macro.cc
- `loadFromParseState()`(game-options.cc)と `read_rc_file_macro()`(macro.cc)が返す
  エラー文は initfile.cc の `report_error("%s", …)` を通ってそのまま出る。各ファイル側で包む。

## acquire.cc
- `_generate_gizmo_name()` は「形容詞 + 半角空白 + 名詞」。カタカナ語なら「・」区切りか
  空白なしの方が自然。

## describe.cc
- `trap_name()` / `full_trap_name()` が英語のまま。

## newgame.cc
- `_highlight_pattern()` は英語の種族名を `show_help('%')` に渡している。適性表を訳すときは
  日本語のパターンが必要。

## tileweb.cc(Web 版。優先度低)
- `action->quiver_verb()` が英語(訳は quiver.txt の `[quiver verb]<動詞>`)。
- :1273 の状態の DB 検索名と " (expiring)" の判定は `fill_status_info_en()` で。
- :1141 の称号、:1323 の素手攻撃名、:1457-1463 の menu_colour(`item_name_en`)。

## initfile.cc
- 最初の rc 読み込みは DB の初期化より前なので、その時点のオプションのエラー文は英語。

## invent.cc / 選択メニュー
- `prompt_invent_item()`・`select_items()`・`_invent_select()` は渡された題名を訳さない。
  呼び出し側で包む。`equip_slot_name()` は英語のまま(表示側で `[equip slot]`)。

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
- C++ に残る英文は、ほぼデバッグ・内部エラー・ウィザード用(wiz-*.cc は未着手)。
  `scratchpad` の unwrapped.py のような grep で時々洗い出す。

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
