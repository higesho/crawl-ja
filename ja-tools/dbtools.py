#!/usr/bin/env python3
"""説明文・DB テキスト(dat/descript, dat/database)の訳の進み具合を調べるツール。

リポジトリのルートで実行する: python -I ja-tools/dbtools.py <サブコマンド> ...

  status [DIR[/FILE]]   ファイルごとの 英語の項目数 / 訳あり / 要再翻訳 / 未訳
                        (DIR は descript か database)
  todo DIR/FILE [N]     未訳・要再翻訳の項目を英文つきで表示(N 件まで)。
                        要再翻訳は ja-work/stale/ の旧訳も添える
  keys DIR/FILE         未訳・要再翻訳のキーだけを 1 行ずつ表示

判定:
  訳あり    ja/ に同じキーがあり、"# en:" の英文が今の英文と(空白を無視して)
            同じもの。"# en:" がなく本文に日本語を含む項目も訳ありとみなす
  要再翻訳  ja/ にあるが "# en:" の英文が今の英文と違う、または ja/ にはなく
            ja-work/stale/ に旧訳があるもの
  未訳      それ以外
  要確認    訳ありのうち、本家 0.34 の ja/ から取り込んだ訳(ja-work/review/ にある項目)。
            元の英文が不明で、古い英文の訳のことがある。今の英文と見比べて直し、
            直したら ja-work/review/ の該当項目を消す

訳の書き方(既存の ja/*.txt と同じ):

  %%%%
  <キー(英語版と同じ)>

  # en:
  #   <今の英文(1 行ずつ "#   " を付ける)>
  <訳文>
  %%%%
"""
import os
import re
import sys

sys.stdout.reconfigure(encoding='utf-8')

DAT = 'crawl-ref/source/dat'
WORK = 'ja-work'
JA_RE = re.compile('[぀-ヿ一-鿿]')


def norm(s):
    return re.sub(r'\s+', ' ', s or '').strip().lower()


def parse(text):
    """database.cc と同じ規則で [(キー, 本文, コメント)] にする。"""
    out = []
    key, body, comment, in_entry = None, [], [], False

    def flush():
        if key:
            b = list(body)
            while b and not b[0].strip():
                b.pop(0)
            while b and not b[-1].strip():
                b.pop()
            out.append((key, '\n'.join(b), '\n'.join(comment)))

    for line in text.split('\n'):
        if line.startswith('%%%%'):
            flush()
            key, body, comment, in_entry = None, [], [], True
            continue
        if line.startswith('#'):
            if key is not None:
                comment.append(line)
            continue
        if not in_entry:
            continue
        if key is None:
            key = line.strip() or None
        else:
            body.append(line.rstrip())
    flush()
    return out


def read(path):
    if not os.path.exists(path):
        return []
    with open(path, encoding='utf-8', errors='replace') as fp:
        return parse(fp.read().replace('\r\n', '\n'))


def en_comment(comment):
    """"# en:" ブロックの英文を取り出す(なければ None)。"""
    lines = comment.split('\n')
    for i, l in enumerate(lines):
        if l.strip() == '# en:':
            got = []
            for l2 in lines[i + 1:]:
                if l2.rstrip() == '#':
                    got.append('')       # 空行は "#" だけでもよい
                    continue
                m = re.match(r'#   ?(.*)$', l2)
                if not m:
                    break
                got.append(m.group(1))
            return '\n'.join(got)
    return None


def classify(d, f):
    # 同じキーが複数あると database.cc(DBM_REPLACE)では後の項目が使われる
    en_all = read(f'{DAT}/{d}/{f}')
    last = {k.lower(): i for i, (k, b, c) in enumerate(en_all)}
    en = [e for i, e in enumerate(en_all) if last[e[0].lower()] == i]
    ja = {k.lower(): (b, c) for k, b, c in read(f'{DAT}/{d}/ja/{f}')}
    stale = {k.lower(): (b, c) for k, b, c in read(f'{WORK}/stale/{d}/{f}')}
    # 本家 0.34 の ja/ から取り込んだ訳(元の英文が不明で、古い英文の訳のことがある)。
    # 見直したら ja-work/review/ の該当項目を消す。
    review = {k.lower() for k, b, c in read(f'{WORK}/review/{d}/{f}')}
    rows = []
    for k, b, _ in en:
        j = ja.get(k.lower())
        e = en_comment(j[1]) if j else None
        if j and e is not None and norm(e) == norm(b):
            # 今の英文に対する訳(参照だけの値など、日本語を含まなくてもよい)
            state = 'ok'
        elif j and JA_RE.search(j[0]):
            state = 'ok' if e is None else 'stale'
        elif k.lower() in stale:
            state = 'stale'
        else:
            state = 'missing'
        if state == 'ok' and k.lower() in review:
            state = 'review'
        rows.append((k, b, state, stale.get(k.lower())))
    return rows


def files(arg):
    if arg and '/' in arg:
        d, f = arg.split('/', 1)
        return [(d, f)]
    dirs = [arg] if arg else ['descript', 'database']
    out = []
    for d in dirs:
        for f in sorted(os.listdir(f'{DAT}/{d}')):
            if f.endswith('.txt'):
                out.append((d, f))
    return out


def cmd_status(args):
    tot = [0, 0, 0, 0, 0]
    for d, f in files(args[0] if args else None):
        rows = classify(d, f)
        c = [len(rows), sum(r[2] == 'ok' for r in rows),
             sum(r[2] == 'stale' for r in rows), sum(r[2] == 'missing' for r in rows),
             sum(r[2] == 'review' for r in rows)]
        tot = [a + b for a, b in zip(tot, c)]
        if c[2] or c[3] or c[4] or args:
            print(f'{d}/{f}: 項目 {c[0]}  訳あり {c[1]}  要再翻訳 {c[2]}  未訳 {c[3]}  要確認 {c[4]}')
    print(f'合計: 項目 {tot[0]}  訳あり {tot[1]}  要再翻訳 {tot[2]}  未訳 {tot[3]}  要確認 {tot[4]}')


def cmd_todo(args):
    d, f = args[0].split('/', 1)
    limit = int(args[1]) if len(args) > 1 else 10 ** 9
    n = 0
    for k, b, state, old in classify(d, f):
        if state == 'ok':
            continue
        print('%%%%')
        print(f'[{state}] {k}')
        print(b)
        if old:
            print('--- ja-work/stale の旧訳と旧英文 ---')
            print(old[1])
        n += 1
        if n >= limit:
            break
    print('%%%%')


def cmd_keys(args):
    d, f = args[0].split('/', 1)
    for k, b, state, old in classify(d, f):
        if state != 'ok':
            print(f'{state}\t{k}')


def main():
    if not os.path.isdir(DAT):
        sys.exit('リポジトリのルートで実行してください')
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    cmd, args = sys.argv[1], sys.argv[2:]
    {'status': cmd_status, 'todo': cmd_todo, 'keys': cmd_keys}[cmd](args)


if __name__ == '__main__':
    main()
