#!/usr/bin/env python3
"""ja-work/new/ に分けて書いた訳を dat/<dir>/ja/<file> に結合する。

リポジトリのルートで実行する:
  python -I ja-tools/dbmerge.py descript/monsters.txt ja-work/new/monsters-1.txt [...]

同じキーが ja/ にあればその項目を置き換え、なければ英語版の並びに近い位置ではなく
末尾に足す(ゲームはキーで引くので順序は問わない)。項目の書式はそのまま写す。
"""
import os
import sys

sys.stdout.reconfigure(encoding='utf-8')
DAT = 'crawl-ref/source/dat'


def split_entries(text):
    """%%%% で区切られた項目を [(キー, 項目の全行)] にする(# コメントも含めて保持)。"""
    head, entries = [], []
    cur = None
    for line in text.split('\n'):
        if line.startswith('%%%%'):
            if cur is not None:
                entries.append(cur)
            cur = []
            continue
        if cur is None:
            head.append(line)
        else:
            cur.append(line)
    if cur is not None and any(l.strip() for l in cur):
        entries.append(cur)
    out = []
    for lines in entries:
        key = None
        for l in lines:
            if l.startswith('#') or not l.strip():
                continue
            key = l.strip()
            break
        if key:
            while lines and not lines[-1].strip():
                lines.pop()
            out.append((key, lines))
    return head, out


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    target = f'{DAT}/{sys.argv[1].split("/")[0]}/ja/{sys.argv[1].split("/", 1)[1]}'
    with open(target, encoding='utf-8') as fp:
        head, base = split_entries(fp.read().replace('\r\n', '\n'))
    index = {k.lower(): i for i, (k, _) in enumerate(base)}
    replaced = added = 0
    for src in sys.argv[2:]:
        with open(src, encoding='utf-8') as fp:
            _, new = split_entries(fp.read().replace('\r\n', '\n'))
        for k, lines in new:
            i = index.get(k.lower())
            if i is None:
                index[k.lower()] = len(base)
                base.append((k, lines))
                added += 1
            else:
                base[i] = (k, lines)
                replaced += 1
    with open(target, 'w', encoding='utf-8', newline='\n') as fp:
        if any(l.strip() for l in head):
            fp.write('\n'.join(head).rstrip('\n') + '\n')
        for k, lines in base:
            fp.write('%%%%\n' + '\n'.join(lines) + '\n')
        fp.write('%%%%\n')
    print(f'{target}: 置き換え {replaced}  追加 {added}  合計 {len(base)}')


if __name__ == '__main__':
    main()
