#!/usr/bin/env python3
"""ゲームパッドの状態機械(crawl-ref/source/gamepad.cc)の単体検査を組んで走らせる。

使い方(リポジトリのルートで): python -I ja-tools/gamepad_test/run.py

gamepad.cc はゲームの AppHdr.h を読むので、gamepad.h/.cc を一時ディレクトリへ写し、
中身の無い AppHdr.h と並べて cl.exe で組む(ゲームの本体は組まない)。
cl.exe は Visual Studio の vcvars64.bat で環境を整えてから呼ぶ。
"""

import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SOURCE = os.path.join(ROOT, "crawl-ref", "source")


def find_vcvars():
    pattern = (r"C:\Program Files\Microsoft Visual Studio\*\*"
               r"\VC\Auxiliary\Build\vcvars64.bat")
    found = glob.glob(pattern)
    if not found:
        sys.exit("vcvars64.bat が見つからない")
    # ゲームを組む MSBuild(STATUS.md の「ビルドと確認」)と同じ Visual Studio 18 を選ぶ。
    preferred = [p for p in found if os.sep + "18" + os.sep in p]
    return (preferred or sorted(found))[-1]


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    work = tempfile.mkdtemp(prefix="gamepad_test_")
    try:
        for name in ("gamepad.h", "gamepad.cc"):
            shutil.copy(os.path.join(SOURCE, name), work)
        shutil.copy(os.path.join(HERE, "test_gamepad.cc"), work)
        with open(os.path.join(work, "AppHdr.h"), "w", encoding="utf-8") as f:
            f.write("#pragma once\n")
        exe = os.path.join(work, "test_gamepad.exe")
        lines = [
            f'@call "{find_vcvars()}" >nul',
            f'@cd /d "{work}"',
            '@cl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX '
            f'test_gamepad.cc gamepad.cc /Fe:"{exe}"',
        ]
        bat = os.path.join(work, "build.bat")
        with open(bat, "w", encoding="cp932", newline="\r\n") as f:
            f.write("\n".join(lines) + "\n")
        build = subprocess.run(["cmd", "/c", bat], capture_output=True,
                               text=True, encoding="cp932", errors="replace")
        if build.returncode != 0:
            print(build.stdout)
            print(build.stderr)
            sys.exit("組めなかった")
        run = subprocess.run([exe], capture_output=True, text=True,
                             encoding="utf-8", errors="replace")
        print(run.stdout, end="")
        sys.exit(run.returncode)
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
