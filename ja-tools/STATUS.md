# 日本語化の進捗と引き継ぎ(ja-0.34)

新しいスレッドで作業を再開するときは、まずこのファイルと
`CONVENTIONS.md`、`README.md`、リポジトリの 1 つ上の `../CLAUDE.md` を読む。

## 目標

0.19-ja を土台に DCSS 0.34 安定版を完訳する。順序:
1. C++ の張り直し(英文を jtrans で包み、辞書 `dat/database/ja/jtrans/` に訳を書く)
2. dat/des, dlua, clua 内の英文
3. 説明文・台詞の未訳(`ja-work/stale`, `ja-work/missing`)の全訳
4. 実機で確認しながら表示の不具合を直す

ユーザーの方針: 判断が必要なときは聞いてよいが、基本は完訳まで自走する。

## 完了

- 説明文・DB データの移行(`ja-tools/migrate_db.py`)
- jtrans の土台(database.cc)、japanese.cc/h、表示用の名前関数(`*_j`)
- 全角文字の折り返し・禁則(libutil.cc, stringutil.cc, fontwrapper-ft.cc)
- 所有格「〜の」(english.cc の apostrophise)
- message.cc(canned_msg、神の発言、--more-- 案内)

## C++ 張り直しの分担計画

各ファイルは規約の手順(old → wrap → 手作業 → harvest → 翻訳 → check)で処理。
サブエージェントに 5〜10 ファイルずつ割り当て、終わるたびにまとめてビルド。

第 1 陣(実行中、または完了済み。git log で確認):
- item-name.cc/.h(アイテム名の中核)
- mon-info.cc/.h, mon-util.cc/.h, monster.cc/.h(モンスター名の中核)
- データ辞書: data-spells/skills/species/jobs/gods/branches/features/zaps.txt
- データ辞書: data-durations/forms/mutations/unrands.txt
- potion.cc, item-use.cc, player-equip.cc, evoke.cc, delay.cc, items.cc
- spl-damage.cc, spl-summoning.cc, spl-transloc.cc, spl-selfench.cc,
  spl-other.cc, spl-monench.cc, spl-goditem.cc, spl-clouds.cc,
  spl-miscast.cc, spl-vortex.cc

第 2 陣以降の候補(英文の多い順。データ表を表示する側も含む):
- 戦闘: beam.cc, melee-attack.cc, attack.cc, fight.cc, ranged-attack.cc, throw.cc, mon-project.cc
- モンスター行動: mon-cast.cc, mon-ench.cc, mon-death.cc, mon-act.cc, mon-abil.cc, mon-behv.cc, mon-speak.cc, mon-place.cc, mon-poly.cc, mon-explode.cc, mon-clone.cc, mon-transit.cc, mon-tentacle.cc, mon-movetarget.cc, mon-pathfind.cc, mon-aura.cc, mon-gear.cc
- 宗教: religion.cc, god-abil.cc, god-wrath.cc, god-conduct.cc, god-passive.cc, god-companions.cc, god-prayer.cc, god-item.cc, god-blessing.cc, god-menu.cc, describe-god.cc, xom.cc, attitude-change.cc, piety-info.cc
- プレイヤー: player.cc, player-reacts.cc, player-act.cc, player-notices.cc, player-stats.cc, ouch.cc, transform.cc, mutation.cc, skills.cc, skill-menu.cc, ability.cc, status.cc, timed-effects.cc, movement.cc, death-curse.cc, behold.cc, fearmonger.cc
- 説明・UI: describe.cc, describe-spells.cc, lookup-help.cc, output.cc, invent.cc, menu.cc, command.cc, hints.cc, tutorial.cc, newgame.cc, ng-*.cc, startup.cc, main.cc, directn.cc, target.cc, target-compass.cc, prompt.cc, quiver.cc, macro.cc, outer-menu.cc, scroller.cc, ui.cc, viewmap.cc, view.cc, show.cc, viewchar.cc, tilereg-*.cc, game-options.cc
- 世界: stairs.cc, travel.cc, terrain.cc, traps.cc, cloud.cc, abyss.cc, zot.cc, orb.cc, areas.cc, exclude.cc, branch.cc, feature.cc, dgn-overview.cc, shopping.cc, stash.cc, acquire.cc, artefact.cc, decks.cc, spl-cast.cc, spl-book.cc, spl-util.cc, spl-zap.cc, randbook.cc, item-prop.cc, known-items.cc, makeitem.cc, corpse.cc, shout.cc, teleport.cc, misc.cc
- 記録: hiscores.cc, chardump.cc, notes.cc, kills.cc, end.cc, files.cc, dactions.cc, fineff.cc, arena.cc, wizard.cc
- 対象外・後回し: wiz-*.cc, dbg-*.cc, l-*.cc(Lua バインディング)、tile*.cc の内部処理、
  mapdef.cc / dungeon.cc / dgn-*.cc(迷宮生成)、initfile.cc(オプション解析。
  ヘルプ文など表示部分だけ後で)、tags.cc、lang-fake.cc、tileweb.cc(Web 版)

## ビルドと確認

```powershell
$env:PATH = "C:\Program Files\Git\usr\bin;" + $env:PATH
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" `
  "L:\project4\DCSS\crawl\crawl-ref\source\MSVC\crawl.vcxproj" /m `
  /p:Configuration="Release Tiles" /p:Platform=x64 `
  /p:WindowsTargetPlatformVersion=10.0.26100.0 /p:BuildProjectReferences=false /v:minimal
```

初回だけソリューション全体(`crawl-ref.sln`、依存ライブラリ込み)をビルドする。
起動確認は `crawl-ref/source/crawl.exe -rc <rc>`。rc の例(フォントは Windows の BIZ UDゴシック):

```
language = ja
tile_font_crt_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_msg_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_stat_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_tip_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_lbl_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
```

画面確認は computer-use で、アプリ名にパス
`L:\project4\DCSS\crawl\crawl-ref\source\crawl.exe` を指定して許可を取る。

## 第 1 陣の報告から出た、後続の担当への申し送り

- skills.cc / describe-god.cc: 称号の置換(`@Walker@`, `@Child@`, `@Weight@`,
  種族名など)が英語のまま入る。data-species.txt の `[spec genus]`,
  `[walker]`, `[walking]`, `[child]`、data-skills.txt の `[title]Heavy` 等を
  使って訳した値を入れる。`player_title()` の冠詞("La " や ", ")は不要。
- god_name_j(): ジーヴァの長い名前は data-gods.txt の
  `%s the Shapeless`(形なき『%s』)で組み立てる処理が必要。
- descript/ja/backgrounds.txt の Artificer「熟練工」・Warper「転移術師」が
  職業名(data-jobs.txt の発動術師・転位術師)と不一致。説明文の全訳時に揃える。
- dat/database/ja/godname.txt は神の長い名前を一部しか訳していない。
- DUR_CONFUSING_TOUCH の終了メッセージは `you.hands_act("stop", "glowing.")` で
  組み立てられるので、C++ 側で `[hands act]...` キーを引く対応が必要。
- 0.34 で新しく決めた訳語(他の担当も揃える): Will=意志力(括弧内の (Will+) は
  英語のまま)、Slay=殺傷、Apt=適性、XL=レベル、Speed=速度。
  新種族: 鬼、ジンニー、バラチ、アルマタウル、メテオラン、コグリン、山ドワーフ、
  汚泥エルフ、ポルターガイスト。支部: Gauntlet=試練場、Crucible=肉の坩堝、
  Necropolis=死者の都。
- 辞書の重複キーは最後に一括チェックする(特に `[verb]hit` などの攻撃動詞)。

## 既知の問題・未対応

- `crawl-ref/source/nul` はビルドの前処理が作るゴミ。コミットしない。
- 既定の設定(settings/init.txt)にはまだ `language = ja` とフォント指定を
  入れていない。配布時は OFL の日本語フォント(BIZ UDGothic 等)を同梱して
  既定にする予定。
- 辞書の重複キー(同じ英文を複数ファイルに書いたもの)は最後にまとめて整理する。
- コミットは `git -c user.name=yamashirok -c user.email=yamashirok@n-ring.jp commit`
  (git の利用者設定がないため)。
