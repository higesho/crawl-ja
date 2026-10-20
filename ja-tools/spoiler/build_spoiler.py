# -*- coding: utf-8 -*-
r"""DCSS 日本語版のスポイラー(静的サイト)を作る。

    python ja-tools\spoiler\build_spoiler.py              # データを書き出してサイトを作る
    python ja-tools\spoiler\build_spoiler.py --no-dump    # 書き出し済みのデータからサイトだけ作る
    python ja-tools\spoiler\build_spoiler.py --out DIR    # 出力先(既定は下の OUT)

流れ:

1. 組んだ crawl.exe(タイル版)に `-spoiler-json` を付けて走らせ、ゲーム内検索(?/)の
   全項目の説明と、表に使う数値を `work/spoiler.json` に書き出させる
   (`lookup-help.cc` の末尾。文はゲームの説明画面と同じ日本語)。
2. その JSON から HTML を作る。**数値も文もここでは書かない**——全部ゲームから取る。
   手で書くのは見出し・案内と、ページの組み立てだけ。
3. タイルは `rltiles/*.png`(ビルドで作られる画像シート)から切り出す。DCSS のタイルは CC0。

見た目は変愚蛮怒HDの紹介サイトの Sil-Q スポイラー(silq.css)に揃えてある。
英語版は作らない。
"""
from __future__ import annotations

import argparse
import html
import json
import re
import shutil
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]                      # crawl/
SRC = ROOT / "crawl-ref" / "source"
WORK = HERE / "work"
JSON_PATH = WORK / "spoiler.json"
OUT = Path(r"C:\Project2\higesho.github.io\dcss")
ASSETS = HERE / "assets"

UPSTREAM = "https://github.com/crawl/crawl"


# ------------------------------------------------------------------ 書き出し
def dump() -> None:
    """crawl.exe にデータを書き出させる。"""
    WORK.mkdir(exist_ok=True)
    exe = SRC / "crawl.exe"
    if not exe.exists():
        sys.exit(f"{exe} が無い。先に Windows のタイル版を組むこと")
    cmd = [str(exe), "-rc", str(HERE / "spoiler.rc"),
           "-spoiler-json", str(JSON_PATH)]
    r = subprocess.run(cmd, cwd=SRC, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if r.stderr.strip():
        print(r.stderr.strip()[:3000])
    if r.returncode != 0 or not JSON_PATH.exists():
        sys.exit(f"書き出しに失敗した(終了コード {r.returncode})")


# ------------------------------------------------------------------ 色の印
# ゲームの文の <lightred>…</lightred> などを span にする(format.cc と同じ解釈)。
COLOURS = {
    "black": "#555", "blue": "#5c7cff", "green": "#3fae49", "cyan": "#2ab3b3",
    "red": "#d24b4b", "magenta": "#b24bc0", "brown": "#b8862e",
    "lightgrey": "", "lightgray": "", "darkgrey": "#7c8494", "darkgray": "#7c8494",
    "lightblue": "#8fa6ff", "lightgreen": "#7fdc7f", "lightcyan": "#7fe3e3",
    "lightred": "#ff8a80", "lightmagenta": "#f08cff", "yellow": "#f2d64b",
    "white": "#ffffff", "w": "#ffffff", "h": "#f2d64b",
}


def fmt(s: str) -> str:
    """ゲームの色の印付きの文を HTML にする。"""
    out: list[str] = []
    stack: list[str] = []
    i, n = 0, len(s)
    while i < n:
        c = s[i]
        if c == "<" and i + 1 < n:
            if s[i + 1] == "<":
                out.append("&lt;")
                i += 2
                continue
            j = s.find(">", i + 1)
            tag = s[i + 1:j] if j > 0 else ""
            name = tag.lstrip("/")
            if j > 0 and tag and tag != "/" and (name in COLOURS
                                                 or name.startswith("bg:")):
                if tag.startswith("/"):
                    if stack:
                        out.append(stack.pop())
                else:
                    col = "" if name.startswith("bg:") else COLOURS[name]
                    out.append(f'<span style="color:{col}">' if col else "<span>")
                    stack.append("</span>")
                i = j + 1
                continue
        out.append(html.escape(c, quote=False))
        i += 1
    out.extend(reversed(stack))
    return "".join(out)


def jp(s: str) -> str:
    """手書きの日本語の案内の、ソースの折り返し(改行と字下げ)を詰める。"""
    return re.sub(r"\n\s*", "", s)


def esc(s: str) -> str:
    return html.escape(s, quote=True)


def plain(s: str) -> str:
    """色の印を除いた素の文(絞り込み用)。"""
    s = re.sub(r"<(?!<)/?[a-z:]+>", "", s)
    return s.replace("<<", "<")


# ゲームの画面の操作に向けた行で、ここでは意味の無いもの(素の文の頭で見る)
DROP_LINES = ("アイテム検索用の接頭辞", "メニュー・色分け用の接頭辞", "説明を読むには")


def clean(s: str) -> str:
    """ゲームの文から、画面の操作に向けた行や余分な空行を除く。"""
    lines = []
    for line in s.split("\n"):
        if plain(line).strip().startswith(DROP_LINES):
            continue
        # 呪文の一覧の選択キー(" a - 魔法の矢")は押せないので「・」にする
        line = re.sub(r"(^|\s{2,})([a-zA-Z]) - ", r"\1・", line)
        # 画面の右に寄せるための長い字下げ
        line = re.sub(r"^ {20,}", "", line)
        lines.append(line.rstrip())
    s = "\n".join(lines)
    # 色の印だけの行も空行とみなして、続く空行を 1 行に詰める
    s = re.sub(r"\n(?:[ \t]*(?:<(?!<)/?[a-z:]+>)*[ \t]*\n){2,}", "\n\n", s)
    return s


def split_quote(s: str) -> tuple[str, str]:
    """呪文の説明などの本文に入っている引用(下線の行の後)を分ける。"""
    m = re.search(r"\n_{5,}\n", s)
    if not m:
        return s, ""
    return s[:m.start()], s[m.end():]


def text_block(s: str, cls: str = "game") -> str:
    s = clean(s).strip("\n")
    if not s.strip():
        return ""
    return f'<div class="{cls}">{fmt(s)}</div>'


# ------------------------------------------------------------------ タイル
class Tiles:
    """画像シートからタイルを切り出して tile/ に置く。同じ絵は一枚にまとめる。"""

    def __init__(self, out: Path):
        from PIL import Image  # noqa: F401  (無ければここで止める)
        self.out = out / "tile"
        self.sheets: dict = {}
        self.done: dict = {}

    def path(self, t) -> str:
        if not t:
            return ""
        from PIL import Image
        key = (t["sheet"], t["sx"], t["sy"], t["ex"], t["ey"])
        if key in self.done:
            return self.done[key]
        sheet = self.sheets.get(t["sheet"])
        if sheet is None:
            sheet = Image.open(SRC / "rltiles" / f"{t['sheet']}.png").convert("RGBA")
            self.sheets[t["sheet"]] = sheet
        crop = sheet.crop((t["sx"], t["sy"], t["ex"], t["ey"]))
        canvas = Image.new("RGBA", (max(t["w"], 1), max(t["h"], 1)), (0, 0, 0, 0))
        canvas.paste(crop, (t["ox"], t["oy"]))
        name = f"{t['sheet']}-{t['sx']}-{t['sy']}.png"
        self.out.mkdir(parents=True, exist_ok=True)
        canvas.save(self.out / name, optimize=True)
        rel = f"tile/{name}"
        self.done[key] = rel
        return rel

    def img(self, t, alt: str = "") -> str:
        p = self.path(t)
        return f'<img loading="lazy" src="{p}" alt="{esc(alt)}">' if p else ""


# ------------------------------------------------------------------ HTML の骨組み
BRAND = "DCSS スポイラー"
NAV = [("index.html", "入口"), ("monster.html", "モンスター"),
       ("spell.html", "呪文"), ("item.html", "アイテム"),
       ("artefact.html", "固定アーティファクト"), ("species.html", "種族"),
       ("job.html", "職業"), ("god.html", "神"), ("branch.html", "ダンジョン"),
       ("mutation.html", "変異"), ("misc.html", "事典"),
       ("wiki/index.html", "wiki 翻訳")]


def page(fname: str, title: str, lead: str, body: str, version: str,
         wide: bool = False) -> str:
    nav = "".join(
        f'<a href="{h}"{" class=here" if h == fname else ""}>{t}</a>'
        for h, t in NAV)
    cls = " wide" if wide else ""
    return f"""<!doctype html>
<html lang="ja">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)} &mdash; {BRAND}</title>
<meta name="description" content="{esc(lead)}">
<link rel="stylesheet" href="dcss.css">
</head>
<body>
<header class="top"><div class="band">
  <a class="brand" href="index.html">{BRAND}</a>
  <nav>{nav}</nav>
</div></header>
<main class="band{cls}">
{body}
</main>
<footer class="foot"><div class="band">
  <p>Dungeon Crawl Stone Soup(DCSS)はオープンソースのローグライクゲームです。\
本家は<a href="{UPSTREAM}">こちら</a>。このページは日本語版 {esc(version)} の非公式スポイラーで、\
文章も数値もゲーム内の <code>?/</code> 検索で出てくるものをそのまま使っています。\
タイル画像も DCSS のもの(CC0)です。</p>
</div></footer>
<script src="dcss.js"></script>
</body>
</html>
"""


def filter_bar(*selects: str, ph: str = "名前や説明で検索") -> str:
    return f"""<div class="bar">
  <input type="search" id="q" placeholder="{ph}" autocomplete="off">
  {"".join(selects)}
  <span class="count" id="count"></span>
</div>"""


def select(sid: str, none_label: str, options: list[tuple[str, str]]) -> str:
    opts = [f'<option value="">{esc(none_label)}</option>']
    opts += [f'<option value="{esc(v)}">{esc(t)}</option>' for v, t in options]
    return f'<select id="{sid}">' + "".join(opts) + "</select>"


def card(art: str, title_html: str, inner: str, data_t: str,
         attrs: str = "", anchor: str = "") -> str:
    aid = f' id="{esc(anchor)}"' if anchor else ""
    art_div = f'<div class="art">{art}</div>' if art else ""
    return f"""<article class="card"{aid}{attrs} data-t="{esc(data_t.lower())}">
  {art_div}
  <div class="txt">
    <h3>{title_html}</h3>
{inner}
  </div>
</article>"""


def en(name: str) -> str:
    return f' <span class="en">{esc(name)}</span>'


def slug(s: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-")


def details(summary: str, inner: str) -> str:
    if not inner:
        return ""
    return f"<details><summary>{esc(summary)}</summary>{inner}</details>"


# ------------------------------------------------------------------ データ
class Data:
    def __init__(self, d: dict):
        self.d = d
        # git describe の版(0.34.1-ja1-2-gxxxx)は、リリースの名前までにする
        self.version = re.sub(r"-\d+-g[0-9a-f]+$", "", d["version"])
        self.lookup = d["lookup"]
        self.branches = d["branches"]
        self.br_by_id = {b["id"]: b for b in self.branches}
        self.br_by_abbrev = {b["abbrev"].lower(): b for b in self.branches}
        self.br_by_abbrev.update({b["short_en"].lower(): b for b in self.branches})
        # モンスターの英語名 → 出現の一覧
        self.places: dict[str, list] = defaultdict(list)
        for b in self.branches:
            for mon, lo, hi, rar, dist, kind in b["population"]:
                if mon.startswith("invalid"):   # 表の空き枠
                    continue
                lo = max(1, lo)
                hi = min(b["levels"], hi)
                if lo > hi:
                    continue
                self.places[mon].append((b, lo, hi, kind))
        self.uniq_places = parse_unique_depths(self)
        self.mon_by_en = {m["key"]: m for m in self.lookup["monster"]}

    def abs_depth(self, b: dict, lo: int) -> int:
        # 神の試練で入る場所(闘技場など)は基準の深さが 0 なので、並べるときは最後に回す
        if b["absdepth"] == 0 and b["abbrev"] != "D":
            return 99
        return b["absdepth"] + lo - 1


def br_range(b: dict, lo: int, hi: int) -> str:
    name = b["short_ja"]
    if b["levels"] <= 1:
        return name
    if lo == hi:
        return f"{name} {lo}"
    return f"{name} {lo}〜{hi}"


def parse_unique_depths(data: Data) -> dict[str, list[tuple[str, int, str]]]:
    """唯一モンスターの出る階を dat/des/builder/uniques.des の DEPTH から取る。
    名前(小文字)→ [(表示, ダンジョン全体での深さ)]。"""
    text = (SRC / "dat" / "des" / "builder" / "uniques.des").read_text(encoding="utf-8")
    res: dict[str, list[tuple[str, int, str]]] = defaultdict(list)
    for block in re.split(r"\n(?=NAME:)", text):
        dm = re.search(r"^DEPTH:\s*(.+)$", block, re.M)
        names = re.findall(r'place_unique\(\s*_G\s*,\s*"([^"]+)"', block)
        if not dm or not names:
            continue
        parts = []
        for p in dm.group(1).split(","):
            p = p.strip()
            if not p or p.startswith("!"):
                continue
            m = re.match(r"([A-Za-z]+)(?::(\$|\d+)(?:-(\$|\d*))?)?$", p)
            if not m:
                continue
            b = data.br_by_abbrev.get(m.group(1).lower())
            if not b:
                continue
            lo, hi = m.group(2), m.group(3)
            if lo is None:
                parts.append((b["short_ja"], data.abs_depth(b, 1), b["abbrev"]))
                continue
            lo_n = b["levels"] if lo == "$" else int(lo)
            lo_s = "最下層" if lo == "$" else lo
            if hi is None:
                parts.append((f"{b['short_ja']} {lo_s}", data.abs_depth(b, lo_n), b["abbrev"]))
            else:
                hi_s = "最下層" if hi in ("$", "") else hi
                parts.append((f"{b['short_ja']} {lo_s}〜{hi_s}",
                              data.abs_depth(b, lo_n), b["abbrev"]))
        for n in names:
            n = re.sub(r"\s+band$", "", n)
            res[n.lower()].extend(parts)
    return res


# ------------------------------------------------------------------ 各ページ
HOLINESS = {}  # 種別は本文に日本語で出るので数値は使わない


def build_monster(data: Data, tiles: Tiles) -> str:
    rows = []
    brs_seen = {}
    for m in data.lookup["monster"]:
        key = m["key"]
        places = data.places.get(m.get("name_en", key), [])
        uniq_pl = data.uniq_places.get(key.lower(), [])
        if places:
            first = min(data.abs_depth(b, lo) for b, lo, hi, kind in places)
            where = "、".join(dict.fromkeys(br_range(b, lo, hi)
                                            for b, lo, hi, kind in places))
        elif uniq_pl:
            first = min(d for _, d, _ in uniq_pl)
            where = "、".join(dict.fromkeys(t for t, _, _ in uniq_pl))
        else:
            first = 999
            where = ""
        brs = sorted({b["abbrev"] for b, *_ in places} | {a for _, _, a in uniq_pl})
        for b, *_ in places:
            brs_seen[b["abbrev"]] = b
        if "hd" in m:
            hp = m["hp10"] / 10
            nums = (f'<b class="glyph">{esc(m["glyph"])}</b>'
                    f' HD {m["hd"]}/HP 約{hp:g}/AC {m["ac"]}/EV {m["ev"]}'
                    f'/速度 {m["speed"]}')
            name = m["name_ja"]
            name_en = m["name_en"]
        else:
            nums = ""
            name = plain(m["title"]) or key
            name_en = key
        badges = ' <b class="uq">ユニーク</b>' if m.get("unique") else ""
        inner = ""
        if nums:
            inner += f'    <p class="num">{nums}</p>\n'
        if where:
            inner += f'    <p class="where">出現：{esc(where)}</p>\n'
        inner += "    " + text_block(m["body"]) + "\n"
        if m.get("quote"):
            inner += "    " + details("引用", text_block(m["quote"], "quote"))
        rows.append((first, name, card(
            tiles.img(m.get("tile")), esc(name) + badges + en(name_en), inner,
            f"{name} {name_en} {plain(m['body'])}",
            attrs=f' data-b="{" ".join(brs)}"', anchor="m-" + slug(key))))
    rows.sort(key=lambda r: (r[0], r[1]))
    opts = [(ab, b["short_ja"]) for ab, b in
            sorted(brs_seen.items(), key=lambda x: x[1]["id"])]
    lead = f"""全{len(rows)}種を、出てくる階が浅いものから順に並べています。
 「出現」は普通に湧く場所のことで、vault や召喚で出てくる分は入っていません。
 ユニークは決まった範囲の階のどこかに一体だけ置かれます。
 数値はそのモンスターの基本の値です。HP は平均で、速度は10がプレイヤーと同じ速さです。"""
    body = f"""<h1>モンスター</h1>
<p class="lead">{jp(lead)}</p>
{filter_bar(select("b", "場所：すべて", opts))}
<div class="list one" id="list">
{"".join(r[2] for r in rows)}
</div>"""
    return page("monster.html", "モンスター",
                "DCSS のモンスター全種のデータと説明。",
                body, data.version, wide=True)


def build_spell(data: Data, tiles: Tiles) -> str:
    books_of: dict[str, list[str]] = defaultdict(list)
    for bk in data.d["books"]:
        for s in bk["spells"]:
            books_of[s].append(bk["name_ja"])
    rows = []
    schools = set()
    for s in data.lookup["spell"]:
        if "level" not in s:
            continue
        sch = s["schools"]
        schools.update(sch)
        tags = f'<b class="lv">Lv {s["level"]}</b>' + "".join(
            f'<b class="fl">{esc(x)}</b>' for x in sch)
        if not s["player"]:
            tags += '<b class="fl dim">モンスター専用</b>'
        inner = f'    <p class="flags">{tags}</p>\n'
        bks = books_of.get(s["name_en"], [])
        if bks:
            inner += f'    <p class="where">載っている本：{esc("、".join(bks))}</p>\n'
        sbody, squote = split_quote(s["body"])
        inner += "    " + text_block(sbody) + "\n"
        squote = squote or s.get("quote", "")
        if squote.strip():
            inner += "    " + details("引用", text_block(squote, "quote"))
        rows.append(((not s["player"], s["level"], s["name_ja"]), card(
            tiles.img(s.get("tile")), esc(s["name_ja"]) + en(s["name_en"]), inner,
            f'{s["name_ja"]} {s["name_en"]} {plain(s["body"])}',
            attrs=f' data-b="{esc(" ".join(sch))}" data-g="{"p" if s["player"] else "m"}"',
            anchor="s-" + slug(s["name_en"]))))
    rows.sort(key=lambda r: r[0])
    n_player = sum(1 for s in data.lookup["spell"] if s.get("player"))
    lead = f"""全{len(rows)}個のうち、プレイヤーが覚えられるのは{n_player}個です。
 覚えられる呪文をレベルの低い順に並べ、モンスターしか使わない呪文は最後にまとめました。
 威力や射程は、実際のゲームではキャラクターのスキルや能力値によって変わります。"""
    body = f"""<h1>呪文</h1>
<p class="lead">{jp(lead)}</p>
{filter_bar(select("b", "系統：すべて", [(x, x) for x in sorted(schools)]),
            select("g", "使い手：すべて", [("p", "プレイヤーが覚えられる"), ("m", "モンスター専用")]))}
<div class="list" id="list">
{"".join(r[1] for r in rows)}
</div>"""
    return page("spell.html", "呪文", "DCSS の呪文すべてのデータと説明。",
                body, data.version, wide=True)


ITEM_GROUPS = [
    ("weapon", "武器"), ("missile", "矢弾・投擲物"), ("armour", "防具"),
    ("wand", "ワンド"), ("scroll", "巻物"), ("jewellery", "装身具"),
    ("potion", "薬"), ("book", "魔法書"), ("staff", "魔法の杖"),
    ("orb", "オーブ"), ("misc", "道具"), ("talisman", "護符"),
    ("gem", "宝石"), ("rune", "ルーン"), ("gold", "金貨"), ("other", "その他"),
]


def item_group(base: str) -> str:
    b = base.lower()
    for k, _ in ITEM_GROUPS:
        if k in b:
            return k
    if "ring" in b or "amulet" in b:
        return "jewellery"
    if "miscellaneous" in b:
        return "misc"
    return "other"


def build_items(data: Data, tiles: Tiles, unrand: bool) -> str:
    groups: dict[str, list] = defaultdict(list)
    for it in data.lookup["item"]:
        if it.get("unrand", False) != unrand:
            continue
        g = item_group(it.get("base", ""))
        name = it.get("name_ja") or plain(it["title"])
        inner = "    " + text_block(it["body"]) + "\n"
        groups[g].append((name, card(
            tiles.img(it.get("tile")), esc(name) + en(it["key"]), inner,
            f'{name} {it["key"]} {plain(it["body"])}', attrs=f' data-g="{g}"',
            anchor="i-" + slug(it["key"]))))
    secs = []
    opts = []
    total = 0
    for g, label in ITEM_GROUPS:
        if g not in groups:
            continue
        rows = sorted(groups[g], key=lambda r: r[0])
        total += len(rows)
        opts.append((g, label))
        secs.append(f"""<section class="tree" id="g-{g}">
<h2>{label}<span class="n">{len(rows)}</span></h2>
<div class="list">
{"".join(r[1] for r in rows)}
</div>
</section>""")
    jump = '<p class="jump">' + "".join(
        f'<a href="#g-{g}">{label}</a>' for g, label in opts) + "</p>"
    quick = "" if unrand else quick_tables(data)
    if unrand:
        title = "固定アーティファクト"
        lead = f"""名前と性能が決まっている一点ものの装備(unrandart)で、全部で{total}個あります。
 性能はゲーム内の説明どおりで、実際に拾ったときもこのとおりです。"""
        fname = "artefact.html"
        desc = "DCSS の固定アーティファクトすべての説明。"
    else:
        title = "アイテム"
        lead = f"""全{total}種を種類ごとに分けて載せています。
 薬や巻物、ワンドの見た目(色や模様)はプレイのたびに変わるので載せていません。"""
        fname = "item.html"
        desc = "DCSS のアイテム全種のデータと説明。"
    body = f"""<h1>{title}</h1>
<p class="lead">{jp(lead)}</p>
{jump}
{quick}
{filter_bar(select("g", "種類：すべて", opts))}
<div id="list">
{"".join(secs)}
</div>"""
    return page(fname, title, desc, body, data.version, wide=True)


def quick_tables(data: Data) -> str:
    """武器と防具の早見表。見出しの語はゲームの説明の訳語に合わせてある。"""
    def link(it):
        name = it.get("name_ja") or it["key"]
        return f'<a href="#i-{slug(it["key"])}">{esc(name)}</a>'

    items = [it for it in data.lookup["item"] if not it.get("unrand")]
    wpn = sorted((it for it in items if "dam" in it),
                 key=lambda it: (it["skill"], it["dam"]))
    rows = "".join(
        f'<tr><th>{link(it)}</th><td class="t">{esc(it["skill"])}</td>'
        f'<td>{it["dam"]}</td><td>{it["hit"]:+d}</td><td>{it["delay"] / 10:.1f}</td></tr>'
        for it in wpn)
    wtab = f"""<details class="quick"><summary>武器の一覧表({len(wpn)}種)</summary>
<div class="scroll"><table class="qt"><thead><tr><th>名前</th><th>スキル</th><th>基本ダメージ</th>
<th>基本命中</th><th>基本所要時間</th></tr></thead><tbody>{rows}</tbody></table></div></details>"""
    arm = sorted((it for it in items if "ac" in it),
                 key=lambda it: (it["evp"] == 0, it["evp"], -it["ac"]))
    rows = "".join(
        f'<tr><th>{link(it)}</th><td>{it["ac"]}</td><td>{-it["evp"] // 10}</td></tr>'
        for it in arm)
    atab = f"""<details class="quick"><summary>防具の一覧表({len(arm)}種)</summary>
<div class="scroll"><table class="qt"><thead><tr><th>名前</th><th>基本防御値</th>
<th>重さ</th></tr></thead><tbody>{rows}</tbody></table></div></details>"""
    return wtab + atab


STAT_JA = {0: "腕力", 1: "知力", 2: "器用"}


def apt_cell(a) -> str:
    if a is None:
        return '<td class="na">×</td>'
    cls = "pos" if a > 0 else "neg" if a < 0 else ""
    return f'<td class="{cls}">{a:+d}</td>' if a else '<td>0</td>'


def build_species(data: Data, tiles: Tiles) -> str:
    skills = data.d["skills"]
    sp = data.d["species"]
    head = "".join(f'<th><span>{esc(s["ja"])}</span></th>' for s in skills)
    trs = []
    for s in sp:
        cells = "".join(apt_cell(s["apt"].get(k["en"])) for k in skills)
        trs.append(f'<tr><th><a href="#sp-{slug(s["name_en"])}">{esc(s["name_ja"])}</a></th>'
                   f'{apt_cell(s["xp"])}{cells}</tr>')
    table = f"""<div class="scroll"><table class="apt">
<thead><tr><th></th><th><span>経験値</span></th>{head}</tr></thead>
<tbody>{"".join(trs)}</tbody></table></div>"""
    cards = []
    for s in sp:
        stats = (f'腕力 {s["str"]}/知力 {s["int"]}/器用 {s["dex"]}'
                 f'/HP補正 {s["hp"]*10:+d}%/MP補正 {s["mp"]:+d}'
                 f'/意志力 レベル×{s["wl"]}/体格 {s["size"]}')
        ls = "・".join(STAT_JA.get(x, "?") for x in s["level_stats"])
        grow = f'{s["how_often"]}レベルごとに{ls}' + ("のどれか" if len(s["level_stats"]) > 1 else "")
        inner = f'    <p class="num">{esc(stats)}</p>\n'
        inner += f'    <p class="where">{esc(grow)}が1上がります</p>\n'
        inner += "    " + text_block(s["desc"], "desc") + "\n"
        if s.get("lore", "").strip():
            inner += "    " + text_block(s["lore"], "desc lore") + "\n"
        feats =[f'<li><b>レベル{m["xl"]}</b> {fmt(m["desc"])}</li>'
                 if m["xl"] > 1 else f'<li><b>最初から</b> {fmt(m["desc"])}</li>'
                 for m in s["muts"]]
        feats += [f"<li>{fmt(x)}</li>" for x in s["fake_muts"]]
        if feats:
            inner += f'    <ul class="feats">{"".join(feats)}</ul>\n'
        if s["jobs"]:
            inner += f'    <p class="where">おすすめの職業：{esc("、".join(s["jobs"]))}</p>\n'
        if s.get("quote"):
            inner += "    " + details("引用", text_block(s["quote"], "quote"))
        cards.append(card(tiles.img(s.get("tile")), esc(s["name_ja"]) + en(s["name_en"]), inner,
                          f'{s["name_ja"]} {s["name_en"]} {plain(s["desc"])}',
                          anchor="sp-" + slug(s["name_en"])))
    lead = f"""選べる種族は{len(sp)}あります。下の表はスキルの適性で、
 プラスなら上がりやすく、マイナスなら上がりにくくなります。×はそのスキルを使えないという意味です。
 「経験値」の列は、レベルの上がりやすさです。"""
    body = f"""<h1>種族</h1>
<p class="lead">{jp(lead)}</p>
{table}
{filter_bar()}
<div class="list one" id="list">
{"".join(cards)}
</div>"""
    return page("species.html", "種族", "DCSS の種族の特徴と、スキル適性の一覧表。",
                body, data.version, wide=True)


def equip_ja(spec: str, item_names: dict[str, str]) -> str:
    """職業の装備の指定(vault の書式)を日本語の名前にする。"""
    name = re.sub(r"\s+\w+:\S+", "", spec).strip()
    q = re.search(r"\bq:(\d+)", spec)
    plus = re.search(r"\bplus:(-?\d+)", spec)
    ja = item_names.get(name.lower(), name)
    if plus:
        ja = f"{int(plus.group(1)):+d} の{ja}"
    if q and q.group(1) != "1":
        ja += f" ×{q.group(1)}"
    return ja


def build_job(data: Data, tiles: Tiles) -> str:
    item_names = {it["key"].lower(): it.get("name_ja") or it["key"]
                  for it in data.lookup["item"]}
    wchoice = {0: "", 1: "武器は開始時に選ぶ", 2: "武器は開始時に選ぶ(上等なもの)"}
    cards = []
    for j in data.d["jobs"]:
        inner = f'    <p class="num">腕力 {j["str"]}/知力 {j["int"]}/器用 {j["dex"]}</p>\n'
        inner += "    " + text_block(j["desc"], "desc") + "\n"
        sk = "、".join(f'{s.get("ja", "選んだ武器")} {s["level"]}' for s in j["skills"])
        inner += f'    <p class="where">開始時のスキル：{esc(sk)}</p>\n'
        eq = [equip_ja(x, item_names) for x in j["equipment"]]
        if wchoice.get(j["wchoice"]):
            eq.append(wchoice[j["wchoice"]])
        if eq:
            inner += f'    <p class="where">所持品：{esc("、".join(eq))}</p>\n'
        if j["spells"]:
            inner += f'    <p class="where">呪文：{esc("、".join(j["spells"]))}</p>\n'
        if j["species"]:
            inner += f'    <p class="where">おすすめの種族：{esc("、".join(j["species"]))}</p>\n'
        if j.get("quote"):
            inner += "    " + details("引用", text_block(j["quote"], "quote"))
        cards.append(card("", esc(j["name_ja"]) + en(j["name_en"]), inner,
                          f'{j["name_ja"]} {j["name_en"]} {plain(j["desc"])}',
                          anchor="j-" + slug(j["name_en"])))
    lead = f"""選べる職業は{len(cards)}あります。能力値は種族の値に足されます。
 スキルの横の数字は、ゲーム開始時のスキルレベルです。"""
    body = f"""<h1>職業</h1>
<p class="lead">{jp(lead)}</p>
{filter_bar()}
<div class="list" id="list">
{"".join(cards)}
</div>"""
    return page("job.html", "職業", "DCSS の職業ごとの開始時のスキルと所持品。",
                body, data.version, wide=True)


def build_god(data: Data, tiles: Tiles) -> str:
    cards = []
    tab_names = ["能力の詳細", "神罰", "その他"]
    for g in data.lookup["god"]:
        # 信仰していない神の力は灰色で出るが、ここでは普通の色で見せる
        tabs = [t.replace("<darkgrey>", "<lightgrey>")
                 .replace("</darkgrey>", "</lightgrey>") for t in g["tabs"]]
        inner = "    " + text_block(tabs[0]) + "\n"
        for name, t in zip(tab_names, tabs[1:]):
            inner += "    " + details(name, text_block(t)) + "\n"
        cards.append(card(tiles.img(g.get("tile")), esc(g["title"]) + en(g["key"]),
                          inner, f'{g["title"]} {g["key"]} ' + plain(" ".join(tabs)),
                          anchor="g-" + slug(g["key"])))
    lead = f"""神様は{len(cards)}柱います。祭壇で入信すると、信仰心に応じて能力をもらえます。
 説明はゲーム内の神様の画面と同じものです。"""
    body = f"""<h1>神</h1>
<p class="lead">{jp(lead)}</p>
{filter_bar()}
<div class="list one" id="list">
{"".join(cards)}
</div>"""
    return page("god.html", "神", "DCSS の神様ごとの能力と神罰。",
                body, data.version, wide=True)


def build_branch(data: Data, tiles: Tiles) -> str:
    lk = {b["key"].lower(): b for b in data.lookup["branch"]}
    cards = []
    for b in data.branches:
        e = lk.get(b["short_en"].lower())
        if not e:
            continue
        inner = "    " + text_block(e["body"]) + "\n"
        mons = defaultdict(list)
        for mon, lo, hi, rar, dist, kind in b["population"]:
            if mon.startswith("invalid"):
                continue
            lo, hi = max(1, lo), min(b["levels"], hi)
            if lo > hi:
                continue
            m = data.mon_by_en.get(mon)
            name = m["name_ja"] if m and "name_ja" in m else mon
            anchor = "m-" + slug(m["key"]) if m else ""
            label = name if b["levels"] <= 1 else (
                f"{name}({lo}" + (f"〜{hi}" if hi != lo else "") + ")")
            link = (f'<a href="monster.html#{anchor}">{esc(label)}</a>'
                    if anchor else esc(label))
            mons[kind].append((lo, name, link))
        for kind, label in ((0, "出るモンスター"), (1, "水の中"), (2, "溶岩の中")):
            if mons[kind]:
                lst = "、".join(x[2] for x in sorted(mons[kind]))
                inner += "    " + details(f"{label}({len(mons[kind])}種)",
                                          f'<p class="mons">{lst}</p>') + "\n"
        cards.append(card(tiles.img(e.get("tile")),
                          esc(b["long_ja"]) + en(b["short_en"]), inner,
                          f'{b["long_ja"]} {b["short_ja"]} {b["short_en"]} {plain(e["body"])}',
                          anchor="b-" + slug(b["short_en"])))
    lead = """本道と分岐、ポータルの先をすべて載せています。
 「出るモンスター」は普通に湧くモンスターの一覧で、カッコの中の数字はその分岐の何階に出るかです。"""
    body = f"""<h1>ダンジョン</h1>
<p class="lead">{jp(lead)}</p>
{filter_bar()}
<div class="list one" id="list">
{"".join(cards)}
</div>"""
    return page("branch.html", "ダンジョン", "DCSS の本道と分岐、それぞれに出るモンスター。",
                body, data.version, wide=True)


def simple_cards(entries: list, tiles: Tiles, prefix: str) -> list[tuple[str, str]]:
    rows = []
    for e in entries:
        name = plain(e["title"]) or e["key"]
        # 見出しは「日本語名 (English)」の形のことがあるので英語を外す
        m = re.match(r"^(.*?)\s*[\(（]([^()（）]*[A-Za-z][^()（）]*)[\)）]\s*$", name)
        name_en = e["key"]
        if m:
            name, name_en = m.group(1), m.group(2)
        inner = "    " + text_block(e["body"]) + "\n"
        if e.get("quote"):
            inner += "    " + details("引用", text_block(e["quote"], "quote"))
        rows.append((name, card(tiles.img(e.get("tile")), esc(name) + en(name_en),
                                inner, f'{name} {e["key"]} {plain(e["body"])}',
                                anchor=prefix + slug(e["key"]))))
    rows.sort(key=lambda r: r[0])
    return rows


def build_mutation(data: Data, tiles: Tiles) -> str:
    rows = simple_cards(data.lookup["mutation"], tiles, "u-")
    lead = f"""全{len(rows)}個。変異は、突然変異の薬や魔力汚染、神様の力などで付いたり消えたりします。
 種族によっては最初から持っているものもあります。段階があるものは、段階ごとの説明を並べています。"""
    body = f"""<h1>変異</h1>
<p class="lead">{jp(lead)}</p>
{filter_bar()}
<div class="list" id="list">
{"".join(r[1] for r in rows)}
</div>"""
    return page("mutation.html", "変異", "DCSS の変異すべての説明。",
                body, data.version, wide=True)


MISC = [("skill", "スキル"), ("ability", "能力"), ("status", "状態"),
        ("passive", "常時の効果"), ("cloud", "雲"), ("feature", "地形"),
        ("card", "カード"), ("bane", "災厄")]


def build_misc(data: Data, tiles: Tiles) -> str:
    secs = []
    opts = []
    for typ, label in MISC:
        rows = simple_cards(data.lookup.get(typ, []), tiles, typ[0] + "-")
        if not rows:
            continue
        opts.append((typ, label))
        cards = "".join(r[1].replace('<article class="card"',
                                     f'<article class="card" data-g="{typ}"', 1)
                        for r in rows)
        secs.append(f"""<section class="tree" id="g-{typ}">
<h2>{label}<span class="n">{len(rows)}</span></h2>
<div class="list">
{cards}
</div>
</section>""")
    jump = '<p class="jump">' + "".join(
        f'<a href="#g-{g}">{label}</a>' for g, label in opts) + "</p>"
    lead = """<code>?/</code> 検索で引ける項目のうち、ほかのページに入らなかったものです。
 能力には神様や種族の能力も入っています。カードはネメレクスのデッキのカードです。"""
    body = f"""<h1>事典</h1>
<p class="lead">{jp(lead)}</p>
{jump}
{filter_bar(select("g", "種類：すべて", opts))}
<div id="list">
{"".join(secs)}
</div>"""
    return page("misc.html", "事典", "DCSS のスキル・能力・状態・雲・地形などの説明。",
                body, data.version, wide=True)


def build_index(data: Data, counts: dict) -> str:
    cards = [
        ("monster.html", "モンスター", f'{counts["monster"]}種。出現場所や強さ、攻撃と呪文'),
        ("spell.html", "呪文", f'{counts["spell"]}個。レベルと系統、どの本に載っているか'),
        ("item.html", "アイテム", f'{counts["item"]}種。武器と防具は一覧表つき'),
        ("artefact.html", "固定アーティファクト", f'{counts["artefact"]}個の一点もの'),
        ("species.html", "種族", f'{counts["species"]}種族。スキル適性の表つき'),
        ("job.html", "職業", f'{counts["job"]}職業。開始時のスキルと所持品'),
        ("god.html", "神", f'{counts["god"]}柱。もらえる能力と神罰'),
        ("branch.html", "ダンジョン", f'{counts["branch"]}か所。それぞれに出るモンスター'),
        ("mutation.html", "変異", f'{counts["mutation"]}個'),
        ("misc.html", "事典", "スキル、能力、状態、雲、地形、カードなど"),
    ]
    tiles_html = "".join(f'<a class="t" href="{h}"><b>{t}</b><span>{esc(s)}</span></a>'
                         for h, t, s in cards)
    body = f"""<section class="hero">
  <h1>{BRAND}</h1>
  <p class="tag">Dungeon Crawl Stone Soup 日本語版のゲーム内データをまとめました。<br>
     モンスターや呪文、アイテム、神様のことを日本語で調べられます。</p>
</section>

<section class="cards">
  <h2>ページ一覧</h2>
  <div class="tiles">{tiles_html}</div>
</section>"""
    # 手書きの文の折り返しを詰める(タグの前後の改行は残す)
    body = re.sub(r"(?<=[^>\n])\n\s+(?=[^<\s])", "", body)
    return page("index.html", "入口", "DCSS 日本語版のゲーム内データをまとめた非公式スポイラー。",
                body, data.version)


# ------------------------------------------------------------------ 本体
def write(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8", newline="\n")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--no-dump", action="store_true",
                    help="crawl を走らせず、書き出し済みの JSON を使う")
    ap.add_argument("--out", type=Path, default=OUT)
    args = ap.parse_args()

    if not args.no_dump:
        dump()
    d = json.loads(JSON_PATH.read_text(encoding="utf-8"))
    data = Data(d)

    out: Path = args.out
    if (out / "tile").exists():
        shutil.rmtree(out / "tile")
    out.mkdir(parents=True, exist_ok=True)
    tiles = Tiles(out)
    for name in ("dcss.css", "dcss.js"):
        shutil.copyfile(ASSETS / name, out / name)

    pages = {
        "monster.html": build_monster(data, tiles),
        "spell.html": build_spell(data, tiles),
        "item.html": build_items(data, tiles, unrand=False),
        "artefact.html": build_items(data, tiles, unrand=True),
        "species.html": build_species(data, tiles),
        "job.html": build_job(data, tiles),
        "god.html": build_god(data, tiles),
        "branch.html": build_branch(data, tiles),
        "mutation.html": build_mutation(data, tiles),
        "misc.html": build_misc(data, tiles),
    }
    lk = data.lookup
    counts = {
        "monster": len(lk["monster"]), "spell": len(lk["spell"]),
        "item": sum(1 for x in lk["item"] if not x.get("unrand")),
        "artefact": sum(1 for x in lk["item"] if x.get("unrand")),
        "species": len(d["species"]), "job": len(d["jobs"]),
        "god": len(lk["god"]), "branch": len(lk["branch"]),
        "mutation": len(lk["mutation"]),
    }
    pages["index.html"] = build_index(data, counts)
    for name, text in pages.items():
        write(out / name, text)
    print(f"{out}: {len(pages)} ページ、タイル {len(tiles.done)} 枚")
    for k, v in counts.items():
        print(f"  {k}: {v}")


if __name__ == "__main__":
    main()
