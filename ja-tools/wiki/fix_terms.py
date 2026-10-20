#!/usr/bin/env python3
# wiki の訳(ja/*.html)の表記ゆれをそろえる。何度かけても結果は変わらない。
#
#   python fix_terms.py [-n] [-v] ja/Foo.html ja/Bar.html ...
#     -n  書き込まない(件数だけ出す)
#     -v  置き換えた箇所を前後の文字といっしょに出す
#
# (a) 変身の名前:「ドラゴンの形態」などを、ゲームの変身名([form]X の訳)にする。
#     「〜の形態になる」は「竜化する」「蜘蛛に変身する」のように活用ごと直す。
# (b) 階の表記: D:3 → 迷宮:3、Lair:5 → 獣の棲み処:5、Zot:1-4 → ゾットの領域:1〜4。
#     分岐の名前は ja-tools/spoiler/work/spoiler.json の long_ja。
#
# 直すのは表示される文字だけ。タグの中(href・title・alt などの属性)と
# <pre> <code> <kbd> <tt> <samp> <script> <style> の中は変えない。
import bisect
import json
import os
import re
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
JATOOLS = os.path.normpath(os.path.join(HERE, '..'))
FORMS_TXT = os.path.join(JATOOLS, '..', 'crawl-ref', 'source', 'dat', 'database',
                         'ja', 'jtrans', 'data-forms.txt')
SPOILER_JSON = os.path.join(JATOOLS, 'spoiler', 'work', 'spoiler.json')

# ---------------------------------------------------------------- 変身名

# 辞書に無い(古い版にしか無い)変身は「〜の形態」のまま残す:
#   氷の形態(Ice Form)、獣の形態(Beast Form)、処刑の形態(Executioner Form)
KEEP_FORMS = {'氷の形態', '獣の形態', '処刑の形態'}

# 辞書の長い名前(X-form の訳)以外で wiki の訳に出てくる言い方
EXTRA_FORMS = {
    '大蛇の形態': '大蛇',
    '蛇の形態': '大蛇',
    '鰻の形態': '鰻の手',
    '鰻の手の形態': '鰻の手',
    '刃の形態': '刃の手',
    '竜の形態': '竜化',
    '樹の形態': '樹化',
    'ゼリーの形態': 'ジェリー',
}


def load_forms():
    """{wiki での言い方: ゲームの変身名}"""
    text = open(FORMS_TXT, encoding='utf-8').read()
    short = {}  # transformation の識別子 -> [form]X の訳
    long_ = {}  # transformation の識別子 -> X-form の訳
    cur = None
    for block in text.split('%%%%'):
        lines = block.strip('\n').split('\n')
        for l in lines:
            m = re.match(r'# transformation::(\w+)', l)
            if m:
                cur = m.group(1)
        body = [l for l in lines if not l.startswith('#')]
        if len(body) < 2 or cur is None:
            continue
        en, ja = body[0], body[-1].strip()
        if re.match(r'\[form\][A-Z]', en) and cur not in short:
            short[cur] = ja
        elif re.match(r'^[a-z]+-form$', en) and cur not in long_:
            long_[cur] = ja
    forms = {}
    for k, ja in long_.items():
        if k in short and ja.endswith('の形態'):
            forms[ja] = short[k]
    forms.update(EXTRA_FORMS)
    for k in KEEP_FORMS:
        forms.pop(k, None)
    # 「の」の無い言い方(樹木形態など)も拾う
    for k in list(forms):
        forms.setdefault(k[:-3] + '形態', forms[k])
    return forms, sorted(set(short.values()), key=len, reverse=True)


# 「〜の形態」に続く語の直し方。(続く語, 化の付く名前のとき, そうでないとき)
# 上から順に試す。
SUFFIXES = [
    ('になっている', 'している', 'に変身している'),
    ('になっていた', 'していた', 'に変身していた'),
    ('になった', 'した', 'に変身した'),
    ('になって', 'して', 'に変身して'),
    ('になれない', 'できない', 'に変身できない'),
    ('になれる', 'できる', 'に変身できる'),
    ('になれば', 'すれば', 'に変身すれば'),
    ('になる', 'する', 'に変身する'),
    ('になり', 'し', 'に変身し'),
    ('に変身', '', 'に変身'),
    ('へ変身', '', 'に変身'),
    ('でいる', 'している', 'に変身している'),
    ('でいた', 'していた', 'に変身していた'),
    ('のまま', 'したまま', 'に変身したまま'),
    ('のあいだ', 'しているあいだ', 'に変身しているあいだ'),
    ('の間', 'している間', 'に変身している間'),
    ('の時', 'した時', 'に変身した時'),
    ('のとき', 'したとき', 'に変身したとき'),
    ('の状態', 'した状態', 'に変身した状態'),
    ('中', '中', 'に変身中'),
    ('で', 'した状態で', 'に変身した状態で'),
]


def fix_suffix(name, rest):
    """name(ゲームの変身名)のあとに続く rest を自然な形にする。(直した続き, 消費した長さ)"""
    ka = name.endswith('化')
    for suf, a, b in SUFFIXES:
        if rest.startswith(suf):
            # 「で」は「では」「でも」「で、」なども含めて「した状態で」にする
            return (a if ka else b), len(suf)
    return '', 0


# ---------------------------------------------------------------- 階の表記

def load_branches():
    j = json.load(open(SPOILER_JSON, encoding='utf-8'))
    return {b['abbrev']: b['long_ja'] for b in j['branches'] if b.get('long_ja')}


# ---------------------------------------------------------------- 保護する範囲

SKIP_ELEMS = ('pre', 'code', 'kbd', 'tt', 'samp', 'script', 'style')


def protected_ranges(html):
    """変えてはいけない範囲(タグの中と、pre などの要素の中)の [(start, end), ...]"""
    r = [(m.start(), m.end()) for m in re.finditer(r'<[^>]*>', html)]
    for el in SKIP_ELEMS:
        for m in re.finditer(r'<%s\b[^>]*>.*?</%s\s*>' % (el, el), html, re.S | re.I):
            r.append((m.start(), m.end()))
    r.sort()
    return r


class Prot:
    def __init__(self, html):
        merged = []
        for a, b in protected_ranges(html):
            if merged and a <= merged[-1][1]:
                merged[-1][1] = max(merged[-1][1], b)
            else:
                merged.append([a, b])
        self.r = merged
        self.starts = [a for a, _ in merged]

    def hit(self, a, b):
        """[a, b) が保護範囲と重なるか"""
        i = bisect.bisect_right(self.starts, b - 1) - 1
        return i >= 0 and self.r[i][1] > a


CJK = r'[　-ヿ㐀-鿿＀-￯]'


def apply(html, forms, short_names, branches, log, verbose_ctx):
    counts = Counter()

    def ctx(m, new):
        if verbose_ctx is not None:
            s = html_now[max(0, m.start() - 25):m.start()]
            e = html_now[m.end():m.end() + 25]
            verbose_ctx.append('%s[%s → %s]%s' % (s, m.group(0), new, e))

    # (a1) 「ドラゴンの形態」(リンクの中でもよい)+続く語
    names = '|'.join(re.escape(k) for k in sorted(forms, key=len, reverse=True))
    pat = re.compile(r'(%s)((?:\s*</a>)?)(\s*)(になっている|になっていた|になった|になって|'
                     r'になれない|になれる|になれば|になる|になり|に変身|へ変身|でいる|でいた|'
                     r'のまま|のあいだ|の間|の時|のとき|の状態|中|で(?![あすしなき]))?' % names)
    html_now = html
    prot = Prot(html_now)

    def sub_a1(m):
        if prot.hit(m.start(1), m.end(1)):
            return m.group(0)
        if m.group(4) and prot.hit(m.start(4), m.end(4)):
            return m.group(0)
        name = forms[m.group(1)]
        tail = ''
        rest = m.group(4) or ''
        if rest:
            new, n = fix_suffix(name, rest)
            tail = new + rest[n:]
        if rest and m.group(2) and tail != rest:
            # </a> をまたぐときは、続く語のほうを直す(スペースは詰める)
            out = name + m.group(2) + tail
        elif rest:
            out = name + m.group(2) + (m.group(3) if tail == rest else '') + tail
        else:
            out = name + m.group(2) + m.group(3)
        counts['変身名'] += 1
        ctx(m, out)
        return out

    html_now = pat.sub(sub_a1, html_now)

    # (a2) 「<a ...>流転</a> の形態」のように、リンクの外に「の形態」があるもの
    sn = '|'.join(re.escape(k) for k in short_names)
    pat2 = re.compile(r'>(%s)(</a>)(\s*)の形態(になっている|になっていた|になった|になって|'
                      r'になれない|になれる|になれば|になる|になり|に変身|へ変身|でいる|でいた|'
                      r'のまま|のあいだ|の間|の時|のとき|の状態|中|で(?![あすしなき]))?' % sn)
    prot = Prot(html_now)

    def sub_a2(m):
        if prot.hit(m.start(1), m.end(1)) or prot.hit(m.end(2), m.end(0)):
            return m.group(0)
        name = m.group(1)
        rest = m.group(4) or ''
        new, n = fix_suffix(name, rest) if rest else ('', 0)
        out = '>' + name + m.group(2) + new + rest[n:]
        counts['変身名'] += 1
        ctx(m, out)
        return out

    html_now = pat2.sub(sub_a2, html_now)

    # (b) 階の表記
    ab = '|'.join(re.escape(k) for k in sorted(branches, key=len, reverse=True))
    pat3 = re.compile(r'( ?)(?<![A-Za-z0-9_.\-])(?<![A-Za-z0-9_.\-:]/)(%s):([1-9]\d*)(?:\s?[-–~〜～]\s?(?:\2:)?(\d+))?'
                      r'(?![\d:A-Za-z])( ?)' % ab)
    prot = Prot(html_now)

    def sub_b(m):
        if prot.hit(m.start(2), m.end(0) - len(m.group(5))):
            return m.group(0)
        out = '%s:%s' % (branches[m.group(2)], m.group(3))
        if m.group(4):
            out += '〜' + m.group(4)
        # 前後が日本語の文字なら、間のスペースを詰める
        pre, post = m.group(1), m.group(5)
        before = html_now[m.start() - 1] if m.start() > 0 else ''
        after = html_now[m.end()] if m.end() < len(html_now) else ''
        if pre and re.match(CJK, before):
            pre = ''
        if post and re.match(CJK, after):
            post = ''
        out = pre + out + post
        counts['階の表記'] += 1
        ctx(m, out)
        return out

    html_now = pat3.sub(sub_b, html_now)
    return html_now, counts


def main(argv):
    dry = '-n' in argv
    verbose = '-v' in argv
    files = [a for a in argv if not a.startswith('-')]
    forms, short_names = load_forms()
    branches = load_branches()
    total = Counter()
    nfiles = 0
    for f in files:
        src = open(f, encoding='utf-8', newline='').read()
        ctxs = [] if verbose else None
        out, c = apply(src, forms, short_names, branches, None, ctxs)
        if out != src:
            nfiles += 1
            total.update(c)
            print('%s: %s' % (f, ', '.join('%s %d' % kv for kv in c.items())))
            for s in ctxs or []:
                print('    ' + s.replace('\n', ' '))
            if not dry:
                with open(f, 'w', encoding='utf-8', newline='') as fp:
                    fp.write(out)
    print('合計: %d ファイル, %s' % (nfiles, ', '.join('%s %d' % kv for kv in total.items())))


if __name__ == '__main__':
    main(sys.argv[1:])
