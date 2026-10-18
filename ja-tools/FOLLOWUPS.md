# ファイル別の申し送り

先に作業した担当が見つけた、他のファイルで対応が必要な点。
そのファイルを担当するときに読み、対応したら項目を消す。

## fight.cc
- `stop_attack_prompt()`: 動詞を `tagged_jtrans("[stop attack prompt]", verb)` で
  引き(訳は「を〜し」の形。spl-damage.txt 等にキーあり)、
  "Really %s%s %s%s?%s" を日本語の語順に組み替える。
- `warn_about_bad_targets()`: 英文が未対応。呪文名は `spell_title_j`。

## player.cc
- `hands_act()`: 0.19-ja と同じく `[hands act]your hands ...` のキーで引く
  (キーは spl-selfench.txt。DUR_CONFUSING_TOUCH の終了メッセージもこれ)。
- `resists_dislodge()`: "Your boots keep you from %s." を包む(引数は呼び出し側で
  「〜こと」の形で渡している)。
- `check_moveto*`: 英語の動作名("blink", "hop", "passwall" など)を受け取るので、
  表示側で訳す。

## spl-cast.cc
- `start_channelling_spell()`: "(Press <w>%</w> to " + msg + ".)" の組み立てを
  日本語の語順に。
- `list_spells()`: action 引数として日本語の「忘却」を受け取るようになっている。

## status.cc
- spl-goditem の取り消し効果の説明で、状態の短い表示を `jtrans(short_text)` で
  引いている。status.cc 側で先に訳すと " (expiring)" の除去が効かなくなるので
  整合をとる。

## describe.cc
- `describe_player_cancellation()` を埋め込んでいる 2 つの文を訳す。

## monster.cc
- `resist_margin_phrase()` が英語を返す(モンスター名担当の作業後に要確認)。

## attack.cc
- `attack_strength_punctuation()` が半角の "." "!" を返す。全角にする。

## melee-attack.cc / mon-cast.cc
- `airstrike_intensity_display()` は連用形を返すようになったので合わせて訳す。
- mon-cast / mon-death / mon-ench から `forest_message()` に渡す英文は呼び出し側で包む。

## beam.cc
- ビーム名の表示を `zap_name_j` で引く("shock of your passage",
  "barrage of energy" など)。

## traps.cc
- `held_status()` が英語を返す。

## directn.cc / terrain.cc
- Zin の牢獄の marker の "a gleaming silver wall" と地形名の訳。
- data-features.txt に "wall", "door", "tree" などの総称の `[feature]` 訳が必要。

## skills.cc / describe-god.cc
- 称号の置換(`@Walker@`, `@Child@`, `@Weight@`, 種族名など)が英語のまま入る。
  data-species.txt の `[spec genus]`, `[walker]`, `[walking]`, `[child]`、
  data-skills.txt の `[title]Heavy` 等を使う。`player_title()` の冠詞は不要。

## religion.cc
- `god_name_j()`(japanese.cc): ジーヴァの長い名前は data-gods.txt の
  `%s the Shapeless`(形なき『%s』)で組み立てる。

## DB テキスト(dat/database/ja)
- 呪文失敗時の文(spl-miscast の `_do_msg`)、`getMiscString("summon_horrible_things")`
  の日本語訳が DB にあるか確認。spl-miscast.cc の "'s body" 除去は英文向けのまま。
- data-items.txt に、薬の味の文で使う `potion of haste` などのキーが必要。

## アイテム名の英語照合(item_name_en への置き換え)
`item_def::name()` は日本語を返す。英語名で照合している所は
`item_name_en(item, desc, ...)`(item-name.h、引数は name() と同じ)に替える。
- オプションのパターン照合: showsymb.cc:263、invent.cc:103/179
  (items.cc の 100 行付近・2356〜2380 行付近は第 1 陣の担当に依頼済み)
- menu_colour にアイテム名を渡している所: acquire.cc:1467, command.cc:321,
  l-item.cc:445, shopping.cc:1017/2447, stash.cc:1773, output.cc:1152/2102,
  quiver.cc:403/507/946, tileweb.cc:1457-1463
- Lua の item.name()(l-item.cc:415)。dat/clua/stash.lua:141 が英語前提。
- 日本語名に英語文法をかけている所: describe.cc:5517(pluralise)、
  mon-cast.cc:7212(pluralise)、directn.cc:3317(article_a(ghost_brand_name()))

## artefact.cc
- ランダムアーティファクト名(make_artefact_name)は英語の合成文字列で、
  セーブの props にも英語で残る。表示時に日本語化する方法を考える。
  ギズモ名(gizmo.txt)も同様。

## アイテム関係の名前関数の表示
- `sub_type_string`, `ego_type_string`, `missile_brand_name`, `potion_type_name`
  などは英語のまま。表示する所では辞書キー(`jtrans("potion of " + 名前)`、
  `tagged_jtrans("[item adj]", "of " + エゴ)` など)で包む。
- ability.cc / spl-cast.cc の `player_silenced_reason()` は
  `tagged_jtrans("[silenced reason]", …)` で引ける(キーは item-name.txt)。

## delay.h
- `EquipOnDelay` / `EquipOffDelay` の `tick()` の "You continue %s %s." を
  `mprf(MSGCH_MULTITURN_ACTION, tagged_jtransc("[eq continue]", get_verb()), 名前)`
  に書き換える(訳は delay.txt にある)。

## travel.cc
- `runmode_name()` の訳は `[runmode]explore` などとして delay.txt にある。

## fight.cc(追加)
- `stop_attack_prompt` の verb 引数として、item-use 系からは日本語の終止形の
  動詞句(「〜を読む」「震石を放つ」)が渡ってくる。その前提で文を組み立てる。

## place.cc
- `prep_branch_level_name()` は前置詞なしの場所名(「ダンジョン第3階層」)を返す
  前提で items.cc 等が訳している。

## describe.cc(追加)
- `origin_desc()` の後ろに "." を足している所は「。」に。
- `item_unequip_verb()` は英語のまま。それで組み立てる文は組み替える。

## 理由文字列を返す関数(各関数側で日本語化が必要)
- `invis_allowed`, `can_go_berserk`, `cant_transform_reason`,
  `cannot_read_item_reason` / `cannot_drink_item_reason`(item-name.cc は済)、
  `no_selectables_message`, `ignore_faith_reason`
- `you.hands_act()`(player-act.cc)、`skill_names()`、orb.cc の `start_orb_run`

## ability.cc
- "Confirm evoke" の訳は item-use.txt にある(重複して足さない)。

## モンスター名の英語照合(*_en への置き換え)
monster / monster_info の名前関数・attributes() は日本語を返す。英語版は
`name_en`, `base_name_en`, `full_name_en`, `attributes_en` など。
- Lua: l-moninf.cc:858 の name → `full_name_en()`、:835 の status →
  `attributes_en()`(autofight.lua 等が英語で比較)。l-mons.cc 54/64/69/156–158 も `*_en`。
- 台詞 DB のキー: mon-speak.cc 657/664, mon-behv.cc 1286/1321,
  mon-cast.cc 6779/8165/8177, god-abil.cc 3188, transform.cc 2532 → `name_en` 等。
- 比較・保存: dgn-overview.cc:965 の unique_name(strstr で英語比較)、
  hiscores.cc:1418(scorefile には英語を残す)。

## mons_type_name() の表示箇所 → mons_type_name_j()
directn.cc 3110/3408, mon-death.cc 437/1702, mon-poly.cc 639,
spl-summoning.cc 406, items.cc 1233, item-name.cc 1912, kills.cc 473,
player-notices.cc 186/340, melee-attack.cc 1158, god-abil.cc 5971, mon-ench.cc 938。
(mapdef / lookup-help / exclude / mon-cast 2940・8774 / l-* は英語のまま)

## 英文法が残る所(モンスター名)
- output.cc:1786 付近で "a " / "the " を足している。
- 日本語名に pluralise_monster / apostrophise をかけている所(mon-cast.cc:6733 等)。
- xom.cc の "starts %s" / "looks %s": description_for_ench() が日本語を返すので文側も訳す。
- player.cc の corrode() に渡す文字列("The trap's stomach acid" 等)。

## 全体
- 死因の文字列(ouch/hurt に渡す aux、"miscasting ..." など)とマイルストーンは
  スコアファイルに残り比較にも使うので包んでいない。死因の表示側
  (hiscores.cc)で訳す。
- `wrap` は `#include "database.h"` を自ファイルのヘッダより前に入れることがある。
  その場合はアルファベット順の位置に直す。
