# 日本語化の進捗と引き継ぎ(ja-0.34)

新しいスレッドで作業を再開するときは、まずこのファイルと
`CONVENTIONS.md`、`FOLLOWUPS.md`(ファイル別の申し送り)、`README.md`、
リポジトリの 1 つ上の `../CLAUDE.md`(日本語で話すルール)を読む。

## 次にやること(2026-10-19 時点)

目標の 1〜4 は一通り完了。実機での総点検(キャラ作成、ステータス欄、持ち物・説明画面、呪文、
キャラクター概要、技能、ヘルプ、探索、戦闘、バーサーク、能力、ダンジョン概要、死亡画面、
スコア、ダンプ)を行い、見つかった表示の不具合を直した(下の「完了」の総点検の項)。
続けて、店、神の祭壇・入信、ポータル(バザール)・ダンジョン概要、変異の一覧と説明、
装備メニュー、ウィザードモードの入力欄、タイル版のツールチップ(能力・持ち物・ミニマップ)、
メッセージ履歴も確認した。
ゲーム内ヘルプの文書(マニュアル、適性表、クイックスタート、マクロ、オプション、タイル版ヘルプ、
0.34 の更新履歴)も訳した(2026-10-19。FOLLOWUPS の「ヘルプ文書」)。英語のまま残るのは、方針で
英語にしている引用文 182 件と、デバッグ用の文だけ。
ゲーム後半もウィザードモードで確認した(2026-10-19): 分岐の移動メニュー、アビス、パンデモニウム、
ゲヘナ、ジッグラト、宝物庫・ゾット・地獄の入口、ルーンと宝石の一覧、ルーンとオーブを拾ったとき、
変身(竜)、脱出と勝利画面、勝利時のダンプ。直したのは、勝利時の「… さらに1個のルーンを手にして」
(「… 1個のルーンとともに」に)、勝っても出る「死亡ダンプは」(「キャラクターダンプは」に)、ダンプの
見出しなどに残っていた「突然変異」(薬の名前と台詞以外は「変異」に)。英語のまま残るのはウィザード
専用の変身メニュー(内部名)と、マップ定義の内部エラー文だけ。
最初のリリース 0.34.1-ja1 を用意した(2026-10-19、タグ 0.34.1-ja1)。Windows 64bit のタイル版 zip と
ソースの zip を `../release/` に作った(手順は `ja-tools/release/README.md`)。公開(push とリリースの作成)は
まだしていない。
Android 版にも日本語化を入れた(2026-10-19、ブランチ `ja-0.34-android`。2026-10-20 に ja-0.34 へ取り込んだ)。
エミュレータ(Android 15、x86_64)で、スタート画面・キャラクター作成・ゲーム画面が日本語で出ることを確かめた。
日本語フォントは BIZ UDゴシック(OFL、`crawl-ref/source/fonts-ja/`)を APK に同梱する。縦長の画面の小さな
ステータス欄の見出しは英語のまま。組み方と分かったことは `ja-tools/ANDROID_PORT.md`。
ローカルのタイル版にゲームパッド対応を入れた(2026-10-19、ブランチ `ja-0.34-gamepad`。2026-10-20 に
ja-0.34 へ取り込んだ)。十字・左スティックで移動、A/B で決定・取消、ほかのボタンにコマンド、rc の `pad_bind`、
LB/RB の同時押し、割り当ての無いボタンはマクロのキー。ゲームメニューの「ゲームパッドの割り当て」で、
ゲームの中から割り当てを変えて pad_binds.txt に保存できる。設計と確認の記録は `ja-tools/GAMEPAD_DESIGN.md`、
状態機械の単体検査は `python -I ja-tools/gamepad_test/run.py`。右スティックでマウスのカーソル、L3・R3 で
左右クリック、ダンジョンの上のホイールで拡大縮小も足した。2026-10-20 にパッドの実機で問題なく動いた。
Android 版でも物理のパッドとバーチャルパッド(起動画面で選ぶ)を使えるようにし、パッドのときは
画面のキーボードを出さない。メッセージ欄で 1 文字を待っていた問い(能力値の上昇など)をカーソルで
選べるようにし、数の問いは上下で増減できるようにした(2026-10-21。GAMEPAD_DESIGN.md §9・§10)。
英語版 wiki の翻訳とスポイラーサイトの道具(`-spoiler-json` を含む)は、ゲームと混ぜないように
ブランチ `ja-0.34-wiki` に置く(ja-0.34 の変更はそちらへ取り込む。逆は取り込まない)。
リリース 0.34.1-ja2(2026-10-20)に、訳の直し・Android 版・ゲームパッド対応を入れた。
今後は、遊びながら見つけた不自然な訳・英語の残り・表示の崩れを直していく。

## 目標

0.19-ja を土台に DCSS 0.34 安定版を完訳する。順序:
1. C++ の張り直し(英文を jtrans で包み、辞書 `dat/database/ja/jtrans/` に訳を書く)
2. dat/des, dlua, clua 内の英文
3. 説明文・台詞の未訳(`ja-work/stale`, `ja-work/missing`)の全訳
4. 実機で確認しながら表示の不具合を直す

ユーザーの方針: 区切りごとに確認を求めず、コミットしながら完訳まで自走する(2026-10-18 の指示)。

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

- 第 3 陣: 下記の第 3 陣の全ファイル(辞書キー約 1,100)。データ辞書の追加
  (地形の総称、変身中の肉体名)、マイルストーン・スコア・Lua 用の英語版の使い分け
  (`mut_upgrade_summary_en`, `fill_status_info_en`)。ビルドエラー 1 件(mon-speak.cc の
  波括弧)を直して通し、実機で能力メニュー(「バーサーク」「信仰心-」)、神の画面、
  ステータス欄の称号(「切りつける者」)を確認済み。辞書の未訳 0・重複 0。

- 第 4 陣: 下記の第 4 陣の全ファイル(辞書キー約 2,900)。説明画面、ステータス欄、
  照準・地形、コマンド・ヘルプ、タイトル・キャラ作成・ヒント、呪文の詠唱・デッキ・店、
  階段・探索・鳴き声。取りまとめで ja/randbook.txt を 0.19-ja から移植・追訳、
  ベオ信徒の種族名 46 件、「ギズモ」に訳語統一。ビルドは通った(リンク時に crawl.exe が
  起動中で 1 回失敗しただけ)。実機確認は IME の窓が前面に居座って入力できず未実施
  (第 5 陣の確認時にまとめて見る)。辞書の未訳 0・重複 0。

- 第 5 陣: 下記の第 5 陣の全ファイル(辞書キー約 1,100 と DB)。死因・ダンプ・ノートの表示
  (保存は英語のまま、表示用の `*_j()` を新設)、ランダムアーティファクト名を生成時に日本語で
  組み立て(ja/randname・rand_*・gizmo を移植・追訳)、Lua 連携(`crawl.jtrans`、比較用の値を
  英語に)、地形名の表示の日本語化、プレイヤー由来の原因名を "you" で保存。
  一発でビルドが通った。実機確認は IME の窓のため未実施(第 4 陣の分も)。
  辞書の未訳 0・重複 0。

- 第 6 陣: dlua(80 キー)と des(37 ファイル、433 キー)の表示文、説明文 DB の
  monsters(724 件すべて)、features・clouds・monstatus・passives、status・mutations、
  spells・ability(本家の旧訳 129 件の見直しを含む)。訳語の統一(君主爆弾、影の人形、
  魔火花の従者、ウカド・ムー、灼心のゴーレム、業火の迫撃砲、白金の傑作、鉄縛団、技能名)。
  BOM なしの dat ファイルを CP932 で読んでいた不具合を unicode.cc で修正(Lua に日本語を
  書くと構文エラーになっていた)。`crawl.exe -builddb` と起動(タイトル画面の読み込み完了)
  まで確認。キー入力は IME の窓のため未確認。

- 第 7 陣: DB の残りを全訳。monspeak(731 件)、monspell・godspeak・shout・wpnnoise、
  items・egos・unrand・unident・cards、gods・branches・skills・species・backgrounds・
  commands・tutorial、hints と小さな DB 12 ファイル、quotes(283 件。182 件は著作権の
  関係で英語のまま)。取りまとめで duration-data.h・delay.h を包み、喋る武器の台詞・
  噴水の文・名前の生成の置換を日本語化、訳語を統一。`-builddb` まで確認。

- 第 8 陣(仕上げ): art-func.h(103 件)、wiz-*.cc(371 件+wiz-dump)、C++ の細部
  (ギズモ名の区切り、movement の veto_reason、eel の部位名)、包み忘れの洗い出し
  (13 箇所、ja-tools/find_unwrapped.py)、訳語の揺れの監査と統一(約 200 箇所、
  ja-work/term-audit.md)。ビルドは警告 0、`-builddb` まで確認。

- 実機での総点検(2026-10-19): 見つけて直したもの
  - メッセージ欄で前の行の末尾が消えずに残る(タイル版の文字領域は 1 文字 1 マスなのに、
    表示幅で空白を足していた。message.cc の out_line を clear_to_end_of_line に)
  - 短いメッセージをまとめるとき「。」の後ろに「; 」が入る(message.cc の
    _ends_in_punctuation に全角の句読点・閉じ括弧を追加)
  - ヘルプの 2 段組みなど、空白で桁を揃えた表が行ごとにずれる(全角の送り幅が空白の
    2 倍にならない。fontwrapper-ft.cc で全角の送り幅を空白の 2 倍にそろえた)
  - ダンプが CP932 で書き出され、CP932 にない字(「噛」など)が「?」になる(chardump.cc で
    UTF-8 のまま書く)
  - 説明画面の見出しの名前の後ろの句点、所持品の見出しの重複、呪文選択の案内のキー表示、
    ゲーム更新のノートとダンプの行動表の見出し(total)
  - 字の種類が多い画面(ウィザードモードのヘルプなど)で字が化ける(フォントの字形置き場
    (atlas)が 256 字しかなく、描画の途中で使い回されていた。fontwrapper-ft.cc で 1024 字に
    広げ、探索を表引きに)
  - メッセージ欄の折り返し判定がバイト数だった(cio.cc の wrapcprint_skipping を表示幅に)
  - 入力欄(ウィザードの「どの種類の地形を…」など)が質問文から大きく離れて出る。タイル版の
    文字領域(TextRegion)は 1 文字 1 マスで、全角文字を 2 マス分の幅で描くため、表示幅で
    cgotoxy すると全角文字の数だけ右にずれていた。全角文字は 2 マスを使い、2 マス目に
    幅 0 の詰め物(unicode.h の WIDE_CHAR_FILLER)を置くようにした(tilereg-text.cc の
    addstr_aux、fontwrapper-ft.cc の render_textblock)。これでマスと表示上の列が一致する
  - 注意: computer-use のスクリーンショットは、Crawl が描き直す前の古い画面(灰色の空の
    ポップアップや全面灰色)を写すことがある。数秒待ってから zoom で撮り直すと正しく写る。
    空のポップアップに見えても不具合とは限らない
  - 確認の手順: STATUS の「ビルドと確認」を参照。computer-use は前面のアプリしか操作できず、
    `open_application` は新しく起動してしまうので、確認の間だけ crawl-ref/source/init.txt に
    日本語設定を置いて `open_application` で起動し、操作のたびにタイトルバーをクリックする
    (前面がデスクトップのときはクリックの許可に "File Explorer" が要る)。終わったら init.txt を消す。

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

第 3 陣(完了):
- ability.cc/.h, skill-menu.cc
- transform.cc/.h, mutation.cc/.h
- player-reacts.cc, player-notices.cc, ouch.cc, timed-effects.cc
- status.cc/.h, movement.cc, skills.cc, death-curse.cc, behold.cc, fearmonger.cc
- mon-act.cc, mon-abil.cc, mon-explode.cc, mon-place.cc, mon-speak.cc
- mon-clone.cc, mon-transit.cc, mon-tentacle.cc, mon-aura.cc, god-menu.cc/.h,
  attitude-change.cc(mon-gear, mon-movetarget, mon-pathfind, god-item, piety-info は
  表示する英文なしを確認)
- actor.cc, misc.cc, traps.cc, cloud.cc(+ 呼び出し側の resist_margin_phrase / held_status の整理)

第 4 陣(完了):
- describe.cc
- output.cc, quiver.cc, invent.cc, adjust.cc, tilereg-*.cc
- directn.cc/.h, target.cc, target-compass.cc, prompt.cc, terrain.cc, viewmap.cc, view.cc(show.cc は英文なし)
- main.cc, command.cc, macro.cc, menu.cc, game-options.cc/.h(outer-menu.cc は英文なし)
- newgame.cc, ng-input.cc, startup.cc, hints.cc, lookup-help.cc, describe-spells.cc
- spl-cast.cc, spl-book.cc, spl-util.cc/.h, randbook.cc, decks.cc, shopping.cc/.h, acquire.cc
- stairs.cc, travel.cc(+ level-id.h), abyss.cc, zot.cc, orb.cc, areas.cc, exclude.cc, branch.cc,
  dgn-overview.cc, shout.cc, teleport.cc, place.cc

第 5 陣(完了):
- hiscores.cc/.h, chardump.cc, notes.cc, kills.cc
- files.cc, end.cc, dactions.cc, fineff.cc, arena.cc, state.cc, nearby-danger.cc(ghost.cc は確認のみ)
- stash.cc, item-prop.cc, known-items.cc, makeitem.cc, artefact.cc, map-knowledge.cc
  + DB の ja/randname・rand_all・rand_wpn・rand_arm・gizmo
- initfile.cc, tilesdl.cc, dgl-message.cc, dungeon.cc(残りの UI・迷宮生成は確認のみ)
- 申し送りの残り: beam, fight, ranged-attack, mon-project, monster, mon-util, spl-summoning,
  spl-vortex, god-abil, player, tilepick, items, item-name, quiver, prompt, player-notices,
  showsymb, tilereg-dgn(+ 取りまとめで ouch, death-curse)
- Lua: l-crawl, l-moninf, l-mons, l-item, l-you, l-dgngrd と dat/clua

第 6 陣(完了): dat/dlua、dat/des、descript の monsters / features / clouds / monstatus /
passives / status / mutations / spells / ability

第 7 陣(完了): DB の残り全部(上の「完了」を参照)

第 8 陣(完了): 上の「完了」を参照。

実機で見えた未訳(第 2 陣の確認時): タイトル・キャラ作成画面(startup.cc, newgame.cc,
ng-*.cc)、ステータス欄(output.cc)、"You encounter ..."(player-notices.cc)、
"... is nearby!" と "No target in view!"(Lua の自動戦闘)、"Found ..."(travel.cc)、
"squeaks loudly."(shout.cc)。(神の画面のコストは第 3 陣で解消)

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
MSBuild は必ず `Binmd64\MSBuild.exe` を使う(`Bin\MSBuild.exe` だと別のコンパイラが選ばれ、
プリコンパイル済みヘッダーの版違い C1853 で失敗する)。perl の生成スクリプトが `source/nul` という
空ファイルを作ることがあるので、コミット前に消す。

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

IME(TextInputHost)の窓が前面に残ると、computer-use のキー入力もクリックも拒否される
(Windows の前面ロックで PowerShell からも切り替えられない)。その場合はユーザーに
IME の窓を閉じてもらうか、確認を次の機会に回す。
画面確認は computer-use で、crawl.exe を起動してから `request_access` に
`crawl.exe` を指定して許可を取る(表示名は "Crawl")。前面に出すときは `open_application` を
使わず、ウィンドウをクリックする(`open_application` は rc なしの crawl.exe を別に起動してしまい、
英語のままの画面を操作することになる)。rc が読まれたかはタイトル画面の Ctrl-P で確認できる。日本語配列なので `^` などの
記号は `key` ではなく `type` で送る(`shift+6` は `&` になりウィザードモードの確認が出る)。

## 用語・説明文の申し送り

- 0.34 で新しく決めた訳語(他の担当も揃える): Will=意志力(括弧内の (Will+) は
  英語のまま)、Slay=殺傷、Apt=適性、XL=レベル、Speed=速度。
  新種族: 鬼、ジンニー、バラチ、アルマタウル、メテオラン、コグリン、山ドワーフ、
  汚泥エルフ、ポルターガイスト。分岐: Gauntlet=試練場、Crucible=肉の坩堝、
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
