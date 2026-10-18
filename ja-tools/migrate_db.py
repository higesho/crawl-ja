#!/usr/bin/env python3
"""旧日本語版の説明文・DBテキストの訳を 0.34 の英語データへ項目単位で移す。

訳元(優先順):
  ja19   dplusplus/dcss-0.19-ja の dat/{descript,database}/ja/
  ja16   dplusplus/dcss-0.16-ja の同上
  jtrans 0.19-ja / 0.16-ja の jtrans 辞書(0.34 で C++ からデータへ移った文章用)
  upstream 0.34 公式の ja/(Transifex 由来。Shift_JIS が混じる)

採用ルール(キーごと):
  - ja19/ja16: 訳が日本語を含み、訳したときの英文(訳文中の "# 英文" コメント、
    なければその版の英語データ)が 0.34 の英文と空白を無視して同一なら採用
  - jtrans: 0.34 の英文が辞書のキーと同一なら採用
  - upstream: 日本語を含めば採用(元の英文が不明なので ja-work/review/ に記録)
  - 英文が変わった旧訳は採用せず ja-work/stale/ へ(再翻訳待ち)
  - どこにも訳がなければ ja-work/missing/ へ
採用しなかった項目はゲーム中では英語で表示される(TextDB のフォールバック)。

使い方: リポジトリのルートで python -I ja-tools/migrate_db.py
"""
import collections
import json
import os
import re
import shutil
import subprocess
import sys

NEW = 'origin/stone_soup-0.34'
SOURCES = [
    # (名前, 訳の ref, 訳したときの英語の ref)
    ('ja19', 'ja19/develop', 'origin/stone_soup-0.19'),
    ('ja16', 'ja16/develop', 'origin/stone_soup-0.16'),
]
JTRANS_REFS = ['ja19/develop', 'ja16/develop']
DAT = 'crawl-ref/source/dat'
DIRS = ['descript', 'database']
OUT_WORK = 'ja-work'

# database.cc の AllDBs と同じまとまり。キーはこの単位でのみ一意。
GROUPS = {
    'descript': [
        ['features.txt', 'items.txt', 'unident.txt', 'unrand.txt',
         'monsters.txt', 'spells.txt', 'gods.txt', 'branches.txt',
         'skills.txt', 'ability.txt', 'cards.txt', 'commands.txt',
         'clouds.txt', 'status.txt', 'monstatus.txt', 'mutations.txt',
         'passives.txt'],
        ['species.txt', 'backgrounds.txt'],
        ['quotes.txt'],
        ['hints.txt', 'tutorial.txt'],
        ['egos.txt'],
    ],
    'database': [
        ['randname.txt', 'rand_wpn.txt', 'rand_arm.txt', 'rand_all.txt',
         'randbook.txt'],
        ['monspeak.txt', 'monspell.txt', 'monflee.txt', 'wpnnoise.txt',
         'insult.txt', 'godspeak.txt', 'monname.txt', 'colourname.txt',
         'graffiti.txt', 'miscast.txt'],
        ['shout.txt', 'insult.txt'],
        ['miscname.txt', 'godname.txt', 'montitle.txt', 'decorlines.txt',
         'monname.txt', 'colourname.txt', 'graffiti.txt', 'gizmo.txt'],
        ['help.txt'],
        ['FAQ.txt'],
    ],
}

JA_RE = re.compile('[぀-ヿ一-鿿]')


def has_ja(s):
    """英語のまま残っている「訳」を除外するため、日本語を含むか見る。"""
    return bool(JA_RE.search(s or ''))


def norm(s):
    return re.sub(r'\s+', ' ', s or '').strip().lower()


def group_of(d, f):
    files = set()
    for g in GROUPS[d]:
        if f in g:
            files.update(g)
    return files or {f}


_cache = {}


def git(*args):
    if args in _cache:
        return _cache[args]
    r = subprocess.run(['git', *args], capture_output=True)
    if r.returncode != 0:
        text = None
    else:
        try:
            text = r.stdout.decode('utf-8')
        except UnicodeDecodeError:
            # 0.34 公式の ja/ には Shift_JIS のファイルが混じっている
            text = r.stdout.decode('cp932', 'replace')
        text = text.replace('\r\n', '\n')
    _cache[args] = text
    return text


def list_txt(ref, path):
    out = git('ls-tree', '--name-only', ref, path + '/')
    if not out:
        return []
    return [p.rsplit('/', 1)[1] for p in out.split('\n')
            if p.endswith('.txt')]


class Entry:
    def __init__(self, key, body, comment):
        self.key = key          # 大文字小文字を保持したキー行
        self.body = body        # 本文(# 行を除く、前後の空行なし)
        self.comment = comment  # キーの後にある # 行(訳文の原文)


def parse(text):
    """database.cc の _parse_text_db と同じ規則でエントリを読む。
    キーより後の # 行は Entry.comment に集める。"""
    entries = []
    if text is None:
        return entries
    key = None
    body, comment = [], []
    in_entry = False

    def flush():
        if key:
            b = list(body)
            while b and not b[0].strip():
                b.pop(0)
            while b and not b[-1].strip():
                b.pop()
            entries.append(Entry(key, '\n'.join(b), '\n'.join(comment)))

    for line in text.split('\n'):
        if line.startswith('%%%%'):
            flush()
            key, body, comment, in_entry = None, [], [], True
            continue
        if line.startswith('#'):
            if key is not None:
                comment.append(re.sub(r'^#\s?', '', line))
            continue
        if not in_entry:
            continue
        if key is None:
            key = line.strip() or None
        else:
            body.append(line.rstrip())
    flush()
    return entries


def file_map(ref, path, only):
    """ref の path 以下のファイル(only で絞る)を key(小文字) -> Entry に。"""
    m = {}
    for f in list_txt(ref, path):
        if f not in only:
            continue
        for e in parse(git('show', f'{ref}:{path}/{f}')):
            m.setdefault(e.key.lower(), e)
    return m


def jtrans_dict():
    """旧版の jtrans 辞書(と montitle)を 正規化英文 -> 訳 にする。"""
    path = f'{DAT}/database/ja'
    m = {}
    for ref in JTRANS_REFS:
        for f in list_txt(ref, path):
            if not (f.startswith('jtrans') or f == 'montitle.txt'):
                continue
            for e in parse(git('show', f'{ref}:{path}/{f}')):
                if has_ja(e.body):
                    m.setdefault(norm(e.key.replace('\\n', ' ')), e.body)
    return m


def comment_block(label, text):
    return f'# {label}:\n' + ''.join(f'#   {l}\n' for l in text.split('\n'))


def write_entries(path, rows):
    if not rows:
        if os.path.exists(path):
            os.remove(path)
        return
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8', newline='\n') as fp:
        for key, text in rows:
            fp.write(f'%%%%\n{key}\n\n{text}\n')
        fp.write('%%%%\n')


def main():
    if not os.path.isdir(DAT):
        sys.exit('run from repository root')

    jdict = jtrans_dict()
    for kind in ('stale', 'missing', 'review'):
        shutil.rmtree(f'{OUT_WORK}/{kind}', ignore_errors=True)

    summary = {}
    for d in DIRS:
        stats_d = {}
        for f in list_txt(NEW, f'{DAT}/{d}'):
            grp = group_of(d, f)
            srcs = [(name,
                     file_map(ja_ref, f'{DAT}/{d}/ja', grp),
                     file_map(en_ref, f'{DAT}/{d}', grp))
                    for name, ja_ref, en_ref in SOURCES]
            up = {e.key.lower(): e for e in
                  parse(git('show', f'{NEW}:{DAT}/{d}/ja/{f}'))}

            out, stale, missing, review = [], [], [], []
            st = collections.Counter()
            for e in parse(git('show', f'{NEW}:{DAT}/{d}/{f}')):
                k = e.key.lower()
                new_en = e.body
                picked = None
                stale_cands = []
                for name, ja, en in srcs:
                    je = ja.get(k)
                    if not je or not has_ja(je.body):
                        continue
                    old_en = je.comment if je.comment.strip() else (
                        en[k].body if k in en else '')
                    if norm(old_en) == norm(new_en):
                        picked = (name, je.body)
                        break
                    stale_cands.append((name, old_en, je.body))
                if not picked and norm(new_en) in jdict:
                    picked = ('jtrans', jdict[norm(new_en)])
                if not picked and k in up and has_ja(up[k].body):
                    picked = ('upstream', up[k].body)
                    review.append((e.key, comment_block('en (0.34)', new_en)
                                   + '\n' + up[k].body))

                if picked:
                    name, ja_text = picked
                    st[name] += 1
                    out.append((e.key, comment_block('en', new_en) + ja_text))
                elif stale_cands:
                    st['stale'] += 1
                    txt = ''
                    for name, old_en, old_ja in stale_cands:
                        txt += comment_block(f'old en ({name})', old_en)
                        txt += comment_block(f'old ja ({name})', old_ja)
                    stale.append((e.key, txt + '\n' + new_en))
                else:
                    st['missing'] += 1
                    missing.append((e.key, new_en))
                st['total'] += 1
            stats_d[f] = dict(st)

            write_entries(f'{DAT}/{d}/ja/{f}', out)
            write_entries(f'{OUT_WORK}/stale/{d}/{f}', stale)
            write_entries(f'{OUT_WORK}/missing/{d}/{f}', missing)
            write_entries(f'{OUT_WORK}/review/{d}/{f}', review)
        summary[d] = stats_d

    os.makedirs(OUT_WORK, exist_ok=True)
    with open(f'{OUT_WORK}/migrate_summary.json', 'w', encoding='utf-8',
              newline='\n') as fp:
        json.dump(summary, fp, indent=1, ensure_ascii=False, sort_keys=True)

    cols = [n for n, _, _ in SOURCES] + ['jtrans', 'upstream', 'stale',
                                         'missing']
    for d, stats_d in summary.items():
        tot = collections.Counter()
        print(f'== {d}')
        print(f'  {"file":18} {"total":>5} ' + ' '.join(f'{c:>8}' for c in cols))
        for f, st in sorted(stats_d.items()):
            tot.update(st)
            print(f'  {f:18} {st.get("total", 0):5} '
                  + ' '.join(f'{st.get(c, 0):8}' for c in cols))
        print(f'  {"TOTAL":18} {tot["total"]:5} '
              + ' '.join(f'{tot[c]:8}' for c in cols))


if __name__ == '__main__':
    main()
