#!/usr/bin/env python3
"""C++ ソースの日本語化(jtrans 張り直し)用ツール。

リポジトリのルートで実行する: python -I ja-tools/jtools.py <サブコマンド> ...

  old FILE           0.19-ja でそのファイルに加えられた差分を表示
                     (FILE は 0.34 のファイル名。item-name.cc → itemname.cc
                     のような改名は自動で探す)
  olddict FILE       そのファイル用の旧辞書(jtrans_<旧名>.txt)を表示
  find TEXT          旧辞書のキー・訳から TEXT を含むものを探す(大小無視)
  check [FILE...]    jtrans 系の呼び出しのうち辞書に訳がないキーを一覧表示
                     (FILE 省略時は全ソース)
  harvest FILE...    FILE の未訳キーのうち、旧辞書に同じキーがあるものを
                     dat/database/ja/jtrans/<FILE の拡張子なし>.txt に追記
  wrap FILE...       mpr("...") などの定型パターンの英文を機械的に
                     jtrans/jtransc で包む(残りは手で包む)

辞書: crawl-ref/source/dat/database/ja/jtrans/*.txt(ゲームが読み込む)
旧辞書: 0.19-ja の jtrans_*.txt と 0.16-ja の jtrans.txt(git から読む)
"""
import os
import re
import subprocess
import sys

SRC = 'crawl-ref/source'
DICT_DIR = f'{SRC}/dat/database/ja/jtrans'
OLD_EN = 'origin/stone_soup-0.19'
OLD_JA = 'ja19/develop'
OLD16_JA = 'ja16/develop'

JTRANS_FUNCS = ('jtrans', 'jtransc', 'jtransln', 'jtranslnc', 'jtrans_notrim',
                'jtrans_notrimc', 'jtrans_has_key')
# 第 1 引数がタグ、第 2 引数がキーのもの
TAGGED_FUNCS = ('tagged_jtrans', 'tagged_jtransc')
# 固定タグ付きのマクロ
TAG_MACROS = {
    'verb_j': '[verb]', 'verb_jc': '[verb]',
    'adj_j': '[adj]', 'adj_jc': '[adj]',
    'adv_j': '[adv]', 'adv_jc': '[adv]',
    'branch_name_j': '[branch]', 'branch_name_jc': '[branch]',
    'zap_name_j': '[zap]', 'zap_name_jc': '[zap]',
}


def git(*args):
    r = subprocess.run(['git', *args], capture_output=True)
    if r.returncode != 0:
        return None
    return r.stdout.decode('utf-8', 'replace').replace('\r\n', '\n')


# ---------------------------------------------------------------------------
# 辞書の読み書き

def parse_db(text):
    """DB テキストを [(キー行, 値)] にする(database.cc と同じ規則)。"""
    out = []
    key, val, in_entry = None, [], False
    for line in text.split('\n'):
        if line.startswith('#'):
            continue
        if line.startswith('%%%%'):
            if key:
                out.append((key, '\n'.join(val).strip('\n')))
            key, val, in_entry = None, [], True
            continue
        if not in_entry:
            continue
        if key is None:
            key = line.strip() or None
        else:
            val.append(line.rstrip())
    if key:
        out.append((key, '\n'.join(val).strip('\n')))
    return out


def canon(key):
    """jtrans() と同じ正規化: 前後の空白除去、改行を \\n に、小文字化。"""
    return key.strip().replace('\n', '\\n').lower()


def load_dict():
    """ゲームが読む辞書を canon(key) -> (値, ファイル) で返す。"""
    d = {}
    if not os.path.isdir(DICT_DIR):
        return d
    for f in sorted(os.listdir(DICT_DIR)):
        if not f.endswith('.txt'):
            continue
        with open(os.path.join(DICT_DIR, f), encoding='utf-8') as fp:
            for k, v in parse_db(fp.read()):
                d.setdefault(k.lower(), (v, f))
    return d


_old_cache = None


def load_old_dict():
    """旧辞書を canon(key) -> [(値, 出典)] で返す。0.19-ja を優先。"""
    global _old_cache
    if _old_cache is not None:
        return _old_cache
    d = {}
    path = f'{SRC}/dat/database/ja'
    for ref, pred in ((OLD_JA, lambda f: f.startswith('jtrans_')
                       or f == 'montitle.txt'),
                      (OLD16_JA, lambda f: f.startswith('jtrans'))):
        names = git('ls-tree', '--name-only', ref, path + '/') or ''
        for p in names.split('\n'):
            f = p.rsplit('/', 1)[-1]
            if not f.endswith('.txt') or not pred(f):
                continue
            src = f'{ref.split("/")[0]}:{f}'
            for k, v in parse_db(git('show', f'{ref}:{p}') or ''):
                d.setdefault(k.lower(), []).append((v, src))
    _old_cache = d
    return d


def append_entries(path, entries):
    """[(キー, 値)] を DB テキスト形式で path に追記する。"""
    exists = os.path.exists(path)
    with open(path, 'a', encoding='utf-8', newline='\n') as fp:
        if not exists:
            fp.write('%%%%\n')
        for k, v in entries:
            fp.write(f'{k}\n\n{v}\n%%%%\n')


# ---------------------------------------------------------------------------
# ソースからキーを抜き出す

STR_LIT = r'(?:u8)?"(?:[^"\\\n]|\\.)*"'
LITS = rf'{STR_LIT}(?:\s*{STR_LIT})*'


def unescape(lits):
    s = ''
    for m in re.finditer(STR_LIT, lits):
        body = m.group(0)
        body = body[body.index('"') + 1:-1]
        s += re.sub(r'\\(x[0-9a-fA-F]+|u[0-9a-fA-F]{4}|.)', _unesc, body)
    return s


def _unesc(m):
    c = m.group(1)
    if c[0] == 'x':
        return chr(int(c[1:], 16))
    if c[0] == 'u':
        return chr(int(c[1:], 16))
    return {'n': '\n', 't': '\t', 'r': '\r', '0': '\0'}.get(c, c)


def strip_comments(code):
    # 文字列リテラルを残しつつコメントを消す
    out = []
    i, n = 0, len(code)
    while i < n:
        c = code[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and code[j] != c:
                j += 2 if code[j] == '\\' else 1
            out.append(code[i:j + 1])
            i = j + 1
        elif code.startswith('//', i):
            j = code.find('\n', i)
            i = n if j < 0 else j
        elif code.startswith('/*', i):
            j = code.find('*/', i + 2)
            seg = code[i:(n if j < 0 else j + 2)]
            out.append('\n' * seg.count('\n'))
            i = n if j < 0 else j + 2
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def _call_arg(code, start):
    """code[start] が '(' のとき、対応する ')' までの第 1 引数を返す。"""
    depth, i, n = 0, start, len(code)
    while i < n:
        c = code[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and code[j] != c:
                j += 2 if code[j] == '\\' else 1
            i = j + 1
            continue
        if c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
            if depth == 0:
                return code[start + 1:i]
        elif c == ',' and depth == 1:
            return code[start + 1:i]
        i += 1
    return ''


def _literal_keys(arg):
    """jtrans の引数式からキーになるリテラルを取り出す。
    "A"、cond ? "A" : "B" のような形だけを対象にし、make_stringf や
    文字列の連結(+)を含む動的な式は対象外(空リスト)。"""
    outside = re.sub(STR_LIT, '""', arg)
    if 'make_stringf' in outside or '+' in outside:
        return []
    # 関数呼び出しを含む式は、三項演算子の選択肢のリテラルだけを見る
    if '(' in outside and '?' not in outside:
        return []
    return [unescape(m.group(0)) for m in re.finditer(LITS, arg)]


def source_keys(path):
    """ファイル中の jtrans 系呼び出しのリテラル引数を
    [(行番号, [候補キー...])] で返す。候補のどれかが辞書にあれば訳あり。"""
    with open(path, encoding='utf-8', errors='replace') as fp:
        code = strip_comments(fp.read())
    res = []
    plain = '|'.join(JTRANS_FUNCS)
    for m in re.finditer(rf'\b(?:{plain})\s*(?=\()', code):
        line = code.count('\n', 0, m.start()) + 1
        for key in _literal_keys(_call_arg(code, m.end())):
            res.append((line, [key]))
    tagged = '|'.join(TAGGED_FUNCS)
    for m in re.finditer(rf'\b(?:{tagged})\s*\(\s*({LITS})\s*,\s*({LITS})\s*[,)]',
                         code):
        line = code.count('\n', 0, m.start()) + 1
        tag, key = unescape(m.group(1)), unescape(m.group(2))
        res.append((line, [tag + key.strip(), key]))
    macros = '|'.join(TAG_MACROS)
    for m in re.finditer(rf'\b({macros})\s*\(\s*({LITS})\s*[,)]', code):
        line = code.count('\n', 0, m.start()) + 1
        key = unescape(m.group(2))
        res.append((line, [TAG_MACROS[m.group(1)] + key.strip(), key]))
    return res


def source_files(args):
    if args:
        return [a if os.path.exists(a) else f'{SRC}/{a}' for a in args]
    return sorted(f'{SRC}/{f}' for f in os.listdir(SRC)
                  if f.endswith('.cc') or f.endswith('.h'))


def missing_keys(path, d):
    out = []
    seen = set()
    for line, cands in source_keys(path):
        if any(canon(c) in d for c in cands):
            continue
        k = cands[0]
        if canon(k) in seen or not k.strip():
            continue
        seen.add(canon(k))
        out.append((line, k, cands))
    return out


# ---------------------------------------------------------------------------
# 旧ファイルとの対応

def old_name(name):
    """0.34 のファイル名から 0.19 での名前を推測する。"""
    names = git('ls-tree', '--name-only', OLD_EN, SRC + '/') or ''
    old = {p.rsplit('/', 1)[-1] for p in names.split('\n')}
    for cand in (name, name.replace('-', '_'), name.replace('-', '')):
        if cand in old:
            return cand
    return None


def dict_name_for(path):
    return os.path.splitext(os.path.basename(path))[0] + '.txt'


# ---------------------------------------------------------------------------
# サブコマンド

def cmd_old(args):
    for a in args:
        o = old_name(os.path.basename(a))
        if not o:
            print(f'# {a}: 0.19 に対応するファイルなし')
            continue
        print(f'# {a} <- 0.19 {o}')
        sys.stdout.write(git('diff', OLD_EN, OLD_JA, '--', f'{SRC}/{o}') or '')


def cmd_olddict(args):
    for a in args:
        o = old_name(os.path.basename(a))
        if not o:
            print(f'# {a}: 0.19 に対応するファイルなし')
            continue
        f = 'jtrans_' + os.path.splitext(o)[0].replace('-', '_') + '.txt'
        text = git('show', f'{OLD_JA}:{SRC}/dat/database/ja/{f}')
        print(f'# {a} <- {f}')
        sys.stdout.write(text or '(なし)\n')


def cmd_find(args):
    text = ' '.join(args).lower()
    for k, vals in load_old_dict().items():
        for v, src in vals:
            if text in k or text in v.lower():
                print(f'[{src}] {k}\n    -> {v}')


def cmd_check(args):
    d = load_dict()
    total = 0
    for path in source_files(args):
        miss = missing_keys(path, d)
        if not miss:
            continue
        total += len(miss)
        print(f'== {path} ({len(miss)})')
        for line, k, _ in miss:
            print(f'  {line}: {k!r}')
    print(f'未訳キー: {total}')


def cmd_harvest(args):
    d = load_dict()
    old = load_old_dict()
    for path in source_files(args):
        found, left = [], []
        for line, k, cands in missing_keys(path, d):
            hit = next(((c, old[canon(c)][0][0]) for c in cands
                        if canon(c) in old), None)
            if hit:
                found.append((hit[0].strip().replace('\n', '\\n'), hit[1]))
                d[canon(hit[0])] = (hit[1], '')
            else:
                left.append((line, k))
        if found:
            append_entries(os.path.join(DICT_DIR, dict_name_for(path)), found)
        print(f'== {path}: 旧辞書から {len(found)} 件、未訳 {len(left)} 件')
        for line, k in left:
            print(f'  {line}: {k!r}')


# 機械的に包める定型パターン: (正規表現, 置換関数)
# 各パターンのグループ 'pre' はそのまま残し、'lit' を包む。
_CH = r'(?:MSGCH_\w+|channel|chan|ch)'
WRAP_PATTERNS = [
    # mpr("...") / mpr_nojoin(ch, "...")
    (rf'(?P<pre>\bmpr\(\s*)(?P<lit>{LITS})(?=\s*\))', 'jtrans'),
    (rf'(?P<pre>\bmpr_nojoin\(\s*{_CH}\s*,\s*)(?P<lit>{LITS})(?=\s*\))',
     'jtrans'),
    # mprf("fmt", ...) / mprf(ch, "fmt", ...) / mprf(ch, param, "fmt", ...)
    (rf'(?P<pre>\bmprf(?:_nojoin|_nocap)?\(\s*(?:{_CH}\s*,\s*'
     rf'(?:[^,"()]+,\s*)?)?)(?P<lit>{LITS})', 'jtransc'),
    (rf'(?P<pre>\bsimple_monster_message\(\s*[^,()]+(?:\([^()]*\))?\s*,\s*)'
     rf'(?P<lit>{LITS})', 'jtransc'),
    (rf'(?P<pre>\bsimple_god_message\(\s*)(?P<lit>{LITS})', 'jtransc'),
    (rf'(?P<pre>\bgod_speaks\(\s*[^,()]+\s*,\s*)(?P<lit>{LITS})', 'jtransc'),
    (rf'(?P<pre>\byesno\(\s*)(?P<lit>{LITS})', 'jtransc'),
    (rf'(?P<pre>\byes_or_no\(\s*)(?P<lit>{LITS})', 'jtransc'),
    (rf'(?P<pre>\*reason\s*=\s*)(?P<lit>{LITS})(?=\s*;)', 'jtrans'),
    (rf'(?P<pre>\bfail_reason\s*=\s*)(?P<lit>{LITS})(?=\s*;)', 'jtrans'),
]


def wrap_text(code):
    """定型パターンの英文リテラルを jtrans/jtransc で包む。
    戻り値: (新しいコード, 包んだ件数)。コメント内は対象外。"""
    total = 0
    for pat, func in WRAP_PATTERNS:
        def repl(m):
            nonlocal total
            lit = m.group('lit')
            text = re.sub(r'</?[a-z_]+>|%[-+ #0-9.]*[a-zA-Z]', '',
                          unescape(lit))
            if not re.search(r'[A-Za-z]{2,}', text):
                return m.group(0)
            total += 1
            return f'{m.group("pre")}{func}({lit})'
        code = _sub_outside_comments(pat, repl, code)
    return code, total


def _sub_outside_comments(pat, repl, code):
    # コメント部分を伏せた写しでマッチ位置を探し、元のコードを置換する
    masked = strip_comments_keep_len(code)
    out, last = [], 0
    for m in re.finditer(pat, masked):
        m2 = re.match(pat, code[m.start():])
        if not m2 or m2.end() != m.end() - m.start():
            continue
        out.append(code[last:m.start()])
        out.append(repl(m2))
        last = m.end()
    out.append(code[last:])
    return ''.join(out)


def strip_comments_keep_len(code):
    """コメントを同じ長さの空白に置き換える(文字列リテラルは残す)。"""
    out = list(code)
    i, n = 0, len(code)
    while i < n:
        c = code[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and code[j] != c and code[j] != '\n':
                j += 2 if code[j] == '\\' else 1
            i = j + 1
        elif code.startswith('//', i):
            j = code.find('\n', i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = ' '
            i = j
        elif code.startswith('/*', i):
            j = code.find('*/', i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != '\n':
                    out[k] = ' '
            i = j
        else:
            i += 1
    return ''.join(out)


def cmd_wrap(args):
    for path in source_files(args):
        with open(path, encoding='utf-8', newline='') as fp:
            code = fp.read()
        new, n = wrap_text(code)
        if n and '#include "database.h"' not in new:
            new = _add_include(new, 'database.h')
        if new != code:
            with open(path, 'w', encoding='utf-8', newline='') as fp:
                fp.write(new)
        print(f'{path}: {n} 箇所を包んだ')


def _add_include(code, header):
    """#include 群のアルファベット順の位置に header を足す。"""
    lines = code.split('\n')
    incs = [i for i, l in enumerate(lines)
            if re.match(r'#include "(?!AppHdr)', l)]
    if not incs:
        return code
    nl = '\r' if lines[0].endswith('\r') else ''
    pos = incs[-1] + 1
    for i in incs:
        name = re.match(r'#include "([^"]+)"', lines[i]).group(1)
        if name.lower() > header.lower():
            pos = i
            break
    lines.insert(pos, f'#include "{header}"{nl}')
    return '\n'.join(lines)


def main():
    if not os.path.isdir(SRC):
        sys.exit('リポジトリのルートで実行してください')
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    cmd, args = sys.argv[1], sys.argv[2:]
    sys.stdout.reconfigure(encoding='utf-8')
    {'old': cmd_old, 'olddict': cmd_olddict, 'find': cmd_find,
     'check': cmd_check, 'harvest': cmd_harvest, 'wrap': cmd_wrap}[cmd](args)


if __name__ == '__main__':
    main()
