"""arena_sweep.ps1 の結果をまとめる: 辞書に無かったキーと、メッセージに残った英字。

  python -I ja-tools/untrans/arena_summary.py <出力フォルダ>

辞書に無かったキーには、訳済みの文をもう一度引いたもの(台詞の @The_monster@ など)や
"NO_TARGET" のような表示されないものも混じる。メッセージの「enters the arena!」などと
TALK: などの見出しは闘技場だけのものなので数えない。武器の銘の略記({flame}、rF+ など)は
0.19-ja と同じく英語のまま。
"""
import collections
import glob
import os
import re
import sys

sys.stdout.reconfigure(encoding='utf-8')
d = sys.argv[1]
miss = collections.Counter()
for p in glob.glob(os.path.join(d, 'w-*', 'missing.txt')):
    for line in open(p, encoding='utf-8', errors='replace'):
        miss[line.rstrip('\n')] += 1
print('== 辞書に無かったキー', len(miss))
for k, n in sorted(miss.items()):
    print(f'{n:3} {k}')

ign = re.compile(r'enters the arena!|is summoned!|^----|勝者|最終結果|^=+$|^\d+-\d+$|^\s')
eng = collections.Counter()
where = {}
for p in glob.glob(os.path.join(d, 'w-*', 'arena.result')):
    for line in open(p, encoding='utf-8', errors='replace'):
        line = line.rstrip('\n')
        line = re.sub(r'^(TALK|SOUND|WARN|ERROR|DIAG|PLAIN)\w*: ', '', line)
        if ign.search(line) or not re.search(r'[A-Za-z]{3,}', line):
            continue
        pat = re.sub(r'\d+', 'N', re.sub(r'『[^』]*』', '『名』', line))
        if not re.search(r'[A-Za-z]{3,}', pat):
            continue
        eng[pat] += 1
        where.setdefault(pat, os.path.basename(os.path.dirname(p)))
print('\n== メッセージに残った英字', len(eng))
for k, n in eng.most_common(200):
    print(f'{n:4} [{where[k]}] {k}')
