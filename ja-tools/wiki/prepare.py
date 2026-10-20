# -*- coding: utf-8 -*-
r"""取ってきた記事を翻訳の前の形にし、その記事に出てくる用語の対応表を作る。

    python ja-tools\wiki\prepare.py "Vehumet" "Acid Ball"
    python ja-tools\wiki\prepare.py --all                 # 取得済みの全部

出すもの(work/ の下。Git の管理外):

    src/<名前>.html     翻訳の元(本文だけ。編集リンク・コメント・装飾を除いたもの)
    gloss/<名前>.tsv    その記事に出てくるゲームの用語の 英語<TAB>日本語(ゲームの訳語)
    images.txt          記事が使う画像の一覧(取得は fetch_wiki.py --images)

用語の対応はゲームの日本語版から取る。優先の順は、スポイラーの書き出し
(ja-tools/spoiler/work/spoiler.json の名前)→ jtrans の辞書の短い項目。
"""
from __future__ import annotations

import argparse
import html
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORK = HERE / "work"
RAW = WORK / "raw"
SRC = WORK / "src"
GLOSS = WORK / "gloss"
SPOILER_JSON = ROOT / "ja-tools" / "spoiler" / "work" / "spoiler.json"
JTRANS = ROOT / "crawl-ref" / "source" / "dat" / "database" / "ja" / "jtrans"

sys.path.insert(0, str(HERE))
from fetch_wiki import file_name  # noqa: E402


# ------------------------------------------------------------------ 用語
GENERAL_TAGS = {"spell", "dur", "feature", "ability", "spec name", "skill", "job",
                "cloud", "attack flavour", "item class", "form"}


def _add(g: dict, en: str, ja: str, prio: int) -> None:
    en = en.strip()
    ja = re.sub(r"^『(.*)』$", r"\1", ja.strip())
    if not en or not ja or en == ja or len(en) < 3:
        return
    if re.search(r"[%<>\n\\{}]", en) or re.search(r"[%<>\n\\]", ja):
        return
    if not re.search(r"[぀-ヿ一-鿿]", ja):
        return
    key = en.lower()
    if key not in g or g[key][1] > prio:
        g[key] = (ja, prio, en)


def _strip_title(t: str) -> tuple[str, str]:
    """「日本語 (English)」の見出しを分ける。"""
    t = re.sub(r"<[^>]+>", "", t).strip()
    m = re.match(r"^(.*?)\s*[\(（]([^()（）]*[A-Za-z][^()（）]*)[\)）]\s*$", t)
    return (m.group(1), m.group(2)) if m else (t, "")


def build_glossary() -> dict:
    g: dict = {}
    d = json.loads(SPOILER_JSON.read_text(encoding="utf-8"))
    lk = d["lookup"]
    for m in lk["monster"]:
        if "name_ja" in m:
            _add(g, m["name_en"], re.sub(r"^『(.*)』$", r"\1", m["name_ja"]), 0)
    for s in lk["spell"]:
        if "name_ja" in s:
            _add(g, s["name_en"], s["name_ja"], 0)
    for it in lk["item"]:
        _add(g, it["key"], it.get("name_ja", ""), 0)
    for gd in lk["god"]:
        _add(g, gd["key"], gd.get("name_ja", ""), 0)
    for s in d["species"]:
        _add(g, s["name_en"], s["name_ja"], 0)
    for j in d["jobs"]:
        _add(g, j["name_en"], j["name_ja"], 0)
    for s in d["skills"]:
        _add(g, s["en"], s["ja"], 0)
    # 支部は正式名(Lair → 獣の棲み処)。short_ja は画面の略称(獣)なので使わない
    for b in d["branches"]:
        _add(g, b["short_en"], b["long_ja"], 0)
    for bk in d["books"]:
        pass
    for typ in ("mutation", "status", "cloud", "feature", "ability", "card",
                "bane", "passive", "skill", "branch"):
        for e in lk.get(typ, []):
            ja, en = _strip_title(e["title"])
            key = re.sub(r"\s+(mutation|status|cloud|ability|card|bane|passive)$", "",
                         e["key"], flags=re.I)
            _add(g, en or key, ja, 1)
            # 地形のキーは「some deep water」「a fountain of …」の形なので冠詞を外しても引く
            bare = re.sub(r"^(a|an|the|some)\s+", "", key, flags=re.I)
            if bare != key:
                _add(g, bare, ja, 1)
    # jtrans の辞書の短い項目
    for f in sorted(JTRANS.glob("*.txt")):
        key = None
        val: list[str] = []

        def flush():
            if key is None:
                return
            v = "\n".join(val).strip()
            m = re.match(r"^\[([^\]]+)\]", key)
            k = key[m.end():].strip() if m else key.strip()
            if "\n" in v or len(k.split()) > 4 or len(v) > 30:
                return
            # 文脈の印の付いた項目は、その文脈でしか通じない訳が多い
            # ([counter class]weapon → 本、[branch]Lair → 獣 など)。
            # 一般的な意味の印だけを、優先度を下げて使う。
            if m:
                if m.group(1) in GENERAL_TAGS:
                    _add(g, k, v, 3)
                return
            _add(g, k, v, 2)

        for line in f.read_text(encoding="utf-8").split("\n"):
            if line.startswith("%%%%"):
                flush()
                key, val = None, []
            elif line.startswith("#"):
                continue
            elif key is None:
                key = line
            else:
                val.append(line)
        flush()
    return g


def page_glossary(g: dict, text: str) -> list[tuple[str, str]]:
    low = text.lower()
    hits = []
    for key, (ja, prio, en) in g.items():
        if key not in low:
            continue
        if re.search(r"(?<![a-z])" + re.escape(key) + r"(?:s|es)?(?![a-z])", low):
            hits.append((en, ja))
    hits.sort(key=lambda x: (-len(x[0]), x[0]))
    return hits


# ------------------------------------------------------------------ 本文
def clean(raw: str) -> tuple[str, list[str]]:
    a = raw.find('<div class="mw-parser-output">')
    b = raw.find("<!-- \nNewPP limit report")
    if b < 0:
        b = raw.find('<div class="printfooter">')
    body = raw[a + len('<div class="mw-parser-output">'):b]
    body = re.sub(r"<!--.*?-->", "", body, flags=re.S)
    # 先頭のバージョンの箱(こちらの注記に置き換える)
    body = re.sub(r'^\s*<div style="border: 1px solid #33FF66;.*?</div>', "", body, count=1, flags=re.S)
    body = re.sub(r'<span class="mw-editsection">.*?</span>\s*</span>', "", body, flags=re.S)
    body = re.sub(r'<span class="mw-editsection">.*?</span>', "", body, flags=re.S)
    # 白背景前提の装飾を外す(幅と寄せだけ残す)
    def keep_style(m):
        keep = [p for p in m.group(1).split(";")
                if re.match(r"\s*(width|text-align|vertical-align|white-space)\s*:", p)]
        return f' style="{";".join(keep)}"' if keep else ""
    body = re.sub(r'\sstyle="([^"]*)"', keep_style, body)
    body = re.sub(r'\s(bgcolor|border|cellpadding|cellspacing)="[^"]*"', "", body)
    images = re.findall(r'<img [^>]*src="(/images/[^"]+)"', body)
    body = re.sub(r"\n{3,}", "\n\n", body).strip()
    return body, images


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true")
    ap.add_argument("titles", nargs="*")
    args = ap.parse_args()
    SRC.mkdir(parents=True, exist_ok=True)
    GLOSS.mkdir(parents=True, exist_ok=True)
    files = ([p.name for p in sorted(RAW.glob("*.html"))] if args.all
             else [file_name(t) for t in args.titles])
    g = build_glossary()
    print(f"用語 {len(g)} 語")
    imgs_all: set[str] = set()
    for fn in files:
        raw = (RAW / fn).read_text(encoding="utf-8")
        body, imgs = clean(raw)
        imgs_all.update(imgs)
        (SRC / fn).write_text(body, encoding="utf-8")
        text = html.unescape(re.sub(r"<[^>]+>", " ", body))
        hits = page_glossary(g, text)
        (GLOSS / fn.replace(".html", ".tsv")).write_text(
            "".join(f"{en}\t{ja}\n" for en, ja in hits), encoding="utf-8")
        print(f"{fn}: 本文 {len(text)} 字、用語 {len(hits)} 語、画像 {len(imgs)} 枚")
    old = set((WORK / "images.txt").read_text(encoding="utf-8").split()) \
        if (WORK / "images.txt").exists() else set()
    (WORK / "images.txt").write_text("\n".join(sorted(old | imgs_all)) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
