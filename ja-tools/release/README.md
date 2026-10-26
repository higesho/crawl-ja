# リリースの手順

1. 変更をすべてコミットし、`ja-tools/release/<版>.md`(リリースの案内文)を書いてコミットする。
2. 版のタグを付ける: `git tag -a 0.34.1-ja1 -m "..."`。ゲーム内の版表記は `git describe` から
   作られる(`util/gen_ver.pl`)ので、タグを付けてからビルドする。版の形は本家の正規表現に
   合わせ、`0.34.1-ja1` のように「本家の版-ja番号」にする。
3. Release Tiles / x64 でビルドする(STATUS.md の「ビルドと確認」)。
4. `python -I ja-tools/make_release.py 0.34.1-ja1 ../release` で zip を作る。
5. 実行ファイルの zip を空のフォルダに展開し、`crawl.exe` だけで日本語で始まること、版表記、
   ヘルプ、新しいゲームの開始を確かめる。
6. Android 版(arm64-v8a)の APK を `stone_soup-<版>-android-arm64.apk` の名前で同じフォルダに
   写す(ANDROID_PORT.md。Windows のビルドの後は config.h を消してから `make ANDROID=1 android`)。
7. `python -I ja-tools/release/publish_release.py <版> ../release --publish` で GitHub のリリースを
   作り(無ければ下書き)、3 つのファイルを上げて公開する。`--publish` を外すと下書きのまま止める。
   認証は git が push に使っているもの(`git credential fill`)で、値は表示しない。最後に添付の
   大きさと本文がリリース文と同じことを表示する(2026-10-27 の ja6 から。それまでは下書きを
   ブラウザで作り、添付と公開は手で行っていた)。

`README-ja.txt` は実行ファイルの zip に同梱する説明(BOM 付き UTF-8・CRLF に変えて入れる)。

リポジトリの説明文(GitHub の About 欄など)の例:

    Dungeon Crawl Stone Soup 0.34 の日本語版。0.19-ja の訳を土台に、0.34.1 へ移し直したもの。
