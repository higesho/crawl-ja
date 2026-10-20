# Android 版への日本語化の移植

本家の DCSS 0.34 には Android 版がある(`crawl-ref/source/android-project`、SDL2 + NDK、
`docs/develop/android.txt`)。このフォークの日本語化を、その Android 版でも使えるようにする。
作業ブランチは `ja-0.34-android`(ja-0.34 から分岐)。

## 1. 本家の Android 版の作り

1. `make ANDROID=1 android`(Linux 前提の Makefile)が、ホスト用のタイルの道具を組み、
   データ(des・dlua・database・descript・docs・settings・タイル・フォント)を
   `android-project/app/src/main/assets` に写し、`.android-cflags`・`app/build.gradle`・
   `local.properties` を生成する。C++ 本体はここでは組まない。
2. Gradle(AGP 8.11.2 / Gradle 8.13)が NDK の `jni/Android.mk` で C++ を組み、APK にする。
   ライブラリ(SDL2・freetype・lua・sqlite など)は `jni/` の下のシンボリックリンクで
   `contrib/` を指す。
3. ゲームはデータを APK の assets から読み(`ANDROID_ASSETS`、`AAssetManager`)、
   セーブや設定は外部ストレージのアプリ用の場所(`SDL_AndroidGetExternalStoragePath`)に置く。
4. 起動画面(`DCSSLauncher`)が、無ければ**空の** `init.txt` を外部ストレージに作る。
   ゲームはそれを読むので、assets の `settings/init.txt` は Android では使われない。

## 2. 日本語化で要ること

| # | 内容 | 対応 |
|---|---|---|
| 1 | 足したソース `japanese.cc` が NDK のソース一覧に無い | `jni/src/Android.mk` に足した(Makefile.obj と突き合わせて漏れは他に無い) |
| 2 | jtrans の辞書・`docs/ja` が assets に入るか | 入る(`install-data` が `dat/database` 一式と `docs/ja` を写す。ja-0.34 で対応済み) |
| 3 | **フォント**: `settings/init.txt` は `C:/Windows/Fonts/BIZ-UDGothicR.ttc` を指す。Android には無く、そもそも使われない(§1-4)。フォントが見つからないとゲームは起動時に終了する(`fontwrapper-ft.cc`) | **BIZ UDゴシック Regular(OFL)を APK に同梱**し(`fonts-ja/`。Makefile の android で `assets/dat/tiles` へ、ライセンス文は `assets/docs/license/BIZUDGothic-OFL.txt` へ)、起動画面が init ファイルを**新しく作るとき**に `language = ja` とこのフォントを書く。端末のフォントは使わない(6) |
| 4 | 起動画面などの Android 側の文字列(36 個) | `res/values-ja/strings.xml` を足した。端末の言語が日本語なら日本語になる |
| 5 | 起動画面の「オプションの説明」 | 端末の言語が日本語なら `docs/ja/options_guide.txt` を開く |
| 6 | 端末の Noto Sans CJK(`/system/fonts/NotoSansCJK-Regular.ttc`)は、Android 15 のエミュレータでは **CFF2 の可変フォント**だった。同梱の freetype 2.5.3 は CFF2 を読めない(2.8 から)。古い端末の CFF 版なら freetype に cff などのモジュールを足せば読めるが、端末によって動いたり動かなかったりするので採らなかった | 3 の同梱フォント(TrueType の輪郭なので freetype 2.5.3 のままで読める) |
| 7 | **jtrans の辞書が読まれず英語になる**。`_init_jtrans_files` の `get_dir_files_ext` は一つずつ `dir_exists` でディレクトリかを見るが、Android の assets では `AAssetManager_openDir` がどのパスにも成功するので、`.txt` が全部ディレクトリ扱いになって一覧から落ちていた | `database.cc`: 一覧(`get_dir_files_sorted`)を拡張子で絞るだけにした(下位のディレクトリは無い。Windows でも同じ結果) |
| 8 | **日本語の文で起動直後に落ちる**(SIGSEGV)。Android 版は Bionic の POSIX 正規表現(`regexec`)を使っていて、メッセージの絞り込みを 0x80 以上のバイトを含む文に当てると `mstep` で落ちる | `jni/src/Android.mk` で `-DREGEX_PCRE` と libpcre(`jni/pcre`)をリンクし、Java 側の読み込みの一覧にも `pcre` を足した。Windows 版と同じ PCRE になる |
| 9 | 縦長の画面ではステータス欄が小さな形(タッチ UI)になり、見出し(`AC EV SH`・`St In De`・`XL Next`・`Noise`・`Place`・`Time`)が英語のまま。前の日本語化で意図して残したもの(`output.cc` の `_hud_caption`。見出しを先頭のバイト数で切って 2 文字幅などに収める作りのため) | **未対応**。日本語にするなら短い訳を決め、切り方を表示幅にする |

## 3. この PC での組み方

本家は Linux で組む前提。この PC では、データの生成を WSL(Ubuntu)で、APK を Windows の
Android 環境(`C:\Android`。変愚の Android 移植で入れたもの)で組む。

| 段 | どこで | 何を |
|---|---|---|
| データ | WSL | `make -j16 ANDROID=1 android`(`/mnt/l/...` の作業ツリーのまま)。生成物はすべて Git の管理外 |
| `jni/` のリンク | Windows | このチェックアウトは `core.symlinks=false` なので、`jni/SDL2` などは「リンク先を書いたテキスト」になっている。ジャンクションに置き換え、`git update-index --skip-worktree` で変更として見せない(手元だけ。コミットしない) |
| `local.properties` | Windows | `sdk.dir=C\:/Android/sdk`(make が WSL の HOME で作った分を書き換える。管理外) |
| APK | Windows | Gradle 8.13(キャッシュ済み)、JDK 17(`C:\Android\jdk17`)、NDK 27.2(`-PcrawlNdkVersion=27.2.12479018`) |

> **罠: 生成ファイル `config.h` を Windows 版と取り合う。**`source/config.h`(`usleep`・`fdatasync`
> などがあるか)は、WSL の make では Linux 用(全部ある。Android の Bionic と同じ)に、Windows 版の
> MSBuild では Windows 用(全部無い)に作られる。同じ作業ツリーなので、Windows 版を組んだあとに
> Android 版を組むと、`syscalls.h` の代わりの `usleep` 宣言が Bionic のものとぶつかって止まる
> (arm64 で `call to 'usleep' is ambiguous`)。**Windows 版を組んだあとは `config.h` を消して
> WSL の make をやり直してから** Gradle で組む(逆に、Android のあとに Windows 版を組むときは
> MSBuild が作り直す)。

本家の設定との差(このフォークで変えたもの):

- `app/build.gradle.in`: Java のソース・バイトコードの版を 21 → 17(JDK 17 で組めるように)。
  `ndkVersion` を Gradle のプロパティ `crawlNdkVersion` で差し替えられるようにした(既定は本家の 28.2 のまま)。
- `jni/src/Android.mk`: `LOCAL_DISABLE_FORMAT_STRING_CHECKS := true`(文は `jtransc("...")` で
  辞書から引いて書式文字列として渡すので、NDK が既定で付ける `-Werror=format-security` で止まる)、
  `LOCAL_SHORT_COMMANDS := true`(Windows ではリンクのコマンド行が約 32K 文字の上限を超える)、
  `-DREGEX_PCRE` と libpcre(§2 の 8)。

Gradle の実行(PowerShell):

```powershell
$env:JAVA_HOME = "C:\Android\jdk17"; $env:ANDROID_HOME = "C:\Android\sdk"
# Android.mk の $(shell cat ...) のため、Git の usr\bin を PATH の末尾に足す
$env:PATH = "$env:JAVA_HOME\bin;$env:PATH;C:\Program Files\Git\usr\bin"
cd crawl-ref\source\android-project
gradle --no-daemon :app:assembleDebug -PcrawlNdkVersion=27.2.12479018 -Pandroid.injected.build.abi=x86_64
```

実機で試す APK(arm64-v8a。端末に直接入れられる通常のデバッグ版):

```powershell
gradle --no-daemon :app:assembleDebug -PcrawlNdkVersion=27.2.12479018 -PcrawlAbis=arm64-v8a
```

できる APK は `app/build/outputs/apk/debug/app-debug.apk`(デバッグ鍵で署名)。
`-PcrawlAbis` は日本語版で足したプロパティ(`app/build.gradle.in`。カンマ区切りで複数可。
外すと `jni/Application.mk` の `APP_ABI := all` で全部)。

`gradle` はキャッシュ済みの Gradle 8.13(`%USERPROFILE%\.gradle\wrapper\dists\gradle-8.13-bin\...\bin\gradle.bat`)。
`-Pandroid.injected.build.abi` はエミュレータ用に x86_64 だけを組むため(外すと全 ABI)。
できる APK は `app/build/intermediates/apk/debug/app-debug.apk`(テスト用の印が付くので
`adb install -r -t`)。init ファイルを作り直させるには `adb shell pm clear org.develz.crawl` と
`/sdcard/Android/data/org.develz.crawl/files` の削除。

## 4. 確かめ方

エミュレータ `hb35`(Android 15、google_apis、x86_64)で、起動画面、作られた init ファイル、
ゲームの表示(スタート画面・キャラクター作成・ゲーム画面)を見る。

## 5. 確認の記録(2026-10-19、エミュレータ hb35)

| # | 見たこと | 結果 |
|---|---|---|
| 1 | 起動画面(端末の言語は英語) | 英語のまま(values-ja は端末が日本語のときだけ) |
| 1b | アプリの言語を日本語に(`adb shell cmd locale set-app-locales org.develz.crawl --locales ja-JP`) | 起動画面が日本語(ゲームを始める・設定ファイルを編集・記録ファイル・改造データ・画面のキーボード など) |
| 2 | 起動画面が作った init ファイル | `language = ja` と `tile_font_*_file = BIZUDGothic-Regular.ttf` が 5 行 |
| 3 | ゲームのスタート画面 | 「… 0.34.1-ja1 へようこそ!」、ゲームモードの一覧と説明が日本語 |
| 4 | 名前入力 → 種族 → 職業 → 武器 | 種族名・職業名・説明文・操作の案内・武器の名前(適性+2 など)が日本語 |
| 5 | ゲーム画面 | メッセージ欄は日本語(「ようこそ、ミノタウロスの狂戦士『andtest』よ。」など)。ステータス欄の見出しは英語(§2 の 9) |

`database.cc` の変更(§2 の 7)は Windows 版にも入る。Windows 版は組めて、起動すると jtrans.db
(2.3MB)が作り直された。ただしこのときは、この PC で Crawl の窓が白いまま描かれない状態になっていて
(**変更を入れる前のリリース版 0.34.1-ja1 でも同じ**だったので、この変更が原因ではない)、画面での確認は
できていない。

確かめていないこと: 実機(arm64)、長く遊んだとき、
ヘルプ・説明画面などゲームの中の他の画面、セーブと読み込み。
