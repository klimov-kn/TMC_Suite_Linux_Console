#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Проверка регистра имён заголовков в дереве win_src.

ЗАЧЕМ. На Windows и на /mnt в WSL2 регистр имён файлов не важен, поэтому
несовпадение `#include <compfile.h>` с реальным файлом `COMPFILE.H` там не видно.
На родной ФС Linux (ext4) регистр важен — и сборка падает с
`fatal error: ...: No such file or directory`. Скрипт ловит такие места БЕЗ
сборки на Linux.

ЧТО ДЕЛАЕТ. Для каждой директивы #include ищет реальный файл по include-путям:
  * совпал точно            -> OK;
  * совпал только без учёта регистра -> ОШИБКА (соберётся лишь на Windows//mnt);
  * не найден               -> внешний заголовок (CRT/система), пропускаем.

ЗАПУСК (из корня проекта):
    python3 tools/check_case.py src/win_src

Код возврата: 0 — чисто, 1 — есть несовпадения (годится как гейт в CI).
"""
import os
import re
import sys
from collections import defaultdict

INC_RE = re.compile(rb'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)

# Каталоги поиска заголовков — как в src/CMakeLists.txt (target_include_directories).
SEARCH = ["Include", "kernels/PlanarRT_H", "kernels/PlanarRT_X", "viewers/Tmcrtout"]


def main() -> int:
    root = sys.argv[1] if len(sys.argv) > 1 else "src/win_src"
    if not os.path.isdir(root):
        print(f"Нет каталога: {root}")
        return 2

    # реальные файлы: нижний регистр пути -> как записано на диске
    real = {}
    for dirpath, _dirs, files in os.walk(root):
        for fn in files:
            p = os.path.relpath(os.path.join(dirpath, fn), root).replace("\\", "/")
            real[p.lower()] = p

    bad = defaultdict(list)
    for dirpath, _dirs, files in os.walk(root):
        for fn in files:
            if not fn.lower().endswith((".cpp", ".h", ".hpp")):
                continue
            src = os.path.join(dirpath, fn)
            owner = os.path.relpath(dirpath, root).replace("\\", "/")
            try:
                data = open(src, "rb").read()
            except OSError:
                continue
            for m in INC_RE.finditer(data):
                spell = m.group(1).decode("latin-1").strip().replace("\\", "/")
                exact = False
                hit = None
                for base in [owner] + SEARCH:
                    cand = os.path.normpath(f"{base}/{spell}").replace("\\", "/")
                    key = cand.lower()
                    if key in real:
                        if real[key] == cand:
                            exact = True
                        else:
                            hit = real[key]
                        break
                if not exact and hit:
                    rel = os.path.relpath(src, root).replace("\\", "/")
                    bad[hit].append((rel, spell))

    if not bad:
        print("OK: регистр всех #include совпадает с именами файлов на диске.")
        print("    Дерево соберётся на родной ФС Linux (ext4).")
        return 0

    total = sum(len(v) for v in bad.values())
    print("НАЙДЕНЫ НЕСОВПАДЕНИЯ РЕГИСТРА — на ext4 сборка упадёт:\n")
    for hit in sorted(bad):
        forms = sorted({s for _f, s in bad[hit]})
        print(f"  файл на диске: {hit}")
        print(f"    в #include пишут: {', '.join(forms)}")
        for f, s in sorted(bad[hit])[:5]:
            print(f"      - {f}: #include {s}")
        if len(bad[hit]) > 5:
            print(f"      ... ещё {len(bad[hit]) - 5}")
        print()
    print(f"ИТОГО: {total} вхождений, {len(bad)} заголовков.")
    print("Лечится переименованием файла под то, как его пишут исходники")
    print("(если пишут единообразно), иначе — правкой директив-исключений.")
    print("См. раздел «Регистр имён файлов» в ISSUES.md.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
