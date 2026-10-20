# -*- coding: utf-8 -*-
r"""まだ訳していない記事を、エージェント 1 体ぶんずつの組に分ける。

    python ja-tools\wiki\batches.py             # 組を作って work/batches.json に書き、一覧を出す
    python ja-tools\wiki\batches.py --size 18000

前処理(prepare.py)を済ませた記事のうち、ja/ にまだ無いものを、本文の字数の合計が
--size 前後になるように詰める。割り振り済みの記事は work/assigned.txt に足していく
(assign で記録)。
"""
from __future__ import annotations

import argparse
import html
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE / "work" / "src"
JA = HERE / "ja"
ASSIGNED = HERE / "work" / "assigned.txt"


def text_len(p: Path) -> int:
    return len(html.unescape(re.sub(r"<[^>]+>", " ", p.read_text(encoding="utf-8"))))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--size", type=int, default=16000)
    ap.add_argument("--assign", nargs="*", help="割り振り済みとして記録する記事のファイル名")
    args = ap.parse_args()
    assigned = set(ASSIGNED.read_text(encoding="utf-8").split()) if ASSIGNED.exists() else set()
    if args.assign:
        assigned.update(args.assign)
        ASSIGNED.write_text("\n".join(sorted(assigned)) + "\n", encoding="utf-8")
        return
    todo = [(p.name, text_len(p)) for p in sorted(SRC.glob("*.html"))
            if not (JA / p.name).exists() and p.name not in assigned]
    todo.sort(key=lambda x: -x[1])
    batches: list[list] = []
    for name, n in todo:
        for b in batches:
            if sum(x[1] for x in b) + n <= args.size:
                b.append((name, n))
                break
        else:
            batches.append([(name, n)])
    (HERE / "work" / "batches.json").write_text(
        json.dumps([[x[0] for x in b] for b in batches], ensure_ascii=False, indent=1),
        encoding="utf-8")
    print(f"未訳 {len(todo)} 本、{len(batches)} 組")
    for i, b in enumerate(batches):
        print(i, sum(x[1] for x in b), " ".join(x[0] for x in b))


if __name__ == "__main__":
    main()
