# 名前を返す関数の結果で辞書を引いている所について、その関数が返しうる英語の
# リテラルに訳があるかを調べる(今回の get_short_name → "ice" の型)。
# crawl-ref/source で実行する。
import re, glob, collections, sys
sys.stdout.reconfigure(encoding='utf-8')
MAC = {'zap_name_j': '[zap]', 'zap_name_jc': '[zap]',
       'branch_name_j': '[branch]', 'branch_name_jc': '[branch]',
       'verb_j': '[verb]', 'verb_jc': '[verb]', 'adj_j': '[adj]', 'adj_jc': '[adj]',
       'adv_j': '[adv]', 'adv_jc': '[adv]'}
pat = re.compile(r'\b(tagged_jtransc?|jtransc?|jtrans_notrimc?|' + '|'.join(MAC) + r')\s*\(')
files = glob.glob('*.cc') + glob.glob('*.h')
srcs = {f: open(f, encoding='utf-8', errors='replace').read() for f in files}


def close(s, i):
    d = 1
    while i < len(s) and d:
        if s[i] == '(':
            d += 1
        elif s[i] == ')':
            d -= 1
        i += 1
    return i


sites = collections.defaultdict(set)   # 関数名 -> {(タグ, 場所)}
for f, s in srcs.items():
    for m in pat.finditer(s):
        e = close(s, m.end())
        arg = s[m.end():e - 1].strip()
        fn = m.group(1)
        tag = ''
        if fn.startswith('tagged'):
            a = arg.split(',', 1)
            mt = re.match(r'"([^"]*)"', a[0].strip())
            if not mt or len(a) < 2:
                continue
            tag = mt.group(1)
            arg = a[1].strip()
        elif fn in MAC:
            tag = MAC[fn]
        if arg.startswith('"'):
            continue
        mm = re.match(r'(?:[\w:]+(?:\.|->))*(\w+)\s*\(', arg)
        if mm:
            line = s.count('\n', 0, m.start()) + 1
            sites[mm.group(1)].add((tag, f'{f}:{line}'))

keys = set()
for p in glob.glob('dat/database/ja/jtrans/*.txt'):
    t = open(p, encoding='utf-8').read().replace('\r\n', '\n')
    for blk in t.split('%%%%\n'):
        lines = [l for l in blk.split('\n') if not l.startswith('#')]
        while lines and not lines[0].strip():
            lines.pop(0)
        if lines:
            keys.add(lines[0].strip().lower())


def has(k):
    return k.strip().lower() in keys


ret_lit = re.compile(r'return\s+"([^"\\]+)"\s*;')
out = []
for fn, ss in sorted(sites.items()):
    defre = re.compile(r'^[\w:<>,\s\*&]*?\b' + re.escape(fn)
                       + r'\s*\([^;{]*\)\s*(?:const\s*)?\{', re.M)
    lits = set()
    for f, s in srcs.items():
        for m in defre.finditer(s):
            b = m.end()
            d = 1
            i = b
            while i < len(s) and d:
                if s[i] == '{':
                    d += 1
                elif s[i] == '}':
                    d -= 1
                i += 1
            lits |= set(ret_lit.findall(s[b:i]))
    if not lits:
        continue
    for tag in sorted({t for t, _ in ss}):
        miss = sorted(l for l in lits
                      if not has(tag + l) and not has(l) and re.search('[A-Za-z]', l))
        if miss:
            where = sorted(w for t, w in ss if t == tag)[:3]
            out.append(f'{fn} {tag or "(タグなし)"} 未訳 {len(miss)}/{len(lits)}: '
                       f'{miss[:12]}  呼び出し: {where}')
print('\n'.join(out))
