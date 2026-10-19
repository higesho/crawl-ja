# DCSS 日本語版のスポイラー

変愚蛮怒HDの紹介サイト(`higesho.github.io`)の `dcss/` に置く、日本語版のスポイラーを作る道具。
Sil-Q・Frox のスポイラーと並べるので、見た目(`assets/dcss.css`)はそれらに揃えてある。英語版は作らない。

## 作り方

1. Windows のタイル版(`crawl.exe`)を組む。タイルの画像シート(`rltiles/*.png`)もこのとき作られる。
2. 次を実行する。

```
python ja-tools\spoiler\build_spoiler.py                 # データを書き出してサイトを作る
python ja-tools\spoiler\build_spoiler.py --no-dump       # 書き出し済みのデータからサイトだけ作る
python ja-tools\spoiler\build_spoiler.py --out <フォルダ> # 出力先(既定は C:\Project2\higesho.github.io\dcss)
```

## 仕組み

- `crawl.exe -rc spoiler.rc -spoiler-json work/spoiler.json` が、初期化(辞書と説明文の読み込み)のあと、
  ゲーム内検索(`?/`)の全項目の説明を JSON に書いて終わる(`lookup-help.cc` の末尾)。
  説明の画面(`show_description`)を開く代わりに中身を受け取るので、文はゲームの画面と同じになる。
  モンスター・呪文・アイテム・神は、説明を作る関数を直接呼ぶ。
- 表に使う数値(モンスターの HD・AC など、種族の適性、職業の初期技能と装備、支部、出現表、魔法書)も
  同じ JSON に構造のまま書く。唯一モンスターの出る階は `dat/des/builder/uniques.des` の `DEPTH:` から取る。
- `build_spoiler.py` は JSON から HTML を作り、タイルを画像シートから切り出す。
  **数値も文もここでは書かない**。手で書くのは見出しと案内だけ。
- 文の色の印(`<lightred>…</lightred>`)は span に、画面の操作に向けた行(「説明を読むには…」、
  アイテムの検索用の接頭辞)は取り除く。
- `work/` は生成物で、Git の管理外。
