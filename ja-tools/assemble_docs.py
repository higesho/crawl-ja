#!/usr/bin/env python3
"""分担して訳したヘルプ文書(ja-work/docs-ja/parts/)を組み合わせ、
ゲームが読む crawl-ref/docs/ja/ に置く。適性表の前文は
crawl-ref/docs/template/ja/apt-preface.txt に置き、gen_apt_ja.py で適性表を作る。

使い方(リポジトリのルートで): python -I ja-tools/assemble_docs.py
原文(英語)の控えは ja-work/docs-en/ にある。本家の文書が変わったら、
控えとの差分を見て訳を直す。
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
PARTS = os.path.join(ROOT, "ja-work", "docs-ja", "parts")
DOCS = os.path.join(ROOT, "crawl-ref", "docs")

OUTPUTS = {
    "crawl_manual.txt": [f"manual-{i}.txt" for i in range(1, 9)],
    "options_guide.txt": [f"options-{i}.txt" for i in range(1, 7)],
    "quickstart.txt": ["quickstart.txt"],
    "macros_guide.txt": ["macros_guide.txt"],
    "tiles_help.txt": ["tiles_help.txt"],
    "changelog.txt": ["changelog.txt"],
}


def read_part(name):
    with open(os.path.join(PARTS, name), encoding="utf-8-sig") as f:
        return f.read().replace("\r\n", "\n").rstrip("\n") + "\n"


def main():
    os.makedirs(os.path.join(DOCS, "ja"), exist_ok=True)
    for out, parts in OUTPUTS.items():
        text = "".join(read_part(p) for p in parts)
        with open(os.path.join(DOCS, "ja", out), "w", encoding="utf-8",
                  newline="\n") as f:
            f.write(text)
        print(f"docs/ja/{out}: {text.count(chr(10))} lines")

    tmpl = os.path.join(DOCS, "template", "ja")
    os.makedirs(tmpl, exist_ok=True)
    with open(os.path.join(tmpl, "apt-preface.txt"), "w", encoding="utf-8",
              newline="\n") as f:
        f.write(read_part("apt-preface.txt"))

    check = subprocess.call(
        [sys.executable, "-I", os.path.join(HERE, "docwrap.py"), "--check"]
        + [os.path.join(DOCS, "ja", o) for o in OUTPUTS])
    gen = subprocess.call(
        [sys.executable, "-I", os.path.join(HERE, "gen_apt_ja.py")])
    return check or gen


if __name__ == "__main__":
    sys.exit(main())
