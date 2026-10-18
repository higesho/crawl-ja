# 日本語化の進捗と引き継ぎ(ja-0.34)

新しいスレッドで作業を再開するときは、まずこのファイルと
`CONVENTIONS.md`、`FOLLOWUPS.md`(ファイル別の申し送り)、`README.md`、
リポジトリの 1 つ上の `../CLAUDE.md`(日本語で話すルール)を読む。

## 次にやること(2026-10-18 時点)

C++ 張り直しの第 3 陣から。下の「第 3 陣の案」の 7 グループを
サブエージェントに並行で割り当てる(指示文の型は第 1・2 陣と同じ: CONVENTIONS.md と
FOLLOWUPS.md を読ませ、担当ファイルを列挙し、FOLLOWUPS.md の該当項目も対応させる。
担当ファイルとその辞書以外、japanese.cc/h、data-*.txt は編集させない。
data-*.txt に足す訳は報告させ、取りまとめ役が書く。ビルド・コミットはさせない)。
全員の完了後にまとめて dups/check → ビルド → 起動確認 → FOLLOWUPS.md 更新 → コミット。

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
- 第 1 陣(コミット ffb31efe8c): 下記の第 1 陣の全ファイル。実機で、アイテム名
  (「ハンドアックス (+0)」「2本の…薬」)とモンスター名(「オークの魔術師」)が
  日本語で表示されることを確認済み。辞書の未訳 0・重複 0。
- 第 2 陣: 下記の第 2 陣の全ファイル(辞書キー約 2,600)と、delay.h の装備継続文、
  データ辞書の追加(data-clouds.txt 新規、ビーム名・疑似変異・技能略称など)。
  一発でビルドが通り(警告は本家由来の 1 件のみ)、実機で神の画面(概要・能力・神罰)、
  近接戦闘の文(「ネズミはあなたを噛んだ。」「あなたはネズミへの攻撃をきわどく外した。」)、
  神の反応(「トログはあなたの殺しを善しとした。」)を確認済み。辞書の未訳 0・重複 0。

## C++ 張り直しの分担計画

各ファイルは規約の手順(old → wrap → 手作業 → harvest → 翻訳 → check)で処理。
サブエージェントに 3〜5 ファイルずつ(英文の量で均す)割り当て、終わるたびにまとめてビルド。

第 1 陣(完了):
- item-name.cc/.h(アイテム名の中核)
- mon-info.cc/.h, mon-util.cc/.h, monster.cc/.h(モンスター名の中核)
- データ辞書: data-spells/skills/species/jobs/gods/branches/features/zaps.txt
- データ辞書: data-durations/forms/mutations/unrands.txt
- potion.cc, item-use.cc, player-equip.cc, evoke.cc, delay.cc, items.cc
- spl-damage.cc, spl-summoning.cc, spl-transloc.cc, spl-selfench.cc,
  spl-other.cc, spl-monench.cc, spl-goditem.cc, spl-clouds.cc,
  spl-miscast.cc, spl-vortex.cc

第 2 陣(完了):
- beam.cc, mon-project.cc, throw.cc, ranged-attack.cc
- melee-attack.cc, attack.cc, fight.cc, mon-death.cc
- mon-cast.cc, mon-ench.cc, mon-behv.cc, mon-poly.cc
- religion.cc, god-conduct.cc, god-prayer.cc, god-passive.cc(+ japanese.cc の god_name_j)
- god-abil.cc, god-companions.cc, god-blessing.cc
- xom.cc, god-wrath.cc, describe-god.cc
- player.cc/.h, player-act.cc, player-stats.cc

第 3 陣の案(7 グループ。英文の量はおおよそ均してある):
1. ability.cc, skill-menu.cc
2. transform.cc, mutation.cc
3. player-reacts.cc, player-notices.cc, ouch.cc, timed-effects.cc
4. status.cc, movement.cc, skills.cc, death-curse.cc, behold.cc, fearmonger.cc
5. mon-act.cc, mon-abil.cc, mon-explode.cc, mon-place.cc, mon-speak.cc
6. mon-clone.cc, mon-transit.cc, mon-tentacle.cc, mon-aura.cc, mon-gear.cc,
   mon-movetarget.cc, mon-pathfind.cc, god-item.cc, god-menu.cc,
   attitude-change.cc, piety-info.cc
7. actor.cc, misc.cc, traps.cc, cloud.cc(FOLLOWUPS で多くの担当から参照される共通部品)

第 4 陣以降の候補:
- 説明・UI: describe.cc, describe-spells.cc, lookup-help.cc, output.cc, invent.cc, menu.cc, command.cc, hints.cc, tutorial.cc, newgame.cc, ng-*.cc, startup.cc, main.cc, directn.cc, target.cc, target-compass.cc, prompt.cc, quiver.cc, macro.cc, outer-menu.cc, scroller.cc, ui.cc, viewmap.cc, view.cc, show.cc, viewchar.cc, tilereg-*.cc, game-options.cc
- 世界: stairs.cc, travel.cc, terrain.cc, abyss.cc, zot.cc, orb.cc, areas.cc, exclude.cc, branch.cc, feature.cc, dgn-overview.cc, shopping.cc, stash.cc, acquire.cc, artefact.cc, decks.cc, spl-cast.cc, spl-book.cc, spl-util.cc, spl-zap.cc, randbook.cc, item-prop.cc, known-items.cc, makeitem.cc, corpse.cc, shout.cc, teleport.cc, place.cc
- 記録: hiscores.cc, chardump.cc, notes.cc, kills.cc, end.cc, files.cc, dactions.cc, fineff.cc, arena.cc, wizard.cc
- 対象外・後回し: wiz-*.cc, dbg-*.cc, l-*.cc(Lua バインディング)、tile*.cc の内部処理、
  mapdef.cc / dungeon.cc / dgn-*.cc(迷宮生成)、initfile.cc(オプション解析。
  ヘルプ文など表示部分だけ後で)、tags.cc、lang-fake.cc、tileweb.cc(Web 版)

実機で見えた未訳(第 2 陣の確認時): タイトル・キャラ作成画面(startup.cc, newgame.cc,
ng-*.cc)、ステータス欄(output.cc)、"You encounter ..."(player-notices.cc)、
"... is nearby!" と "No target in view!"(Lua の自動戦闘)、"Found ..."(travel.cc)、
"squeaks loudly."(shout.cc)、神の画面のコスト "Piety-"(ability.cc)。

## ビルドと確認

```powershell
$env:PATH = "C:\Program Files\Git\usr\bin;" + $env:PATH
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" `
  "L:\project4\DCSS\crawl\crawl-ref\source\MSVC\crawl.vcxproj" /m `
  /p:Configuration="Release Tiles" /p:Platform=x64 `
  /p:WindowsTargetPlatformVersion=10.0.26100.0 /p:BuildProjectReferences=false /v:minimal
```

初回だけソリューション全体(`crawl-ref.sln`、依存ライブラリ込み)をビルドする。
ビルドのログは UTF-8(BOM 付き)で出る。数分かかるのでバックグラウンドで回す。

起動確認は `crawl-ref/source/crawl.exe -rc <rc>`。rc は `crawl-ref/source/` に一時的に置き、
`-rc ja-test.rc` のように相対パスで渡す(引用符付きの絶対パスだと読まれず英語で起動する)。
rc の例(フォントは Windows の BIZ UDゴシック):

```
language = ja
tile_font_crt_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_msg_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_stat_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_tip_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_lbl_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
name = jatest2
```

画面確認は computer-use で、crawl.exe を起動してから `request_access` に
`crawl.exe` を指定して許可を取る(表示名は "Crawl")。日本語配列なので `^` などの
記号は `key` ではなく `type` で送る(`shift+6` は `&` になりウィザードモードの確認が出る)。

## 用語・説明文の申し送り

- 0.34 で新しく決めた訳語(他の担当も揃える): Will=意志力(括弧内の (Will+) は
  英語のまま)、Slay=殺傷、Apt=適性、XL=レベル、Speed=速度。
  新種族: 鬼、ジンニー、バラチ、アルマタウル、メテオラン、コグリン、山ドワーフ、
  汚泥エルフ、ポルターガイスト。支部: Gauntlet=試練場、Crucible=肉の坩堝、
  Necropolis=死者の都。
- descript/ja/backgrounds.txt の Artificer「熟練工」・Warper「転移術師」が
  職業名(data-jobs.txt の発動術師・転位術師)と不一致。説明文の全訳時に揃える。
- dat/database/ja/godname.txt は神の長い名前を一部しか訳していない。
- ファイル別の技術的な申し送りは `FOLLOWUPS.md` にまとめてある。

## 既知の問題・未対応

- `crawl-ref/source/nul` はビルドの前処理が作るゴミ。コミットしない。
  `grep.exe.stackdump` ができることもある(消す)。
- 既定の設定(settings/init.txt)にはまだ `language = ja` とフォント指定を
  入れていない。配布時は OFL の日本語フォント(BIZ UDGothic 等)を同梱して
  既定にする予定。
- 辞書の重複キーは `python -I ja-tools/jtools.py dups` で確認する(各陣の後に 0 件にする)。
- コミットは `git -c user.name=yamashirok -c user.email=yamashirok@n-ring.jp commit`
  (git の利用者設定がないため)。
- サブエージェントは scratchpad を共有するので、補助スクリプトは各自のサブフォルダに
  置くよう指示する(第 2 陣で上書きが起きた)。
