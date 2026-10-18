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
  `mut_upgrade_summary()`(英語版 `mut_upgrade_summary_en()`)、`cant_transform_reason()`,
  `ability_name_j()`(新設。`ability_name()` は英語のまま)
- 二重の包み(害はないが不要): player.cc:3619 の `jtrans(inf.long_text)`、
  movement.cc / status.cc の `jtransc(held_status())`、player-equip.cc / evoke.cc /
  items.cc の `jtransc(you.hand_name(…))`、ability.cc `_desc_sac_mut` の外側の `jtrans`

## 能力名の表示 → ability_name_j()
describe.cc:4827(`inf.title`)、adjust.cc:202、quiver.cc:1471/1482、tilereg-abl.cc:49。
(l-you.cc:961 の Lua と chardump.cc:1537 は英語のままでよい)

## 称号の表示側
- output.cc:1432/1454/2183、tileweb.cc:1141、ghost.cc:528、wiz-item.cc:
  `player_title()` が冠詞なしの日本語を返すので、名前との組み立てを日本語の形にする
  (旧版は「称号『名前』」)。output.cc の `title[0] == ','` の判定は不要。
- スコアファイル・ghost に保存される称号も日本語になる。英語を残すなら describe-god.cc に
  `god_title_en()` を足し、skills.cc に英語版の称号関数を作って hiscores.cc:1724 から呼ぶ。

## player.cc(残り)
- 毒・麻痺の原因として props に保存する名前(POISONER_KEY、DISABLED_BY_KEY)が
  「あなた」になる。hiscores.cc の `death_source_name == "you"` は `jtrans("you")` とも比べる。
- `vex()`: mon-cast から `special_msg` に日本語が渡る。
- "You now have enough gold to %s." の訳(「%sのに…」)が religion.cc の「〜こと」形の
  部分文と合わない(religion.cc 側は「あなたは十分な所持金があるので、%sができる。」)。
- tilepick.cc:3954 の `you.hand_name(false)` の英語比較 → `hand_name_en(false)`。

## fight.cc(残り)
- `stop_summoning_prompt` の verb に beam.cc から「%sを魅了」が渡る。連用形
  (「%sを魅了し」)か `[stop summoning]` 付きキーに揃える。

## beam.cc(残り)
- 撃つ確認(7960 行付近)は fight.txt の "fire at %s" / "fire in %s direction" /
  "Really %s%s?%s" を使える。
- mon-cast の `hit_verb`("envelops", "skewers", "grips", "grasp", "engulfs",
  "passes through")とクァザラル・マクレブの hit_verb の表示側の訳
  (`[beam verb]<動詞>`、助詞から書く)。
- god-abil.cc:7013 付近(マクレブ)で `get_short_name()` をそのまま表示している →
  `tagged_jtrans("[beam name]", ...)`。
- 汎用キー "The %s %s %s%s%s%s"(beam/ranged-attack 共通)、"%s %s!"、"The %s %s %s." は beam.txt。
- explode_noise_msg と "You exhale a blast of poison gas." は beam.cc の表示側で訳している。

## ranged-attack.cc
- `apply_damage_brand()` に渡す投射物の名前から "the " を外す。

## quiver.cc
- :566 "You cannot attack while %s." が未包(`held_status()` は日本語の連体形)。
- :519 の素手攻撃名は日本語が返るようになった。

## mon-cast.cc の台詞判定 / mon-speak.cc
- 台詞の日本語文を「身振り」「指さ」「指差」で探して身振りかどうかを判定している。
- mon-speak.cc の `_invalid_msg` は、プレイヤーが相手でないときに「あなた」を含む ja 台詞を弾く。
  台詞 DB を訳すときは注意。

## terrain.cc / directn.cc
- `feature_description_at()` は player.cc 側で `tagged_jtrans("[feature]", …)` で包んでいる。
- Zin の牢獄の marker の "a gleaming silver wall" と地形名の訳。
- `get_monster_equipment_desc(mi, level, DESC_NONE)` の結果を player-notices.cc の遭遇通知で
  「%sは%s。」の述語として使っている → 「〜を装備している」の形で返す。DESC_WEAPON の結果は
  「そこには%sがある。」に入るので名詞の形。

## movement.cc
- 扉の marker 文("veto_reason", "door_verb_already_open")は vault 由来の英語のまま表示される
  (des 側の作業で訳す)。

## describe.cc
- `describe_player_cancellation()` を埋め込んでいる 2 つの文を訳す。
- `origin_desc()` の後ろに "." を足している所は「。」に。
- `item_unequip_verb()` は英語のまま。それで組み立てる文は組み替える。
- `get_player_aux_names()` の並べ方(" and ")を日本語に。

## religion.cc / describe-god.cc
- `makhleb_mark_name()` は英語の変異名を返す。"You have been branded with the %s." は
  `jtransc(makhleb_mark_name())` で引く。マクレブの印の説明 `get_mutation_desc()` も英語。

## spl-cast.cc
- `start_channelling_spell()`: "(Press <w>%</w> to " + msg + ".)" の組み立てを日本語の語順に。
- `list_spells()`: action 引数として日本語の「忘却」を受け取るようになっている。

## spl-book.cc
- :1108 で `god_spell_warn_string()`(日本語)を英文前提で " " でつないでいる。

## decks.cc
- `deck_name()` は英語。ability.cc は `jtrans(deck_name())` で引くので、"deck of destruction"
  (破壊のデッキ)、"deck of escape"(脱出のデッキ)、"deck of summoning"(召喚のデッキ)、
  "stacked deck"(積み札)の訳を decks.txt に。`deck_description()`、`deck_summary()`
  (「あなたは今%sを持っている。」に入る)、`deck_status()` も英語。
- カード名は `tagged_jtrans("[card]", stack_top())` で引かれる。

## shopping.cc
- god-abil.cc のゴザーグの店で `tagged_jtrans("[shop]", 店の種類名や "Shoppe" などの接尾語)` を
  使っている。キーを shopping.txt に(旧辞書の jtrans_shopping に同じ形のキーあり)。
- ゴザーグの店主名は `make_name()` の英語の合成名のまま「の」を付けて表示している。

## notes.cc
- `NOTE_DEFEAT_MONSTER` の desc に英語の動詞("killed" など)が入る。表示側で訳す。
- "Anointed %s the %s as your apostle" などのノートの定型文は英語のまま。渡す語
  (職名、祖霊の種類、印の名前、「禁断の知識」)は日本語。`perma_mutate` の理由は英語のまま。

## chardump.cc / wiz-you.cc / abyss.cc
- chardump.cc:891 の `"You are " + describe_xom_favour()` は
  `jtrans(describe_xom_favour())` で「あなたはゾムの慰みものだ。」になる(キーは xom.txt)。
  "You were" の場合は別の訳が要る。
- wiz-you.cc:328 の "You are now …" の訳は xom.txt にある。
- abyss.cc:603 の "Xom snickers loudly." は xom.cc の中で訳すので包まない。

## hiscores.cc(死因の表示)
- 死因・召喚者名は英語のまま保存している。表示側で訳す:
  - 神罰の名前("the fury of Makhleb" など。訳は god-wrath.txt)
  - `non_actor_summoner = "the anger of Yredelemnul"`、god-wrath.cc の "Fedhas Madash"
  - 変異の理由 "Jiyva's grace", "Makhleb's blessing", "the all-consuming vengeance of Jiyva"
  - `mons_add_blame("anointed by Ignis")`、mon-clone.cc の "woven by ..."
- player.cc の項の "you" 比較も参照。status 欄は `fill_status_info_en()` に替え済み。

## item-prop.cc / item-name.cc
- mon-gear.cc:1356 で死神とシグムンドの武器に `custom_name = "scythe"` を入れ、
  props(ITEM_NAME_KEY)に英語で保存している。表示する item-prop.cc:3245 で訳す
  (キー `scythe` は data-items.txt にある)。

## god-companions.cc
- `apostle_short_description()` が英語。

## hints.cc
- スキル画面・能力メニュー用のヒント文(`hints_skills_info()` など)が未訳。

## main.cc / skills.cc
- "Game saved, see you later!"(skills.cc:821、main.cc)が未訳。

## Lua(l-*.cc と dat/clua, dat/dlua)
- l-moninf.cc:858 の name → `full_name_en()`、:835 の status → `attributes_en()`
  (autofight.lua 等が英語で比較)。l-mons.cc 54/64/69/156–158 も `*_en`。
- l-you.cc:1111 / autofight.lua:423: `you.caught()` が日本語を返し、
  "You are " .. caught .. "!" で英語と混ざる。英語版の関数を足すか lua 側の文を訳す。
- `crawl.bane_name` を使う lm_trove.lua の英文に日本語が混ざる。
- Lua の item.name()(l-item.cc:415)。dat/clua/stash.lua:141 が英語前提。
- 実機: "... is nearby!" と "No target in view!"(自動戦闘)が英語。

## 未対応のまま残した細部
- melee-attack: Gyre / Gimble の名前の設定、`hands_act("burn")`、`floodify` の物質名。
- mutation-data.h の分類名 any / good / bad / slime / corrupt(ほぼウィザード用)。

## DB テキスト(dat/database/ja)
- 呪文失敗時の文(spl-miscast の `_do_msg`)、`getMiscString("summon_horrible_things")`
  の日本語訳が DB にあるか確認。spl-miscast.cc の "'s body" 除去は英文向けのまま。
- 台詞 DB の未訳: "Beogh apostle challenge" など、ja/godspeak.txt のゾムの台詞
  (246 件中およそ 25 件のみ)、ja/monspeak.txt の `nobody_recollection fire` など約 10 件、
  `recite_closure`(ジンの朗唱の結び)、`eel hand actions` / `eel hand solo actions`、
  `getMiscString(<名前> + " title")`(名前付きモンスターの肩書き)、
  `harlequin_trap_lines`(今は traps.txt で表示側 jtrans)。
- 神の説明画面の能力・神罰の段落(DB)に未訳がある。

## artefact.cc
- ランダムアーティファクト名(make_artefact_name)は英語の合成文字列で、
  セーブの props にも英語で残る。表示時に日本語化する方法を考える。
  ギズモ名(gizmo.txt)も同様。

## アイテム名の英語照合(item_name_en への置き換え)
`item_def::name()` は日本語を返す。英語名で照合している所は
`item_name_en(item, desc, ...)`(item-name.h、引数は name() と同じ)に替える。
- オプションのパターン照合: showsymb.cc:263、invent.cc:103/179
- menu_colour にアイテム名を渡している所: acquire.cc:1467, command.cc:321,
  l-item.cc:445, shopping.cc:1017/2447, stash.cc:1773, output.cc:1152/2102,
  quiver.cc:403/507/946, tileweb.cc:1457-1463
- 日本語名に英語文法をかけている所: describe.cc:5517(pluralise)、
  directn.cc:3317(article_a(ghost_brand_name()))

## アイテム関係の名前関数の表示
- `sub_type_string`, `ego_type_string`, `missile_brand_name`, `potion_type_name`
  などは英語のまま。表示する所では辞書キー(`jtrans("potion of " + 名前)`、
  `tagged_jtrans("[item adj]", "of " + エゴ)` など)で包む。
- `player_silenced_reason()` は `tagged_jtrans("[silenced reason]", …)` で引ける(キーは item-name.txt)。
- 装備スロット名は transform.txt の `[equip slot]Weapon` などを使える(command.cc:288/301 など)。

## travel.cc
- `runmode_name()` の訳は `[runmode]explore` などとして delay.txt にある。
- 探索中の "Found ..." なども未訳(実機で確認)。

## place.cc
- `prep_branch_level_name()` は前置詞なしの場所名(「ダンジョン第3階層」)を返す
  前提で items.cc、god-abil.cc(黒き松明)が訳している。

## 理由文字列を返す関数(各関数側で日本語化が必要)
- `no_selectables_message`, orb.cc の `start_orb_run`

## モンスター名の英語照合(*_en への置き換え)
- 比較・保存: dgn-overview.cc:965 の unique_name(strstr で英語比較)、
  hiscores.cc:1418(scorefile には英語を残す)。

## mons_type_name() の表示箇所 → mons_type_name_j()
directn.cc 3110/3408, spl-summoning.cc 405, items.cc 1236, item-name.cc 1963, kills.cc 473。
(mapdef / lookup-help / exclude / mon-cast 8774 付近 / l-* は英語のまま)

## 英文法が残る所(モンスター名)
- output.cc:1786 付近で "a " / "the " を足している。

## 全体
- 死因の文字列(ouch/hurt に渡す aux、"miscasting ..." など)とマイルストーンは
  スコアファイルに残り比較にも使うので包んでいない。死因の表示側
  (hiscores.cc)で訳す。
- `wrap` は `#include "database.h"` を自ファイルのヘッダより前に入れることがある。
  その場合はアルファベット順の位置に直す。`wrap` は `#ifdef DEBUG_*` 内の
  mprf などのデバッグ出力も包むので戻す。
- 書式文字列: 日本語の語順に合わせて引数の数・順番を変えた呼び出しがある
  (キーの英文と %s の数が合わない)。訳を直すときは呼び出し側の引数を確認する
  (`jtools.py fmt`)。
- 句読点を全角にしたので、英文に句読点を足してから辞書を引いている所は、
  引いた後で足すようにする。
