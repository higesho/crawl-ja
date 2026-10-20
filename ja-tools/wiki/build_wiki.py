# -*- coding: utf-8 -*-
r"""訳した wiki の記事を、紹介サイトの dcss/wiki/ のページにする。

    python ja-tools\wiki\build_wiki.py              # 既定の出力先(C:\Project2\higesho.github.io\dcss\wiki)
    python ja-tools\wiki\build_wiki.py --out DIR

読むもの(work/ の下):
    ../ja/<名前>.html  訳した本文(TRANSLATING.md の決まりで訳したもの)
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
JA = HERE / "ja"
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
       ("../mutation.html", "変異"), ("../misc.html", "事典"),
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


LATEST = "0.34"


def article_version(raw: str) -> str:
    """元の記事の先頭の版の箱(「Version 0.33: …」)から版を取る。"""
    m = re.search(r"<b>Version <a [^>]*>(\d+\.\d+)</a></b>", raw) or \
        re.search(r"Version (\d+\.\d+)\s*</", raw)
    return m.group(1) if m else ""


def _ver_key(v: str):
    try:
        return tuple(int(x) for x in v.split("."))
    except ValueError:
        return (0,)


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
    # 訳した人(エージェント)が記事ごとに書く題の訳(titles/<ファイル名の幹>.txt)。
    # 1 行目が英語の題、2 行目が訳。titles_ja.tsv が優先
    for f in sorted((HERE / "titles").glob("*.txt")):
        lines = f.read_text(encoding="utf-8").strip().splitlines()
        if len(lines) >= 2 and lines[0] not in t:
            t[lines[0].strip()] = lines[1].strip()
    return t


SRC = WORK / "src"


def _en_sentences(text: str) -> list[str]:
    text = html.unescape(re.sub(r"<[^>]+>", "\n", text))
    return [s.strip() for s in text.split("\n")
            if len(s.strip()) >= 30 and len(re.findall(r"[a-z]+ ", s)) >= 4]


def unfinished(f: Path) -> bool:
    """訳しかけの記事か。エージェントは原文を写してから訳していくので、
    原文の英文が半分以上そのまま残っていれば、まだ訳している途中とみなす。"""
    src = SRC / f.name
    if not src.exists():
        return False
    sents = _en_sentences(src.read_text(encoding="utf-8"))
    if not sents:
        return False
    # 属性(title など)は英語のまま残す決まりなので、表示される文字だけで比べる
    ja = "\n".join(_en_sentences(f.read_text(encoding="utf-8")))
    return sum(1 for s in sents if s in ja) * 2 >= len(sents)


ALIASES: dict[str, str] = {}
if (HERE / "aliases.tsv").exists():
    for _line in (HERE / "aliases.tsv").read_text(encoding="utf-8").splitlines():
        if "\t" in _line:
            _k, _v = _line.split("\t", 1)
            ALIASES[_k] = _v


def _find_done(title: str, done: dict[str, str]) -> str:
    """訳済みの記事を引く。wiki のリンクは転送用の別名(大文字小文字・複数形の違い)が
    多いので、それも試す。depth.py が記録した別名(aliases.tsv)も引く。"""
    title = ALIASES.get(title, title)
    if title in done:
        return done[title]
    low = {k.lower(): v for k, v in done.items()}
    t = title.lower()
    for cand in (t, t[:-1] if t.endswith("s") else "", t[:-2] if t.endswith("es") else "",
                 t[:-3] + "y" if t.endswith("ies") else ""):
        if cand and cand in low:
            return low[cand]
    return ""


def rewrite(body: str, done: dict[str, str], used_imgs: set[str]) -> str:
    def href(m):
        target = m.group(1)
        if target.startswith("#"):
            return m.group(0)
        path, _, frag = target.partition("#")
        title = urllib.parse.unquote(path.lstrip("/")).replace("_", " ")
        frag = f"#{frag}" if frag else ""
        hit = _find_done(title, done)
        if hit:
            return f'href="{hit}{frag}"'
        # 未訳の記事は英語版へ。印(data-untr)を付けて見分けられるようにする
        return f'href="{BASE}{path.lstrip("/")}{frag}" data-untr="1"'
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
    skipped = [f for f in files if unfinished(f)]
    files = [f for f in files if f not in skipped]
    if skipped:
        print("訳しかけなので飛ばす:", " ".join(f.stem for f in skipped))
    # 英語の記事名 → 出力のファイル名
    done: dict[str, str] = {}
    meta = []
    for f in files:
        raw = (RAW / f.name).read_text(encoding="utf-8")
        m = re.search(r'<h1 id="firstHeading"[^>]*>(.*?)</h1>', raw, re.S)
        en = html.unescape(re.sub(r"<[^>]+>", "", m.group(1))).strip() if m else f.stem
        ja = titles_ja.get(en) or (gloss.get(en.lower(), ("",))[0])
        if not ja:
            # 「Bane of the X」は災厄の名前(用語表では X → 「Xの災厄」)、
            # 「名前 (補足)」は補足を外して引く
            m2 = re.match(r"Bane of (?:the )?(.+)$", en, re.I)
            key = m2.group(1) if m2 else re.sub(r"\s*\([^)]*\)$", "", en)
            ja = gloss.get(key.lower(), ("",))[0] or en
        # トップページ(Main Page)は記事としてではなく、wiki 翻訳の入口にする
        done[en] = "index.html" if en == "Main Page" else f.name
        meta.append((en, ja, f, last_edited(raw), article_version(raw)))

    used_imgs: set[str] = set()
    rows: dict[str, list[str]] = {}
    main_body = ""
    for en, ja, f, edited, ver in sorted(meta, key=lambda x: x[1]):
        body = rewrite(f.read_text(encoding="utf-8"), done, used_imgs)
        if en == "Main Page":
            main_body = body
            continue
        orig = BASE + url_path(en)
        hist = f"{BASE}index.php?title={url_path(en)}&amp;action=history"
        vtext = f"{ver} 向け" if ver else "対象の版の記載なし"
        note = (f'<p class="wnote">英語版 CrawlWiki の記事「<a href="{orig}">{esc(en)}</a>」'
                f'({vtext}{f"、{edited} 版" if edited else ""})の翻訳です。'
                f'原文の著者は<a href="{hist}">編集履歴</a>をご覧ください。')
        if ver != LATEST:
            note += (f'<br><b>この記事は {ver or "古い版"} の時点の内容で、'
                     f'最新の {LATEST} とは違うところがあるかもしれません。</b>')
        note += "</p>"
        sub = f' <span class="en">{esc(en)}</span>' if ja != en else ""
        text = f"""<h1>{esc(ja)}{sub}</h1>
{note}
<div class="wiki">
{body}
</div>"""
        (out / f.name).write_text(page(ja, f"{ja}({en})の日本語訳。", text, ""),
                                  encoding="utf-8", newline="\n")
        rows.setdefault(ver or "?", []).append(
            f'<li><a href="{f.name}">{esc(ja)}</a>'
            + (f' <span class="en">{esc(en)}</span>' if ja != en else "") + "</li>")

    (out / "img").mkdir(exist_ok=True)
    for name in used_imgs:
        if (IMG / name).exists():
            shutil.copyfile(IMG / name, out / "img" / name)

    total = sum(len(v) for v in rows.values())
    secs = []
    for ver in sorted(rows, key=_ver_key, reverse=True):
        head = f"{ver} 向けの記事" if ver != "?" else "対象の版の記載が無い記事"
        if ver == LATEST:
            head += "(最新)"
        secs.append(f'<h2>{head}<span class="n">{len(rows[ver])}</span></h2>\n'
                    f'<ul class="wlist">\n{"".join(rows[ver])}\n</ul>')
    allpage = f"""<h1>記事一覧</h1>
<p class="lead">訳した記事の全部(いま {total} 本)。元の wiki で付いている対象の版ごとに分けています。
古い版の記事も、仕組みの説明などは今でも通用することが多いので載せていますが、最新の {LATEST} とは違うところがあるかもしれません。</p>
{"".join(secs)}"""
    (out / "all.html").write_text(
        page("記事一覧", "英語版 DCSS wiki の訳した記事の一覧。", allpage, "all.html"),
        encoding="utf-8", newline="\n")

    # 入口は訳したトップページ。まだ無ければ記事一覧を入口にする
    lead = f"""<p class="lead">英語版の DCSS wiki(CrawlWiki)を日本語に訳しています。
ゲームの用語は日本語版の訳語に合わせました。下は元の wiki のトップページの訳で、
訳した記事は日本語のページに、まだ訳していない記事は英語版<span class="untr-mark">(英)</span>につながります。
<a href="all.html">訳した記事の一覧</a>もあります。</p>"""
    if main_body:
        index = f"""<h1>wiki 翻訳</h1>
{lead}
<div class="wiki main">
{main_body}
</div>"""
    else:
        index = allpage
    (out / "index.html").write_text(
        page("wiki 翻訳", "英語版 DCSS wiki の日本語訳。", index, "index.html"),
        encoding="utf-8", newline="\n")

    fdl = (HERE / "assets" / "fdl-1.3.txt").read_text(encoding="utf-8")
    lic = f"""<h1>GNU Free Documentation License 1.3</h1>
<p class="lead">wiki 翻訳の記事はこのライセンスで公開しています。元の CrawlWiki の記事が
GNU Free Documentation License で公開されていて、版を指定していないため、1.3 を選びました。</p>
<pre class="license">{esc(fdl)}</pre>"""
    (out / "gfdl.html").write_text(
        page("GNU FDL 1.3", "wiki 翻訳のライセンス。", lic, ""), encoding="utf-8", newline="\n")
    print(f"{out}: 記事 {total} 本、画像 {len(used_imgs)} 枚")


if __name__ == "__main__":
    main()
