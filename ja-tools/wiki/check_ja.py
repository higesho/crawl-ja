# -*- coding: utf-8 -*-
r"""訳した記事を検査する。

    python ja-tools\wiki\check_ja.py            # ja/ の全部
    python ja-tools\wiki\check_ja.py Vehumet.html

見ること:
- タグの数(種類ごと)が原文と同じか(隣り合うリンクの入れ替えは許す)
- href と id の集まりが原文と同じか
- 文の部分に英語の語がたくさん残っていないか(略語・式・括弧で添えた原語は数えない)
"""
from __future__ import annotations

import collections
import html
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE / "work" / "src"
JA = HERE / "ja"

ALLOWED = {"HP", "MP", "AC", "EV", "SH", "XL", "XP", "HD", "Crawl", "DCSS", "ID", "LOS", "LoS",
           "CrawlWiki", "Wiki", "wiki", "GDR", "ER", "L", "x", "d", "rF", "rC", "rN", "rPois",
           "rElec", "rCorr", "Will", "Str", "Int", "Dex", "MR", "WebTiles", "Tiles", "Lua"}


def tags(s: str) -> collections.Counter:
    return collections.Counter(re.findall(r"<(/?[a-zA-Z0-9]+)", s))


def attrs(s: str, name: str) -> collections.Counter:
    return collections.Counter(re.findall(rf'\s{name}="([^"]*)"', s))


def english_words(s: str) -> list[str]:
    s = re.sub(r"<(code|pre|tt|math)[^>]*>.*?</\1>", " ", s, flags=re.S)
    text = html.unescape(re.sub(r"<[^>]+>", " ", s))
    # 括弧で添えた原語は数えない
    text = re.sub(r"[(（][^()（）]*[A-Za-z][^()（）]*[)）]", " ", text)
    words = re.findall(r"[A-Za-z][A-Za-z'\-]{2,}", text)
    return [w for w in words if w not in ALLOWED]


def check(name: str) -> list[str]:
    src = (SRC / name).read_text(encoding="utf-8")
    ja = (JA / name).read_text(encoding="utf-8")
    probs = []
    ts, tj = tags(src), tags(ja)
    if ts != tj:
        diff = {k: (ts[k], tj[k]) for k in set(ts) | set(tj) if ts[k] != tj[k]}
        probs.append(f"タグの数が違う: {diff}")
    for a in ("href", "id"):
        if attrs(src, a) != attrs(ja, a):
            probs.append(f"{a} が違う")
    eng = english_words(ja)
    if len(eng) > 25:
        top = collections.Counter(eng).most_common(8)
        probs.append(f"英語が多い({len(eng)} 語): {top}")
    return probs


def main() -> None:
    names = sys.argv[1:] or [p.name for p in sorted(JA.glob("*.html"))]
    bad = 0
    for n in names:
        probs = check(n)
        if probs:
            bad += 1
            print(n)
            for p in probs:
                print("   ", p)
    print(f"検査 {len(names)} 本、問題あり {bad} 本")


if __name__ == "__main__":
    main()
