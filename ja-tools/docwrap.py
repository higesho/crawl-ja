#!/usr/bin/env python3
"""日本語訳したヘルプ文書(docs/ja/*.txt)を表示幅で折り返す・検査する。

ゲーム内のヘルプは折り返し済みのテキストとしてそのまま表示される
(command.cc の FS_PREWRAPPED_TEXT)ので、1 行を表示幅 WIDTH 桁以内に収める。
全角文字は 2 桁。訳は段落を 1 行に書いてよく、このツールが折り返す。

使い方:
  python -I ja-tools/docwrap.py <file>...           その場で折り返す
  python -I ja-tools/docwrap.py --check <file>...   幅超え・曖昧幅の文字を報告するだけ
  オプション --width N(既定 79)

折り返し:
- 幅を超える行だけを折る。続きの行は元の行の字下げ(先頭の空白)にそろえ、
  行頭が箇条書きの記号("* ", "- ", "1. " など)ならその分も下げる。
- 半角の空白か、全角文字の前後(禁則を守る)で折る。英単語の途中では折らない。
"""

import argparse
import re
import sys
import unicodedata

NO_LINE_START = set(
    "、。，．・：；？！ー…‥）〕］｝〉》」』】々゛゜"
    "ぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮヵヶ"
    ".,:;!?)]}'\"%")
NO_LINE_END = set("（〔［｛〈《「『【([{")

# 曖昧幅(East Asian Ambiguous)の文字は crawl では 1 桁と数えるが、日本語の
# フォントでは全角の字形になって重なることがある。訳文では使わない
# 例外: 原文にもある記号(• はボーブルの表示記号として options_guide に出る)
ALLOWED_AMBIGUOUS = {"•"}


def char_width(c):
    if unicodedata.combining(c):
        return 0
    return 2 if unicodedata.east_asian_width(c) in ("W", "F") else 1


def str_width(s):
    return sum(char_width(c) for c in s)


def is_wide(c):
    return unicodedata.east_asian_width(c) in ("W", "F")


def break_ok(prev, nxt):
    """prev と nxt の間(空白なし)で改行してよいか。"""
    if prev == " " or nxt == " ":
        return False
    if not (is_wide(prev) or is_wide(nxt)):
        return False
    if nxt in NO_LINE_START or prev in NO_LINE_END:
        return False
    return True


LIST_MARK = re.compile(r"^(\s*)((?:[*\-+]|\d+[.)]|[a-z][.)])\s+)")


def wrap_line(line, width):
    if str_width(line) <= width:
        return [line]
    m = LIST_MARK.match(line)
    if m:
        cont = " " * str_width(m.group(1) + m.group(2))
    else:
        cont = " " * (len(line) - len(line.lstrip(" ")))
    out = []
    rest = line
    first = True
    while str_width(rest) > width:
        # 幅に収まる最長の位置を探す
        w = 0
        limit = 0
        for i, c in enumerate(rest):
            w += char_width(c)
            if w > width:
                break
            limit = i + 1
        # limit 以前で折れる位置を後ろから探す
        indent_len = len(rest) - len(rest.lstrip(" "))
        cut = -1
        skip_space = False
        for i in range(limit, indent_len, -1):
            if i < len(rest) and rest[i] == " ":
                cut, skip_space = i, True
                break
            if 0 < i < len(rest) and break_ok(rest[i - 1], rest[i]):
                cut = i
                break
        if cut <= indent_len:
            # 折れる所がない(長い英単語・URL など)。そのまま出す
            break
        head = rest[:cut].rstrip(" ")
        tail = rest[cut + 1:] if skip_space else rest[cut:]
        out.append(head)
        rest = cont + tail.lstrip(" ")
        first = False
    out.append(rest)
    return out


def ambiguous_chars(line):
    return sorted({c for c in line
                   if unicodedata.east_asian_width(c) == "A"
                   and c not in ALLOWED_AMBIGUOUS})


def main():
    # Windows の端末(cp932)で表せない字があっても落ちないように
    sys.stdout.reconfigure(errors="replace")
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--width", type=int, default=79)
    args = ap.parse_args()

    bad = 0
    for fn in args.files:
        with open(fn, encoding="utf-8", newline="") as f:
            text = f.read()
        if text.startswith("﻿"):
            text = text[1:]
        lines = text.replace("\r\n", "\n").split("\n")
        out = []
        for no, line in enumerate(lines, 1):
            line = line.rstrip()
            amb = ambiguous_chars(line)
            if amb:
                bad += 1
                print(f"{fn}:{no}: 曖昧幅の文字 {' '.join(amb)}")
            if args.check:
                if str_width(line) > args.width:
                    bad += 1
                    print(f"{fn}:{no}: 幅 {str_width(line)} > {args.width}")
                out.append(line)
            else:
                wrapped = wrap_line(line, args.width)
                if len(wrapped) > 1 or str_width(wrapped[-1]) > args.width:
                    for w in wrapped:
                        if str_width(w) > args.width:
                            bad += 1
                            print(f"{fn}:{no}: 折れない行(幅 {str_width(w)})")
                out.extend(wrapped)
        if not args.check:
            with open(fn, "w", encoding="utf-8", newline="\n") as f:
                f.write("\n".join(out))
    if bad:
        print(f"問題 {bad} 件")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
