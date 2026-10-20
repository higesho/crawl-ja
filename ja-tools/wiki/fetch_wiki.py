# -*- coding: utf-8 -*-
r"""英語版 wiki(CrawlWiki)の記事を取ってくる。

    python ja-tools\wiki\fetch_wiki.py --list             # 0.34 向けの記事名の一覧を作り直す
    python ja-tools\wiki\fetch_wiki.py "Vehumet" "Acid Ball"   # 記事を取る(取得済みは飛ばす)

wiki の robots.txt は API(/api.php)・/index.php・Special: を全部のボットに禁じているので、
普通の記事の URL(/記事名)とカテゴリーの URL だけを使う。1 本ごとに 5 秒空ける。
取ったものは work/raw/ に置く(Git の管理外)。
"""
from __future__ import annotations

import argparse
import html
import json
import re
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
RAW = HERE / "work" / "raw"
IMG = HERE / "work" / "img"
TITLES = HERE / "work" / "titles.json"
BASE = "http://crawl.chaosforge.org/"
UA = "crawl-ja-translation/0.1 (Japanese translation of CrawlWiki; https://github.com/higesho/crawl-ja)"
WAIT = 5.0
CATEGORY = "Category:0.34_articles"

_last = 0.0


def get_bytes(path: str) -> bytes:
    global _last
    wait = WAIT - (time.time() - _last)
    if wait > 0:
        time.sleep(wait)
    req = urllib.request.Request(BASE + path, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=60) as r:
        body = r.read()
    _last = time.time()
    return body


def get(path: str) -> str:
    return get_bytes(path).decode("utf-8")


def url_path(title: str) -> str:
    return urllib.parse.quote(title.replace(" ", "_"), safe=":/'(),!")


def file_name(title: str) -> str:
    return re.sub(r'[\\/:*?"<>|]', "_", title.replace(" ", "_")) + ".html"


def list_titles() -> list[str]:
    titles: list[str] = []
    path = CATEGORY
    while path:
        s = get(path)
        part = s[s.find('id="mw-pages"'):]
        if 'id="catlinks"' in part:
            part = part[:part.find('id="catlinks"')]
        titles += [html.unescape(t) for t in
                   re.findall(r'<li><a href="/[^"]+" title="([^"]+)"', part)]
        m = re.search(r'pagefrom=([^"#&]+)[^"]*"[^>]*>next page', part)
        # 次のページは /index.php を通さない短い形で取る
        path = f"{CATEGORY}?pagefrom={m.group(1)}" if m else ""
    titles = [t for t in dict.fromkeys(titles) if ":" not in t or t.startswith("Zot:")]
    return titles


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--images", action="store_true",
                    help="prepare.py が書いた work/images.txt の画像を取る")
    ap.add_argument("--all", action="store_true", help="titles.json の記事を全部取る")
    ap.add_argument("titles", nargs="*")
    args = ap.parse_args()
    if args.all:
        args.titles = json.loads(TITLES.read_text(encoding="utf-8"))
    RAW.mkdir(parents=True, exist_ok=True)
    if args.images:
        IMG.mkdir(parents=True, exist_ok=True)
        for path in (HERE / "work" / "images.txt").read_text(encoding="utf-8").split():
            out = IMG / urllib.parse.unquote(path.rsplit("/", 1)[1])
            if out.exists():
                continue
            out.write_bytes(get_bytes(path.lstrip("/")))
            print(f"画像: {out.name}")
    if args.list:
        t = list_titles()
        TITLES.write_text(json.dumps(t, ensure_ascii=False, indent=0), encoding="utf-8")
        print(f"{len(t)} 本")
    for title in args.titles:
        out = RAW / file_name(title)
        if out.exists():
            print(f"取得済み: {title}")
            continue
        out.write_text(get(url_path(title)), encoding="utf-8")
        print(f"取得: {title}")


if __name__ == "__main__":
    main()
