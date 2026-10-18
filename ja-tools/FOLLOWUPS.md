# ファイル別の申し送り

先に作業した担当が見つけた、他のファイルで対応が必要な点。
そのファイルを担当するときに読み、対応したら項目を消す。

## 日本語を返すようになった関数(呼び出し側で二重に包まない・英語前提で加工しない)
- 名前: `monster::name()` 系、`item_def::name()`、`player::name()`(「あなた」)、
  `hand_name()`/`foot_name()`/`arm_name()`(英語版は `*_name_en`)
- 第 2 陣: `evasion_margin_adverb()`, `get_player_aux_names()`, `describe_contamination()`,
  `ignore_faith_reason()`, `god_spell_warn_string()`, `god_title()`, `ru_sac_text()`,
  `yred_cannot_light_torch_reason()`, `dithmenos_cannot_*_reason()`, `zin_recite_text(-1)`,
  `can_go_berserk`/`invis_allowed`/`flight_allowed`/`no_tele_reason` の理由
- 第 3 陣: `resist_margin_phrase()`(助詞から始まる)、`held_status()`(連体形)、
  `multimonster_name_string()`, `describe_monsters_condensed()`, `skill_names()`,
  `player_title()`(冠詞なし)、`duration_end_message()`/`duration_expire_message()`,
  `fill_status_info()`(英語版 `fill_status_info_en()`)、transform の
  `get_description`/`transform_message`/`get_untransform_message`/`melding_description`/
  `get_long_name`/`get_uc_attack_name`、`unarmed_attack_name()`, `mutation_desc()`,
  `bane_name()`(dbkey なし)、`bane_desc()`, `terse_mutation_list()`,
  `mut_upgrade_summary()`(英語版 `mut_upgrade_summary_en()`)、`cant_transform_reason()`
- 第 4 陣: `no_selectables_message()`, `spell_uselessness_reason()` /
  `casting_uselessness_reason()` / `desc_cannot_memorise_reason()`(完結した文)、
  `spell_schools_string()`, `shop_name()`, `deck_description()`/`deck_summary()`/
  `deck_status()`/`deck_contents()`/`stack_contents()`/`which_decks()`,
  `desc_wl_success_chance()` などのターゲット説明、`describe_item_rarity()`(連体形)、
  `damage_rating()`, `describe_hit_chance()`/`describe_to_hit()`, `get_cloud_desc()` の見出し、
  `prep_branch_level_name()`(前置詞なし「迷宮の3階」)、`get_monster_status_descriptors()`、
  `get_monster_equipment_desc()`(DESC_NONE は述語形)
- 表示用に新設した日本語版(元の関数は検索キー・保存・比較のため英語のまま):
  `ability_name_j()`, `feature_description_at_j()`/`feature_description_j()`/
  `raw_feature_description_j()`(directn.h)、`get_monster_status_descriptor_pairs()`
  (英語, 日本語)、`level_id::describe_j()`, `spelltype_long_name_j()`/
  `spelltype_short_name_j()`(spl-util.h)、`get_ghost_description_j()`(describe.h)、
  `rand_store_names()`(shopping.h、カタカナの店主名)
- 二重の包み(害はないが不要): player.cc:3619 の `jtrans(inf.long_text)`、
  movement.cc / status.cc の `jtransc(held_status())`、player-equip.cc / evoke.cc /
  items.cc の `jtransc(you.hand_name(…))`、ability.cc `_desc_sac_mut` の外側の `jtrans`、
  item-use.cc:433 の `jtransc(no_selectables_message(...))`

## 地形名の表示 → feature_description_at_j() / feature_description_j()
DESC_A / DESC_THE の英語を表示している所: beam.cc:1304/1308、items.cc:2814、
mon-project.cc:584、monster.cc:5601、stairs.cc:198、quiver.cc:643、god-abil.cc:6344、
spl-summoning.cc:2714、files.cc:2460、spl-vortex.cc:475、map-knowledge.cc:139、stash.cc:264。
mon-util.cc:4433 の `@feature@` は `raw_feature_description_j()`。
すでに `tagged_jtrans("[feature]", …DESC_PLAIN)` で包んでいる所も `_j` に替えるとよい
(「大きな扉」、vault での改名、覆われた地形などが英語のまま残るため)。

## 系統名の表示 → spelltype_*_name_j()
chardump.cc:1014(short)、god-abil.cc:4546(表示用なら)。

## 称号の表示側(残り)
- tileweb.cc:1141、ghost.cc:528、wiz-item.cc: `player_title()` は冠詞なしの日本語。
  output.cc は「称号『名前』」の形にした。
- スコアファイル・ghost に保存される称号も日本語になる。英語を残すなら describe-god.cc に
  `god_title_en()` を足し、skills.cc に英語版の称号関数を作って hiscores.cc:1724 から呼ぶ。

## player.cc(残り)
- 毒・麻痺の原因として props に保存する名前(POISONER_KEY、DISABLED_BY_KEY)が
  「あなた」になる。hiscores.cc の `death_source_name == "you"` は `jtrans("you")` とも比べる。
- `vex()`: mon-cast から `special_msg` に日本語が渡る。
- "You now have enough gold to %s." の訳(「%sのに…」)が religion.cc の「〜こと」形の
  部分文と合わない(religion.cc 側は「あなたは十分な所持金があるので、%sができる。」)。
- tilepick.cc:3954 の `you.hand_name(false)` の英語比較 → `hand_name_en(false)`。

## fight.cc / beam.cc(残り)
- `stop_summoning_prompt` の verb に beam.cc から「%sを魅了」が渡る。連用形
  (「%sを魅了し」)か `[stop summoning]` 付きキーに揃える。
- beam.cc の撃つ確認(7960 行付近)は fight.txt の "fire at %s" / "fire in %s direction" /
  "Really %s%s?%s" を使える。
- mon-cast の `hit_verb`("envelops", "skewers", "grips", "grasp", "engulfs",
  "passes through")とクァザラル・マクレブの hit_verb の表示側の訳
  (`[beam verb]<動詞>`、助詞から書く)。
- god-abil.cc:7013 付近(マクレブ)で `get_short_name()` をそのまま表示している →
  `tagged_jtrans("[beam name]", ...)`。
- misc.cc の `attacked_monster_list::describe()` が発射確認にも出る(第 3 陣で日本語化済み)。

## ranged-attack.cc
- `apply_damage_brand()` に渡す投射物の名前から "the " を外す。

## mon-cast.cc の台詞判定 / mon-speak.cc
- 台詞の日本語文を「身振り」「指さ」「指差」で探して身振りかどうかを判定している。
- mon-speak.cc の `_invalid_msg` は、プレイヤーが相手でないときに「あなた」を含む ja 台詞を弾く。
  台詞 DB を訳すときは注意。

## player-notices.cc
- 踊る武器が単独のとき、`get_monster_equipment_desc` の DESC_WEAPON の名詞が「%sは%s。」に
  入る。「%sは%sだ。」にする必要がありそう。

## prompt.cc
- `confirm_prompt` の内部バッファが 200 バイト。日本語の長い確認文(約 140 バイト)が上限に
  近いので広げる。

## religion.cc / describe-god.cc
- `makhleb_mark_name()` は英語の変異名を返す。"You have been branded with the %s." は
  `jtransc(makhleb_mark_name())` で引く。マクレブの印の説明 `get_mutation_desc()` も英語。

## mutation.cc
- :846 で `get_size_adj()`(describe.cc、英語)を表示に使っている。

## god-abil.cc
- ゴザーグの店主名(`GOZAG_SHOPKEEPER_NAME_KEY`)を、生成時に
  `rand_store_names(random2(…))` で作るとカタカナになる。

## notes.cc
- `NOTE_DEFEAT_MONSTER` の desc に英語の動詞("killed" など)が入る。表示側で訳す。
- "Anointed %s the %s as your apostle" などのノートの定型文は英語のまま。渡す語
  (職名、祖霊の種類、印の名前、「禁断の知識」)は日本語。`perma_mutate` の理由は英語のまま。
- 開始時のノート(冒険開始、所持品)は日本語で保存されるようになった(main.cc)。

## chardump.cc / wiz-you.cc / abyss.cc
- chardump.cc:891 の `"You are " + describe_xom_favour()` は
  `jtrans(describe_xom_favour())` で「あなたはゾムの慰みものだ。」になる(キーは xom.txt)。
  "You were" の場合は別の訳が要る。
- chardump.cc:850 / hiscores.cc:2150 の `"You were " + prep_branch_level_name()` などは、
  前置詞なしの日本語の場所名に合わせて文を組み直す(「%sにいた」)。
- chardump.cc の `seed_description()` が英語(歓迎文・バージョン画面に出る)。
- wiz-you.cc:328 の "You are now …" の訳は xom.txt にある。

## hiscores.cc(死因の表示)
- 死因・召喚者名は英語のまま保存している。表示側で訳す:
  - 神罰の名前("the fury of Makhleb" など。訳は god-wrath.txt)
  - `non_actor_summoner = "the anger of Yredelemnul"`、god-wrath.cc の "Fedhas Madash"
  - 変異の理由 "Jiyva's grace", "Makhleb's blessing", "the all-consuming vengeance of Jiyva"
  - `mons_add_blame("anointed by Ignis")`、mon-clone.cc の "woven by ..."
- player.cc の項の "you" 比較も参照。status 欄は `fill_status_info_en()` に替え済み。

## kills.cc / dgn-overview.cc / player-notices.cc(幽霊)
- kills.cc:599 の "The ghost of " + `get_ghost_description()`(英語)を表示側で訳す
  (日本語版 `get_ghost_description_j()` がある)。
- `short_ghost_description()`(dgn-overview.cc:989、player-notices.cc:618)が英語。

## item-prop.cc / item-name.cc / artefact.cc
- mon-gear.cc:1356 で死神とシグムンドの武器に `custom_name = "scythe"` を入れ、
  props(ITEM_NAME_KEY)に英語で保存している。表示する item-prop.cc:3245 で訳す
  (キー `scythe` は data-items.txt にある)。
- ランダムアーティファクト名(make_artefact_name)は英語の合成文字列で、
  セーブの props にも英語で残る。randbook.cc と同じく生成時に日本語で組み立てて保存する
  方式が使える。`replace_name_parts()` の @player_name@ などが `apostrophise()` の英語のまま。
  ギズモ名(gizmo.txt)も英語の合成名。
- `trap_name()` / `full_trap_name()` は英語のまま。

## god-companions.cc
- `apostle_short_description()` が英語。

## player-save-info.cc
- `player_save_info::short_desc()` / `really_short_desc()`(タイトル画面のセーブ一覧と
  Tab の再開表示)が英語の可能性。

## newgame.cc
- `_highlight_pattern()` は英語の種族名を `show_help('%')` に渡している。適性表を訳すときは
  日本語のパターンが必要。

## tilereg-dgn.cc / tileweb.cc
- tilereg-dgn.cc の `alt == "Floor."` は、説明文が日本語になると一致しない可能性(未確認)。
- tileweb.cc が送る `action->quiver_verb()` は英語。訳は quiver.txt の `[quiver verb]<動詞>`。
- tileweb.cc:1273 の状態の DB 検索名と " (expiring)" の判定は `fill_status_info_en()` で。

## initfile.cc
- 最初の rc 読み込みは DB の初期化より前なので、その時点のオプションのエラー文は英語。

## invent.cc / 選択メニュー
- `prompt_invent_item()`・`select_items()`・`_invent_select()` は渡された題名を訳さない。
  呼び出し側で包む。`equip_slot_name()` は英語のまま(表示側で `[equip slot]`)。

## Lua(l-*.cc と dat/clua, dat/dlua)
- l-moninf.cc:858 の name → `full_name_en()`、:835 の status → `attributes_en()`
  (autofight.lua 等が英語で比較)。l-mons.cc 54/64/69/156–158 も `*_en`。
- l-you.cc:1111 / autofight.lua:423: `you.caught()` が日本語を返し、
  "You are " .. caught .. "!" で英語と混ざる。英語版の関数を足すか lua 側の文を訳す。
- `crawl.bane_name` を使う lm_trove.lua の英文に日本語が混ざる。
- Lua の item.name()(l-item.cc:415)。dat/clua/stash.lua:141 が英語前提。
- l-item.cc:891 と l-you.cc:284 に日本語の damage_rating の説明文が、clua の
  `target_spell_desc()` に日本語のターゲット説明が渡る。
- `dgn.feature_desc(_at)` は英語。lm_timed.lua の "vanishes" などの文は英語のまま。
- 実機: "... is nearby!" と "No target in view!"(自動戦闘)が英語。

## des(dat/des)
- vault の扉 marker(`door_description_*`、"veto_reason", "door_verb_already_open")と
  `feat_renames` は英語のまま(directn.cc / movement.cc は訳があれば訳す作り)。

## DB テキスト(dat/database/ja, dat/descript/ja)
- 呪文失敗時の文(spl-miscast の `_do_msg`)、`getMiscString("summon_horrible_things")`
  の日本語訳が DB にあるか確認。spl-miscast.cc の "'s body" 除去は英文向けのまま。
- 台詞 DB の未訳: "Beogh apostle challenge" など、ja/godspeak.txt のゾムの台詞
  (246 件中およそ 25 件のみ)、ja/monspeak.txt の `nobody_recollection fire` など約 10 件、
  `recite_closure`(ジンの朗唱の結び)、`eel hand actions` / `eel hand solo actions`、
  `getMiscString(<名前> + " title")`(名前付きモンスターの肩書き)、
  `harlequin_trap_lines`(今は traps.txt で表示側 jtrans)。
- ja/shout.txt は 44/93 項目。imp の台詞、Sphinx riddle(`_riddle_*`)、`__BUGGY` など未訳
  (今は jtrans/shout.txt のテンプレート訳で応急対処)。
- ja/wpnnoise.txt がない(喋る武器の台詞が英語)。
- misc DB の噴水・食料の文(`_walk_on_decor`)に ja がない。`@your_weapon@` /
  `@your_hands@` には英語名を入れている。DB を訳すときに日本語の名前に替える。
- descript/ja/hints.txt は 117 件中 16 件のみ(解説モードのヒントの大半が英語)。
- 神の説明画面の能力・神罰の段落に未訳がある。
- docs のヘルプ(マニュアル、適性表、changelog)は英語のまま。

## アイテム名の英語照合(item_name_en への置き換え、残り)
- オプションのパターン照合: showsymb.cc:263
- menu_colour にアイテム名を渡している所: l-item.cc:445, stash.cc:1773, tileweb.cc:1457-1463

## アイテム関係の名前関数の表示
- `sub_type_string`, `ego_type_string`, `missile_brand_name`, `potion_type_name`
  などは英語のまま。表示する所では辞書キー(`jtrans("potion of " + 名前)`、
  `tagged_jtrans("[item adj]", "of " + エゴ)` など)で包む。
- `player_silenced_reason()` は `tagged_jtrans("[silenced reason]", …)` で引ける(キーは item-name.txt)。
- 装備スロット名は transform.txt の `[equip slot]Weapon` などを使える。

## mons_type_name() の表示箇所 → mons_type_name_j()(残り)
spl-summoning.cc 405, items.cc 1236, item-name.cc 1963, kills.cc 473。
(mapdef / lookup-help / exclude / mon-cast 8774 付近 / l-* は英語のまま)

## 未対応のまま残した細部
- melee-attack: Gyre / Gimble の名前の設定、`hands_act("burn")`、`floodify` の物質名。
- mutation-data.h の分類名 any / good / bad / slime / corrupt(ほぼウィザード用)。
- abyss.cc の Xom の判定 `who != "a distortion unwield"` と player-equip.cc が渡す
  "unwielding a weapon of distortion" の不一致(本家側。触らない)。
- teleport.cc の `jtrans(" " + conj_verb(verb) + "!")` は実行時に組み立てるキー
  (hops! / leaps! / blinks! を手で辞書に入れた)。
- menu.txt にタグなしの短いキー("exit" "select" "page up" "page down")がある。
  衝突したらタグ付きに替える。

## 全体
- 死因の文字列(ouch/hurt に渡す aux、"miscasting ..." など)とマイルストーンは
  スコアファイルに残り比較にも使うので包んでいない。死因の表示側
  (hiscores.cc)で訳す。
- `wrap` は `#include "database.h"` を自ファイルのヘッダより前に入れることがある。
  その場合はアルファベット順の位置に直す。`wrap` は `#ifdef DEBUG_*` 内の
  mprf などのデバッグ出力も包むので戻す。`yesno("…" TOURNEY "…")` のような
  リテラル連結も壊すことがある。
- 書式文字列: 日本語の語順に合わせて引数の数・順番を変えた呼び出しがある
  (キーの英文と %s の数が合わない)。訳を直すときは呼び出し側の引数を確認する
  (`jtools.py fmt`)。
- 句読点を全角にしたので、英文に句読点を足してから辞書を引いている所は、
  引いた後で足すようにする。先頭の ", " を `erase(0, 2)` で削る処理に「、」を混ぜない。
