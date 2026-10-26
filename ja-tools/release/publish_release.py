"""GitHub のリリースに配布ファイルを上げ、必要なら公開する。

  python -I ja-tools/release/publish_release.py <版> <配布ファイルのフォルダ> [--publish]

例: python -I ja-tools/release/publish_release.py 0.34.1-ja6 ../release --publish

- 認証は git が push に使っているもの(git credential fill、host=github.com)。値は表示しない。
- 版のタグのリリース(下書きを含む)を探し、無ければ下書きで作る。題名は
  「DCSS 0.34.1 日本語版 jaN」、本文は ja-tools/release/<版>.md。
- 配布ファイル 3 つ(tiles-win64.zip・android-arm64.apk・src.zip)を上げる。同じ名前で
  同じ大きさのものがあれば上げ直さない。大きさが違えば消して上げ直す。
- --publish を付けると、本文をリリース文にそろえてから公開する(latest にする)。
- 終わったら、添付の大きさが手元と同じことと、公開の状態を表示する。
"""
import json
import os
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request

sys.stdout.reconfigure(encoding='utf-8')
REPO = 'higesho/crawl-ja'
API = f'https://api.github.com/repos/{REPO}'
KINDS = [('tiles-win64.zip', 'application/zip'),
         ('android-arm64.apk', 'application/vnd.android.package-archive'),
         ('src.zip', 'application/zip')]


def token():
    out = subprocess.run(['git', 'credential', 'fill'],
                         input='protocol=https\nhost=github.com\n\n',
                         capture_output=True, text=True, timeout=30,
                         env=dict(os.environ, GIT_TERMINAL_PROMPT='0')).stdout
    cred = dict(l.split('=', 1) for l in out.splitlines() if '=' in l)
    if not cred.get('password'):
        sys.exit('GitHub の認証が見つからない(git credential fill)')
    return cred['password']


TOKEN = token()


def call(method, url, data=None, ctype='application/json', length=None):
    headers = {'Authorization': 'token ' + TOKEN,
               'Accept': 'application/vnd.github+json',
               'X-GitHub-Api-Version': '2022-11-28'}
    body = data
    if isinstance(data, (dict, list)):
        body = json.dumps(data).encode('utf-8')
    if body is not None:
        headers['Content-Type'] = ctype
        if length is not None:
            headers['Content-Length'] = str(length)
    req = urllib.request.Request(url, data=body, method=method, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=1800) as r:
            raw = r.read()
            return json.loads(raw) if raw else None
    except urllib.error.HTTPError as e:
        sys.exit(f'{method} {url.split("?")[0]} -> {e.code} {e.read()[:300]!r}')


def find_release(tag):
    page = 1
    while True:
        rels = call('GET', f'{API}/releases?per_page=50&page={page}')
        if not rels:
            return None
        for r in rels:
            if r['tag_name'] == tag:
                return r
        page += 1


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    publish = '--publish' in sys.argv
    if len(args) != 2:
        sys.exit(__doc__)
    ver, folder = args
    here = os.path.dirname(os.path.abspath(__file__))
    notes = open(os.path.join(here, f'{ver}.md'), encoding='utf-8').read()
    name = 'DCSS 0.34.1 日本語版 ' + ver.rsplit('-', 1)[1]

    files = []
    for kind, ctype in KINDS:
        p = os.path.join(folder, f'stone_soup-{ver}-{kind}')
        if not os.path.isfile(p):
            sys.exit(f'配布ファイルが無い: {p}')
        files.append((p, ctype))

    rel = find_release(ver)
    if rel is None:
        rel = call('POST', f'{API}/releases',
                   {'tag_name': ver, 'name': name, 'body': notes, 'draft': True})
        print('下書きを作った:', rel['name'])
    else:
        print('リリース:', rel['name'], '(下書き)' if rel['draft'] else '(公開済み)')

    assets = {a['name']: a for a in rel['assets']}
    for p, ctype in files:
        fname = os.path.basename(p)
        size = os.path.getsize(p)
        a = assets.get(fname)
        if a and a['size'] == size and a['state'] == 'uploaded':
            print(f'  そのまま: {fname} ({size:,} バイト)')
            continue
        if a:
            call('DELETE', f'{API}/releases/assets/{a["id"]}')
            print(f'  古いものを消した: {fname} ({a["size"]:,} バイト)')
        up = rel['upload_url'].split('{')[0] + '?name=' + urllib.parse.quote(fname)
        with open(p, 'rb') as f:
            call('POST', up, data=f, ctype=ctype, length=size)
        print(f'  上げた: {fname} ({size:,} バイト)')

    if publish:
        rel = call('PATCH', f'{API}/releases/{rel["id"]}',
                   {'name': name, 'body': notes, 'draft': False, 'make_latest': 'true'})
        print('公開した:', rel['html_url'])

    rel = call('GET', f'{API}/releases/{rel["id"]}')
    ok = True
    remote = {a['name']: a['size'] for a in rel['assets']}
    for p, _ in files:
        fname = os.path.basename(p)
        same = remote.get(fname) == os.path.getsize(p)
        ok &= same
        print(f'  確認: {fname} {"同じ" if same else "違う"} ({remote.get(fname)})')
    body_same = rel['body'].replace('\r\n', '\n').rstrip() == notes.rstrip()
    print('本文:', 'リリース文と同じ' if body_same else 'リリース文と違う')
    print('状態:', '下書き' if rel['draft'] else '公開', rel['html_url'])
    if not ok:
        sys.exit(1)


main()
