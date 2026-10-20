# -*- coding: utf-8 -*-
r"""訳した wiki の記事を、紹介サイトの dcss/wiki/ のページにする。

    python ja-tools\wiki\build_wiki.py              # 既定の出力先(C:\Project2\higesho.github.io\dcss\wiki)
    python ja-tools\wiki\build_wiki.py --out DIR

読むもの(work/ の下):
    ja/<名前>.html     訳した本文(TRANSLATING.md の決まりで訳したもの)
    raw/<名前>.html    取ってきた元の記事(最終更新日を取る)
    img/               記事の画像(fetch_wiki.py --images)
    ../titles_ja.tsv   記事名の訳(英語<TAB>日本語)。無い記事は用語の表、それも無ければ英語のまま

記事は GNU FDL 1.3 で公開する(元の wiki が GNU FDL で、版を指定していないため)。
各ページに、元の記事・編集履歴へのリンクと、翻訳であることを書く。
"""
from __future__ import annotations

import argparse
import datetime
import html
import re
import shutil
import sys
import urllib.parse
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORK = HERE / "work"
JA = WORK / "ja"
RAW = WORK / "raw"
IMG = WORK / "img"
OUT = Path(r"C:\Project2\higesho.github.io\dcss\wiki")
BASE = "http://crawl.chaosforge.org/"

sys.path.insert(0, str(HERE))
from fetch_wiki import file_name, url_path  # noqa: E402
from prepare import build_glossary  # noqa: E402

NAV = [("../index.html", "入口"), ("../monster.html", "モンスター"),
       ("../spell.html", "呪文"), ("../item.html", "アイテム"),
       ("../artefact.html", "固定アーティファクト"), ("../species.html", "種族"),
       ("../job.html", "職業"), ("../god.html", "神"), ("../branch.html", "ダンジョン"),
       ("../mutation.html", "突然変異"), ("../misc.html", "事典"),
       ("index.html", "wiki 翻訳")]

MONTHS = {m: i for i, m in enumerate(
    ["January", "February", "March", "April", "May", "June", "July", "August",
     "September", "October", "November", "December"], 1)}


def esc(s: str) -> str:
    return html.escape(s, quote=True)


def page(title: str, desc: str, body: str, here: str) -> str:
    nav = "".join(f'<a href="{h}"{" class=here" if h == here else ""}>{t}</a>'
                  for h, t in NAV)
    return f"""<!doctype html>
<html lang="ja">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)} &mdash; DCSS wiki 翻訳</title>
<meta name="description" content="{esc(desc)}">
<link rel="stylesheet" href="../dcss.css">
</head>
<body>
<header class="top"><div class="band">
  <a class="brand" href="../index.html">DCSS スポイラー</a>
  <nav>{nav}</nav>
</div></header>
<main class="band">
{body}
</main>
<footer class="foot"><div class="band">
  <p>このページは英語版 CrawlWiki(<a href="{BASE}">crawl.chaosforge.org</a>)の記事を日本語に訳したもので、\
<a href="gfdl.html">GNU Free Documentation License 1.3</a> で公開しています。\
ゲームの用語は DCSS 日本語版の訳語に合わせました。</p>
</div></footer>
</body>
</html>
"""


def last_edited(raw: str) -> str:
    m = re.search(r"This page was last edited on (\d+) (\w+) (\d{4})", raw)
    if not m:
        return ""
    return f"{m.group(3)}-{MONTHS[m.group(2)]:02d}-{int(m.group(1)):02d}"


def load_titles() -> dict[str, str]:
    t: dict[str, str] = {}
    p = HERE / "titles_ja.tsv"
    if p.exists():
        for line in p.read_text(encoding="utf-8").splitlines():
            if "\t" in line:
                en, ja = line.split("\t", 1)
                t[en] = ja
    return t


def rewrite(body: str, done: dict[str, str], used_imgs: set[str]) -> str:
    def href(m):
        target = m.group(1)
        if target.startswith("#"):
            return m.group(0)
        path, _, frag = target.partition("#")
        title = urllib.parse.unquote(path.lstrip("/")).replace("_", " ")
        frag = f"#{frag}" if frag else ""
        if title in done:
            return f'href="{done[title]}{frag}"'
        return f'href="{BASE}{path.lstrip("/")}{frag}"'
    body = re.sub(r'href="(/[^"]*|#[^"]*)"', href, body)

    def src(m):
        name = urllib.parse.unquote(m.group(1).rsplit("/", 1)[1])
        used_imgs.add(name)
        return f'src="img/{name}"'
    body = re.sub(r'src="(/images/[^"]+)"', src, body)
    body = re.sub(r'\ssrcset="[^"]*"', "", body)
    return body


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, default=OUT)
    args = ap.parse_args()
    out: Path = args.out
    out.mkdir(parents=True, exist_ok=True)

    titles_ja = load_titles()
    gloss = build_glossary()
    files = sorted(JA.glob("*.html"))
    # 英語の記事名 → 出力のファイル名
    done: dict[str, str] = {}
    meta = []
    for f in files:
        raw = (RAW / f.name).read_text(encoding="utf-8")
        m = re.search(r'<h1 id="firstHeading"[^>]*>(.*?)</h1>', raw, re.S)
        en = html.unescape(re.sub(r"<[^>]+>", "", m.group(1))).strip() if m else f.stem
        ja = titles_ja.get(en) or (gloss.get(en.lower(), ("",))[0]) or en
        done[en] = f.name
        meta.append((en, ja, f, last_edited(raw)))

    used_imgs: set[str] = set()
    rows = []
    for en, ja, f, edited in sorted(meta, key=lambda x: x[1]):
        body = rewrite(f.read_text(encoding="utf-8"), done, used_imgs)
        orig = BASE + url_path(en)
        hist = f"{BASE}index.php?title={url_path(en)}&amp;action=history"
        note = (f'<p class="wnote">英語版 CrawlWiki の記事「<a href="{orig}">{esc(en)}</a>」'
                f'(0.34 向け{f"、{edited} 版" if edited else ""})の翻訳です。'
                f'原文の著者は<a href="{hist}">編集履歴</a>をご覧ください。</p>')
        sub = f' <span class="en">{esc(en)}</span>' if ja != en else ""
        text = f"""<h1>{esc(ja)}{sub}</h1>
{note}
<div class="wiki">
{body}
</div>"""
        (out / f.name).write_text(page(ja, f"{ja}({en})の日本語訳。", text, ""),
                                  encoding="utf-8", newline="\n")
        rows.append(f'<li><a href="{f.name}">{esc(ja)}</a>'
                    + (f' <span class="en">{esc(en)}</span>' if ja != en else "") + "</li>")

    (out / "img").mkdir(exist_ok=True)
    for name in used_imgs:
        if (IMG / name).exists():
            shutil.copyfile(IMG / name, out / "img" / name)

    index = f"""<h1>wiki 翻訳</h1>
<p class="lead">英語版の DCSS wiki(CrawlWiki)のうち、最新版 0.34 向けの記事を日本語に訳しています。
ゲームの用語は日本語版の訳語に合わせました。少しずつ増やしていきます(いま {len(rows)} 本)。</p>
<ul class="wlist">
{"".join(rows)}
</ul>"""
    (out / "index.html").write_text(
        page("wiki 翻訳", "英語版 DCSS wiki の 0.34 向け記事の日本語訳。", index, "index.html"),
        encoding="utf-8", newline="\n")

    fdl = (HERE / "assets" / "fdl-1.3.txt").read_text(encoding="utf-8")
    lic = f"""<h1>GNU Free Documentation License 1.3</h1>
<p class="lead">wiki 翻訳の記事はこのライセンスで公開しています。元の CrawlWiki の記事が
GNU Free Documentation License で公開されていて、版を指定していないため、1.3 を選びました。</p>
<pre class="license">{esc(fdl)}</pre>"""
    (out / "gfdl.html").write_text(
        page("GNU FDL 1.3", "wiki 翻訳のライセンス。", lic, ""), encoding="utf-8", newline="\n")
    print(f"{out}: 記事 {len(rows)} 本、画像 {len(used_imgs)} 枚")


if __name__ == "__main__":
    main()
