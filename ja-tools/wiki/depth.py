# -*- coding: utf-8 -*-
r"""トップページ(Main Page)を第 0 階層として、第 2 階層までの記事を洗い出し、足りない記事を取ってくる。

    python ja-tools\wiki\depth.py            # 洗い出して、無い記事を取る
    python ja-tools\wiki\depth.py --dry-run  # 取らずに数えるだけ

トップページの上部にある版のページ(0.1〜0.35)へのリンクは第 1 階層までにする
(版のページは訳すが、そこから先はたどらない)。

wiki のリンクは転送用の別名(Skills → Skill など)を指すことが多い。取ってきた記事の
見出し(firstHeading)が正式な題なので、別名 → 正式な題の対応を aliases.tsv に残し、
同じ記事を二度置かないようにする。build_wiki.py もこの対応でリンクを付け替える。

出すもの: work/depth.tsv(ファイル名<TAB>階層)
"""
from __future__ import annotations

import argparse
import html
import re
import sys
import urllib.parse
from pathlib import Path

HERE = Path(__file__).resolve().parent
RAW = HERE / "work" / "raw"
ALIASES = HERE / "aliases.tsv"
OUT = HERE / "work" / "depth.tsv"

sys.path.insert(0, str(HERE))
from fetch_wiki import file_name, get, url_path  # noqa: E402

VERSION = re.compile(r"^\d+\.\d+$")
NAMESPACES = {"File", "Image", "Category", "Help", "Special", "Template", "User", "User talk",
              "Talk", "CrawlWiki", "MediaWiki", "Media"}


def heading(raw: str) -> str:
    m = re.search(r'<h1 id="firstHeading"[^>]*>(.*?)</h1>', raw, re.S)
    return html.unescape(re.sub(r"<[^>]+>", "", m.group(1))).strip() if m else ""


def body_links(raw: str) -> list[str]:
    a = raw.find('<div class="mw-parser-output">')
    b = raw.find('<div class="printfooter">')
    body = raw[a:b] if a >= 0 else ""
    out = []
    for h in re.findall(r'<a href="/([^"#?]+)(?:#[^"]*)?"', body):
        t = urllib.parse.unquote(html.unescape(h)).replace("_", " ").strip()
        # 名前空間つき(Category: File: Help: など)は記事ではないので外す。
        # 題に「:」を含む普通の記事(攻略ガイドなど)は残す
        if not t or t.startswith("index.php") or t.split(":", 1)[0] in NAMESPACES:
            continue
        out.append(t)
    return list(dict.fromkeys(out))


class Index:
    """取得済みの記事の、正式な題 → ファイル名。"""

    def __init__(self) -> None:
        self.files: dict[str, str] = {}
        for f in RAW.glob("*.html"):
            h = heading(f.read_text(encoding="utf-8"))
            if h:
                self.files.setdefault(h, f.name)
        self.low = {k.lower(): v for k, v in self.files.items()}
        self.alias: dict[str, str] = {}
        if ALIASES.exists():
            for line in ALIASES.read_text(encoding="utf-8").splitlines():
                if "\t" in line:
                    k, v = line.split("\t", 1)
                    self.alias[k] = v

    def find(self, title: str) -> str:
        title = self.alias.get(title, title)
        if title in self.files:
            return self.files[title]
        t = title.lower()
        for cand in (t, t[:-1] if t.endswith("s") else "", t[:-2] if t.endswith("es") else "",
                     t[:-3] + "y" if t.endswith("ies") else ""):
            if cand and cand in self.low:
                return self.low[cand]
        return ""

    def fetch(self, title: str) -> str:
        """記事を取って置き、ファイル名を返す。転送なら正式な題で置き、別名を記録する。"""
        raw = get(url_path(title))
        h = heading(raw) or title
        if h != title:
            self.alias[title] = h
            with ALIASES.open("a", encoding="utf-8") as fp:
                fp.write(f"{title}\t{h}\n")
        if h in self.files:
            print(f"別名: {title} → {h}(取得済み)")
            return self.files[h]
        name = file_name(h)
        (RAW / name).write_text(raw, encoding="utf-8")
        self.files[h] = name
        self.low[h.lower()] = name
        print(f"取得: {title}" + (f" → {h}" if h != title else ""), flush=True)
        return name


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--closure", action="store_true",
                    help="階層を問わず、取得済みの全記事からたどれる記事を全部取る")
    args = ap.parse_args()
    idx = Index()
    depth: dict[str, int] = {}
    missing: list[tuple[str, int]] = []

    def visit(title: str, d: int) -> str:
        f = idx.find(title)
        if not f:
            if args.dry_run:
                missing.append((title, d))
                return ""
            try:
                f = idx.fetch(title)
            except Exception as e:  # noqa: BLE001
                print(f"失敗: {title}: {e}", flush=True)
                return ""
        if f not in depth or depth[f] > d:
            depth[f] = d
        return f

    if args.closure:
        # 取得済みの全記事のリンクをたどり、たどれる記事がなくなるまで取る
        queue = sorted(p.name for p in RAW.glob("*.html"))
        seen = set(queue)
        missing_titles: set[str] = set()
        while queue:
            f = queue.pop(0)
            for t in body_links((RAW / f).read_text(encoding="utf-8")):
                if args.dry_run:
                    if not idx.find(t):
                        missing_titles.add(idx.alias.get(t, t))
                    continue
                g = visit(t, 3)
                if g and g not in seen:
                    seen.add(g)
                    queue.append(g)
        if args.dry_run:
            print(f"未取得 {len(missing_titles)} 本:", ", ".join(sorted(missing_titles)[:120]))
        else:
            print(f"取得済み {len(seen)} 本")
        return

    main_file = visit("Main Page", 0)
    level1 = body_links((RAW / main_file).read_text(encoding="utf-8"))
    expand = []
    for t in level1:
        f = visit(t, 1)
        if f and not VERSION.match(t):
            expand.append(f)
    for f in expand:
        for t in body_links((RAW / f).read_text(encoding="utf-8")):
            visit(t, 2)

    OUT.write_text("".join(f"{f}\t{d}\n" for f, d in sorted(depth.items(), key=lambda x: (x[1], x[0]))),
                   encoding="utf-8")
    n = [sum(1 for d in depth.values() if d == k) for k in range(3)]
    print(f"第0階層 {n[0]} 本、第1階層 {n[1]} 本、第2階層 {n[2]} 本(合計 {len(depth)} 本)")
    if args.dry_run:
        print(f"未取得 {len(missing)} 本:", ", ".join(f"{t}({d})" for t, d in missing[:80]))


if __name__ == "__main__":
    main()
