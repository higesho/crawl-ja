#!/usr/bin/env python3
"""日本語版のリリース用 zip を作る(Windows のタイル版と、ソース一式)。

使い方(リポジトリのルートで、Release Tiles / x64 のビルドを済ませてから):
  python -I ja-tools/make_release.py <版> <出力先>
  例: python -I ja-tools/make_release.py 0.34.1-ja1 ../release

作るもの:
  stone_soup-<版>-tiles-win64.zip  crawl.exe と DLL、dat、docs、settings など
                                   (本家の Makefile の install と同じ並び)
  stone_soup-<版>-src.zip          git で管理しているファイルとサブモジュール
                                   (contrib)の中身。util/release_ver に版を書く
"""

import glob
import os
import shutil
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
CREF = os.path.join(ROOT, "crawl-ref")
SRC = os.path.join(CREF, "source")
VS_REDIST = r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC"
RUNTIME_DLLS = ["vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll"]
GAME_DLLS = ["SDL2.dll", "SDL2_image.dll", "libpng.dll"]
FONTS = ["DejaVuSans.ttf", "DejaVuSansMono.ttf"]


def git(*args, cwd=ROOT):
    return subprocess.check_output(["git", *args], cwd=cwd).decode("utf-8")


def copy(src, dst):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst)


def copy_glob(pattern, dstdir):
    files = glob.glob(pattern)
    if not files:
        sys.exit(f"見つからない: {pattern}")
    for f in files:
        copy(f, os.path.join(dstdir, os.path.basename(f)))


def copy_tree(src, dst):
    shutil.copytree(src, dst, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns(".gitignore", "*.db"))


def find_runtime_dir():
    # いちばん新しい VC ランタイム(新しいものは古いツールセットのビルドでも動く)
    for ver in sorted(os.listdir(VS_REDIST), reverse=True):
        crt = glob.glob(os.path.join(VS_REDIST, ver, "x64", "Microsoft.VC*.CRT"))
        if crt:
            return crt[0]
    sys.exit("VC ランタイムの再頒布用ファイルが見つからない")


def text_for_windows(src, dst):
    # メモ帳などで文字化けしないよう、BOM 付き UTF-8・CRLF にする
    with open(src, encoding="utf-8") as f:
        text = f.read()
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "w", encoding="utf-8-sig", newline="\r\n") as f:
        f.write(text)


def make_zip(srcdir, zippath):
    base = os.path.dirname(srcdir)
    with zipfile.ZipFile(zippath, "w", zipfile.ZIP_DEFLATED,
                         compresslevel=9) as z:
        for dirpath, _, files in os.walk(srcdir):
            for name in sorted(files):
                full = os.path.join(dirpath, name)
                z.write(full, os.path.relpath(full, base))


def build_binary(version, outdir):
    name = f"stone_soup-{version}-tiles-win64"
    dst = os.path.join(outdir, name)
    shutil.rmtree(dst, ignore_errors=True)
    os.makedirs(dst)

    copy(os.path.join(SRC, "crawl.exe"), os.path.join(dst, "crawl.exe"))
    for dll in GAME_DLLS:
        copy(os.path.join(SRC, dll), os.path.join(dst, dll))
    crt = find_runtime_dir()
    for dll in RUNTIME_DLLS:
        copy(os.path.join(crt, dll), os.path.join(dst, dll))

    dat = os.path.join(dst, "dat")
    for sub in ["des", "dlua", "database", "defaults", "dist_bones"]:
        copy_tree(os.path.join(SRC, "dat", sub), os.path.join(dat, sub))
    copy_glob(os.path.join(SRC, "dat", "clua", "*.lua"), os.path.join(dat, "clua"))
    copy_glob(os.path.join(SRC, "dat", "descript", "*.txt"),
              os.path.join(dat, "descript"))
    for lang in sorted(os.listdir(os.path.join(SRC, "dat", "descript"))):
        if os.path.isdir(os.path.join(SRC, "dat", "descript", lang)):
            copy_glob(os.path.join(SRC, "dat", "descript", lang, "*.txt"),
                      os.path.join(dat, "descript", lang))
    copy_glob(os.path.join(SRC, "dat", "tiles", "*.png"), os.path.join(dat, "tiles"))
    for font in FONTS:
        copy(os.path.join(SRC, "contrib", "fonts", font),
             os.path.join(dat, "tiles", font))

    docs = os.path.join(dst, "docs")
    copy_glob(os.path.join(CREF, "docs", "*.txt"), docs)
    copy_glob(os.path.join(CREF, "docs", "*.md"), docs)
    for sub in ["develop", os.path.join("develop", "levels"), "license", "ja"]:
        copy_glob(os.path.join(CREF, "docs", sub, "*.txt"), os.path.join(docs, sub))
    copy(os.path.join(CREF, "CREDITS.txt"), os.path.join(docs, "CREDITS.txt"))
    copy_tree(os.path.join(CREF, "settings"), os.path.join(dst, "settings"))

    copy(os.path.join(ROOT, "LICENSE"), os.path.join(dst, "LICENSE"))
    text_for_windows(os.path.join(HERE, "release", "README-ja.txt"),
                     os.path.join(dst, "README-ja.txt"))

    zpath = os.path.join(outdir, name + ".zip")
    make_zip(dst, zpath)
    return zpath


def build_source(version, outdir):
    name = f"stone_soup-{version}-src"
    top = f"stone_soup-{version}"
    zpath = os.path.join(outdir, name + ".zip")
    files = [f for f in git("ls-files", "-z").split("\0") if f]
    subs = [l.split()[1] for l in git("submodule", "status").splitlines()]
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for f in files:
            full = os.path.join(ROOT, f)
            if f in subs or os.path.isdir(full):
                continue
            if f.startswith("crawl-ref/git-hooks/"):
                continue
            z.write(full, f"{top}/{f}")
        for sub in subs:
            subdir = os.path.join(ROOT, sub)
            for f in git("ls-files", "-z", cwd=subdir).split("\0"):
                full = os.path.join(subdir, f)
                if f and os.path.isfile(full):
                    z.write(full, f"{top}/{sub}/{f}")
        # git のないところでもビルドできるよう、版を書いておく(util/gen_ver.pl が読む)
        z.writestr(f"{top}/crawl-ref/source/util/release_ver", version + "\n")
    return zpath


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    version, outdir = sys.argv[1], os.path.abspath(sys.argv[2])
    os.makedirs(outdir, exist_ok=True)
    described = git("describe", "--tags").strip()
    if described != version:
        sys.exit(f"git describe が {described} で、{version} と合わない(タグを付けてから作る)")
    if git("status", "--porcelain", "--untracked-files=no").strip():
        sys.exit("コミットしていない変更がある")
    for p in [build_binary(version, outdir), build_source(version, outdir)]:
        print(f"{os.path.basename(p)}: {os.path.getsize(p) // 1024} KB")


if __name__ == "__main__":
    main()
