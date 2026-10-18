#!/usr/bin/env python3
"""日本語版の適性表 crawl-ref/docs/ja/aptitudes.txt を作る。

ビルドで生成される英語版 crawl-ref/docs/aptitudes.txt(util/gen-apt.pl)をもとに、
- 前文と技能の略語の凡例(`WL  - willpower` の行まで)を訳
  crawl-ref/docs/template/ja/apt-preface.txt に差し替え、
- 表の種族名を data-species.txt の訳([spec name])にする。
数値の列はそのまま。略語(Arm, Ddg ...)も英語のまま。

使い方(リポジトリのルートで): python -I ja-tools/gen_apt_ja.py
util/gen-all.py からも呼ばれる(英語版か訳が変わったとき)。
"""

import os
import sys
import unicodedata

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DOCS = os.path.join(ROOT, "crawl-ref", "docs")
EN = os.path.join(DOCS, "aptitudes.txt")
PREFACE = os.path.join(DOCS, "template", "ja", "apt-preface.txt")
OUT = os.path.join(DOCS, "ja", "aptitudes.txt")
SPECIES = os.path.join(ROOT, "crawl-ref", "source", "dat", "database", "ja",
                       "jtrans", "data-species.txt")

NAME_COLS = 17   # 表の種族名の欄の幅(英語版の見出し行の先頭の空白の数)
DRACONIAN = "Draconian"


def width(s):
    return sum(2 if unicodedata.east_asian_width(c) in ("W", "F") else 1
               for c in s)


def load_species():
    names = {}
    with open(SPECIES, encoding="utf-8") as f:
        text = f.read()
    for entry in text.split("%%%%"):
        lines = [l for l in entry.strip("\n").split("\n")
                 if not l.startswith("#")]
        if not lines or not lines[0].startswith("[spec name]"):
            continue
        vals = [l for l in lines[1:] if l.strip()]
        if vals:
            names[lines[0][len("[spec name]"):]] = vals[0].strip()
    return names


def pad(s, cols):
    return s + " " * max(cols - width(s), 0)


def main():
    names = load_species()
    with open(EN, encoding="utf-8") as f:
        en = f.read().split("\n")
    with open(PREFACE, encoding="utf-8") as f:
        preface = f.read().rstrip("\n").split("\n")

    end = next(i for i, l in enumerate(en) if l.startswith("WL  - "))
    out = preface[:]
    draco_ja = names[DRACONIAN]
    for line in en[end + 1:]:
        head, rest = line[:NAME_COLS], line[NAME_COLS:]
        name = head.strip()
        if not name or set(line) <= {"-"} or line.startswith(" " * NAME_COLS):
            out.append(line)
            continue
        if head.startswith(" "):
            # ドラコニアンの色の行(英語版は色だけを右寄せにしている)
            ja = names.get(f"{name} {DRACONIAN}")
            if ja is None:
                sys.exit(f"種族名の訳がない: {name} {DRACONIAN}")
            color = ja.replace(draco_ja, "")
            label = " " * max(width(draco_ja) - width(color), 0) + color
        else:
            label = names.get(name)
            if label is None:
                sys.exit(f"種族名の訳がない: {name}")
        if width(label) >= NAME_COLS:
            sys.exit(f"種族名が長すぎて表に収まらない: {label}")
        out.append(pad(label, NAME_COLS) + rest)

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
