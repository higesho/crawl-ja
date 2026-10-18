# ファイル別の申し送り

先に作業した担当が見つけた、他のファイルで対応が必要な点。
そのファイルを担当するときに読み、対応したら項目を消す。

## player.cc / player-act.cc(第 2 陣で張り直し済み。残り)
- `player::name()` は「あなた」「あなたの」を返すようになった。毒・麻痺の原因として
  props に保存する名前(POISONER_KEY、DISABLED_BY_KEY)も「あなた」になるので、
  hiscores.cc の `death_source_name == "you"` は `jtrans("you")` とも比べる。
- `vex()`: mon-cast から `special_msg` に日本語(「〜の支配の企てに激しく反発した！」)が渡る。
- "You now have enough gold to %s." の訳(「%sのに…」)が religion.cc の「〜こと」形の
  部分文と合わない(religion.cc 側は「あなたは十分な所持金があるので、%sができる。」)。
- `player::pronoun()` は英語のまま。
- 手足名: `hand_name()` などは日本語を返す。英語版は `hand_name_en()` など(player.h)。
  - tilepick.cc:3954 の英語比較 → `hand_name_en(false)`
  - mutation.cc:1992 の `pluralise(hand_name())` を外す
  - player-equip.cc / evoke.cc / items.cc の `jtransc(you.hand_name(…))` は二重の包み(無害)
- 能力値名は `strength` などのキーで引ける(player-stats.txt)。"You feel stronger." なども
  player-stats.txt にある(mutation.cc で重複させない)。

## actor.cc
- `resist_margin_phrase()` が英語を返す。beam.cc は表示側で `jtrans` をかけて訳は
  beam.txt にあるが、mon-cast.cc は `"You%s"`, `"%s%s"` などに直接つないでいる。
  助詞から始まる日本語(「は辛うじて抵抗した。」)を返す形に統一し、呼び出し側を揃える。
- `stumble_away_from()`: 2 つの文が未訳。`src` は日本語で渡す前提(melee「衝撃」、
  mon-cast「爆風」、throw.cc「『ラバ』の反動」)。mon-cast / spl-cast の "the blast" も日本語に。
- `knockback(..., name)` に英語のビーム名が渡る → 表示側で `zap_name_j`。
  "being knocked back" は melee-attack.txt、"being dragged" は mon-cast.txt にある。
- 拘束から抜ける "%s %s free of %s!" の動詞("slip", "burst", "leap", "dance", "jump")。

## misc.cc
- `attacked_monster_list::describe()`: 形容語が日本語になったので、"the " の付加と
  `pluralise_monster` を外す(beam.cc の発射確認にも出る)。

## fight.cc(第 2 陣で張り直し済み。残り)
- `stop_summoning_prompt` の verb に beam.cc から「%sを魅了」が渡る。連用形
  (「%sを魅了し」)か `[stop summoning]` 付きキーに揃える。
- ability.cc / god-abil.cc から来る "invoke Cleansing Flame" と "Slouch" の
  `[stop attack prompt]` キーがない。キーを足すか日本語の終止形で渡す。

## beam.cc(第 2 陣で張り直し済み。残り)
- 撃つ確認(7960 行付近)は fight.txt の "fire at %s" / "fire in %s direction" /
  "Really %s%s?%s" を使える。
- mon-cast の `hit_verb`("envelops", "skewers", "grips", "grasp", "engulfs",
  "batters", "passes through")とクァザラル・マクレブの hit_verb の表示側の訳
  (`[beam verb]<動詞>`、助詞から書く)。
- god-abil.cc:7013 付近(マクレブ)で `get_short_name()` をそのまま表示している →
  `tagged_jtrans("[beam name]", ...)`(get_short_name は台詞の検索キーなので英語のまま)。
- 汎用キー "The %s %s %s%s%s%s"(beam/ranged-attack 共通)、"%s %s!"、"The %s %s %s." は beam.txt。
- mon-explode.cc / decks.cc の `explode_noise_msg`、ability.cc の
  "You exhale a blast of poison gas." は beam.cc の表示側で訳している(キーは beam.txt。重複させない)。

## ranged-attack.cc
- `apply_damage_brand()` に渡す投射物の名前から "the " を外す。
- `evasion_margin_adverb()` は空白なしの日本語の副詞を返すようになった。

## mon-explode.cc
- `summoned_poof_msg()` は英語を返す → 表示は `tagged_jtrans("[poof]", …)`(キーは mon-death.txt)。

## mon-cast.cc の台詞判定
- 台詞の日本語文を「身振り」「指さ」「指差」で探して身振りかどうかを判定している。
  台詞 DB を直すときは注意。

## traps.cc
- `held_status()` は英語を返す。player.cc は呼び出し側で `jtransc()`、throw.cc は
  連体形(「網に捕らわれている」)を返す前提で「%s間」に組み込んでいる。どちらかに揃える。

## terrain.cc / directn.cc
- `feature_description_at()` は player.cc 側で `tagged_jtrans("[feature]", …)` で包んでいる。
- Zin の牢獄の marker の "a gleaming silver wall" と地形名の訳。

## status.cc
- spl-goditem の取り消し効果の説明で、状態の短い表示を `jtrans(short_text)` で
  引いている。status.cc 側で先に訳すと " (expiring)" の除去が効かなくなるので整合をとる。
- `describe_contamination()` は日本語を返す。`stat_desc(SD_NAME)` は `jtrans()` で訳せる。
- :652 の物質名は `[substance]` で引ける(water は monster.txt、ooze は player.txt)。

## player-reacts.cc
- :922 の物質名は `[substance]` で引ける。
- ジンの朗唱: `"\"%s\""` を『%s』にし、"You finish reciting %s" の末尾の '.' も直す。
  `zin_recite_text(-1)` は「『忌まわしきもの』3章5節」の形を返す。

## skills.cc
- `skill_title()` には `god_title()`(describe-god.cc)から訳済みの神の称号が入る。
  神以外の称号を訳し、`@Weight@` は `[title]Heavy` などで置換する。
  神の称号がスコアファイルに日本語で残る点は要判断。

## describe.cc
- `describe_player_cancellation()` を埋め込んでいる 2 つの文を訳す。
- `origin_desc()` の後ろに "." を足している所は「。」に。
- `item_unequip_verb()` は英語のまま。それで組み立てる文は組み替える。
- `get_player_aux_names()` が日本語名を返すようになった。並べ方(" and ")を日本語に。

## ability.cc
- "Confirm evoke" の訳は item-use.txt にある(重複して足さない)。
- "Recruit " / "Dismiss " + `get_apostle_name()`(`with_title` のとき「名前、戦士」)を訳す。
- `ru_sac_text()`, `yred_cannot_light_torch_reason()`, `dithmenos_cannot_*_reason()` は日本語を返す。
- `make_cost_description()` の "None" を訳すなら「なし」に(describe-god.cc がその値と比べている)。
  コスト表示の "Piety-" なども未訳(神の画面に出る)。
- `player_silenced_reason()` は `tagged_jtrans("[silenced reason]", …)` で引ける(キーは item-name.txt)。

## religion.cc(第 2 陣で張り直し済み。残り)
- `makhleb_mark_name()` は英語の変異名を返す。describe-god.cc の
  "You have been branded with the %s." は `jtransc(makhleb_mark_name())` で引く。

## mutation.cc
- ルーの犠牲のマイルストーンに `mut_upgrade_summary()` の結果が入る(日本語化すると混ざる)。
- `bane_name()` の "Bane of %s" と各 bane、マクレブの印の `mutation_name()` の訳が要る。
- `scale_type` は player.txt の `[scale colour]` を流用できる。
- 種族の疑似変異(species-data.h の fake_mutations)の訳は data-species.txt にある。

## transform.cc
- `base_hand_name()` は英語のまま("Stone %s" などの組み立てと `get_long_name` 用)。
- 素手攻撃名 `unarmed_attack_name()` は英語。表示する output.cc:1177/2152、
  quiver.cc:519、tileweb.cc:1323 で `tagged_jtrans("[uc attack]", …)` で包む。
- 祈りの動作 `player_prayer_action()` は `[verb]` で引かれる(data-forms.txt / data-species.txt)。

## spl-cast.cc
- `start_channelling_spell()`: "(Press <w>%</w> to " + msg + ".)" の組み立てを日本語の語順に。
- `list_spells()`: action 引数として日本語の「忘却」を受け取るようになっている。

## spl-book.cc
- :1108 で `god_spell_warn_string()`(日本語を返すようになった)を英文前提で " " でつないでいる。

## decks.cc
- `deck_summary()` が英語を返す(「あなたは今%sを持っている。」に入る)。`deck_status()` も。

## shopping.cc
- god-abil.cc のゴザーグの店で `tagged_jtrans("[shop]", 店の種類名や "Shoppe" などの接尾語)` を
  使っている。キーを shopping.txt に(旧辞書の jtrans_shopping に同じ形のキーあり)。
- ゴザーグの店主名は `make_name()` の英語の合成名のまま「の」を付けて表示している。

## notes.cc
- `NOTE_DEFEAT_MONSTER` の desc に英語の動詞("killed" など)が入る。表示側で訳す。
- "Anointed %s the %s as your apostle" などのノートの定型文は英語のまま。渡す語
  (職名、祖霊の種類、印の名前、「禁断の知識」)は日本語。`perma_mutate` の理由
  ("Ru sacrifice", "inscribed by the player")は英語のまま。

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
  - `mons_add_blame("anointed by Ignis")`
- player.cc の項の "you" 比較も参照。

## 未訳のまま他から受け取っている文字列
- player-notices: `multimonster_name_string()`
- god-companions: `apostle_short_description()`

## death-curse.cc など
- 句読点を全角にしたので、英文に句読点を足してから辞書を引いている所は、
  引いた後で足すようにする。

## 未対応のまま残した細部
- melee-attack: Gyre / Gimble の名前の設定、`hands_act("burn")`、`floodify` の物質名。
- 台詞 DB: "Beogh apostle challenge" などの ja 訳がない。ja/godspeak.txt のゾムの台詞は
  246 件中およそ 25 件しか訳がない。神の説明画面の能力・神罰の段落(DB)も未訳がある。

## monster.cc
- `resist_margin_phrase()` が英語を返す(actor.cc の項を参照)。

## artefact.cc
- ランダムアーティファクト名(make_artefact_name)は英語の合成文字列で、
  セーブの props にも英語で残る。表示時に日本語化する方法を考える。
  ギズモ名(gizmo.txt)も同様。

## DB テキスト(dat/database/ja)
- 呪文失敗時の文(spl-miscast の `_do_msg`)、`getMiscString("summon_horrible_things")`
  の日本語訳が DB にあるか確認。spl-miscast.cc の "'s body" 除去は英文向けのまま。

## アイテム名の英語照合(item_name_en への置き換え)
`item_def::name()` は日本語を返す。英語名で照合している所は
`item_name_en(item, desc, ...)`(item-name.h、引数は name() と同じ)に替える。
- オプションのパターン照合: showsymb.cc:263、invent.cc:103/179
- menu_colour にアイテム名を渡している所: acquire.cc:1467, command.cc:321,
  l-item.cc:445, shopping.cc:1017/2447, stash.cc:1773, output.cc:1152/2102,
  quiver.cc:403/507/946, tileweb.cc:1457-1463
- Lua の item.name()(l-item.cc:415)。dat/clua/stash.lua:141 が英語前提。
- 日本語名に英語文法をかけている所: describe.cc:5517(pluralise)、
  directn.cc:3317(article_a(ghost_brand_name()))

## アイテム関係の名前関数の表示
- `sub_type_string`, `ego_type_string`, `missile_brand_name`, `potion_type_name`
  などは英語のまま。表示する所では辞書キー(`jtrans("potion of " + 名前)`、
  `tagged_jtrans("[item adj]", "of " + エゴ)` など)で包む。

## travel.cc
- `runmode_name()` の訳は `[runmode]explore` などとして delay.txt にある。
- 探索中の "Found ..." なども未訳(実機で確認)。

## place.cc
- `prep_branch_level_name()` は前置詞なしの場所名(「ダンジョン第3階層」)を返す
  前提で items.cc、god-abil.cc(黒き松明)が訳している。

## 理由文字列を返す関数(各関数側で日本語化が必要)
- `cant_transform_reason`, `no_selectables_message`, `skill_names()`,
  orb.cc の `start_orb_run`
  (済: invis_allowed, can_go_berserk, flight_allowed, no_tele_reason,
  ignore_faith_reason, god_spell_warn_string, cannot_read/drink_item_reason)

## モンスター名の英語照合(*_en への置き換え)
monster / monster_info の名前関数・attributes() は日本語を返す。英語版は
`name_en`, `base_name_en`, `full_name_en`, `attributes_en` など。
- Lua: l-moninf.cc:858 の name → `full_name_en()`、:835 の status →
  `attributes_en()`(autofight.lua 等が英語で比較)。l-mons.cc 54/64/69/156–158 も `*_en`。
- 台詞 DB のキー: mon-speak.cc 657/664, transform.cc 2532 → `name_en` 等。
- 比較・保存: dgn-overview.cc:965 の unique_name(strstr で英語比較)、
  hiscores.cc:1418(scorefile には英語を残す)。

## mons_type_name() の表示箇所 → mons_type_name_j()
directn.cc 3110/3408, spl-summoning.cc 405, items.cc 1236, item-name.cc 1963,
kills.cc 473, player-notices.cc 186/340。
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
  (キーの英文と %s の数が合わない)。訳を直すときは呼び出し側の引数を確認する。
