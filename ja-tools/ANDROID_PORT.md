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
| 3 | **フォント**: `settings/init.txt` は `C:/Windows/Fonts/BIZ-UDGothicR.ttc` を指す。Android には無く、そもそも使われない(§1-4)。フォントが見つからないとゲームは起動時に終了する(`fontwrapper-ft.cc`) | 起動画面が init ファイルを**新しく作るとき**に、日本語版の設定(`language = ja` と、端末に実在する日本語フォント)を書く。候補は `/system/fonts/NotoSansCJK-Regular.ttc` ほか。見つからなければ英語のまま(`language` を書かない)。フォントは同梱しない |
| 4 | 起動画面などの Android 側の文字列(36 個) | `res/values-ja/strings.xml` を足した。端末の言語が日本語なら日本語になる |
| 5 | 起動画面の「オプションの説明」 | 端末の言語が日本語なら `docs/ja/options_guide.txt` を開く |

## 3. この PC での組み方

本家は Linux で組む前提。この PC では、データの生成を WSL(Ubuntu)で、APK を Windows の
Android 環境(`C:\Android`。変愚の Android 移植で入れたもの)で組む。

| 段 | どこで | 何を |
|---|---|---|
| データ | WSL | `make -j16 ANDROID=1 android`(`/mnt/l/...` の作業ツリーのまま)。生成物はすべて Git の管理外 |
| `jni/` のリンク | Windows | このチェックアウトは `core.symlinks=false` なので、`jni/SDL2` などは「リンク先を書いたテキスト」になっている。ジャンクションに置き換え、`git update-index --skip-worktree` で変更として見せない(手元だけ。コミットしない) |
| `local.properties` | Windows | `sdk.dir=C\:/Android/sdk`(make が WSL の HOME で作った分を書き換える。管理外) |
| APK | Windows | Gradle 8.13(キャッシュ済み)、JDK 17(`C:\Android\jdk17`)、NDK 27.2(`-PcrawlNdkVersion=27.2.12479018`) |

本家の設定との差(このフォークで変えたもの):

- `app/build.gradle.in`: Java のソース・バイトコードの版を 21 → 17(JDK 17 で組めるように)。
  `ndkVersion` を Gradle のプロパティ `crawlNdkVersion` で差し替えられるようにした(既定は本家の 28.2 のまま)。

## 4. 確かめ方

エミュレータ `hb35`(Android 15、google_apis、x86_64。Noto Sans CJK が入っている)で、
起動画面が日本語になること、ゲームが日本語で表示されること(キャラクター作成・ゲーム画面・
ヘルプ)、作られた init ファイルの中身を見る。
