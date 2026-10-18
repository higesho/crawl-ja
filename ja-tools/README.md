# DCSS 0.34 日本語化(ja-0.34 ブランチ)

旧日本語版 [dplusplus/dcss-0.19-ja](https://github.com/dplusplus/dcss-0.19-ja)
(と 0.16-ja)の訳を土台に、0.34 安定版へ日本語化を移植する作業用ツールとメモ。

## 仕組み

- **説明文・DB テキスト**(`dat/descript/ja/`, `dat/database/ja/`)
  0.34 本体の翻訳機構をそのまま使う。オプション `language = ja` で読み込まれ、
  訳がない項目は英語で表示される。各訳文の上には `# en:` として
  訳した時点の英文をコメントで残している(本家の英文が変わったら差分で検出できる)。
- **C++ 内の文字列**
  0.19-ja と同じく `jtrans("英文")` で包み、`dat/database/ja/jtrans/*.txt` の
  辞書を引く(`database.cc`)。`language = ja` 以外では英文のまま返す。
  現時点では辞書を 0.19-ja から持ってきただけで、ソース側の張り直しはこれから。

## ツール

- `migrate_db.py` — 旧版の説明文・DB テキストの訳を 0.34 の英語データへ
  項目単位で移す。リポジトリのルートで `python -I ja-tools/migrate_db.py`。
  必要なリモート: `origin`(crawl/crawl), `ja19`, `ja16`(dplusplus の各版)。
  - 出力: `crawl-ref/source/dat/{descript,database}/ja/*.txt`
  - `ja-work/stale/` 英文が変わったため採用しなかった旧訳(再翻訳待ち)
  - `ja-work/missing/` 旧版に訳がない項目
  - `ja-work/review/` 0.34 公式 ja/ から採った訳(元の英文が不明なので要確認)
  - `ja-work/migrate_summary.json` ファイルごとの件数

## 遊ぶときの設定

Tiles 版はフォントを日本語対応のものに変える必要がある。例(Windows):

```
language = ja
tile_font_crt_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_msg_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_stat_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_tip_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
tile_font_lbl_file = C:/Windows/Fonts/BIZ-UDGothicR.ttc
```
