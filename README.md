<div align="center">

# TMC Suite — computation kernels for Linux

**Scientific software package for electrodynamic simulation of planar structures**

C++ · Linux (x64) · console, no graphics required

**English** · [Русский](#tmc-suite--счётные-ядра-для-linux)

</div>

---

## Overview

**TMC Suite** is an authorial scientific software package by **K. N. Klimov** for electrodynamic
computation and visualization of planar structures (H-polarization and X-mode). This repository is
the **Linux port of the computation kernels**: the solvers alone, without windows, for machines
that have nothing but a terminal — a compute server, a cluster node, a container.

The computational code is not rewritten. It is the Windows source tree compiled by `g++` as is; the
MFC layer the kernels were wrapped in is replaced by a thin console entry point. Results stay
compatible with the Windows version: a task calculated here opens in the Windows viewers, and the
other way round.

## Components

| Program | Purpose |
|---|---|
| `tmc_rth` | solver, H-polarization |
| `tmc_rtx` | solver, X-mode (incl. magnetized plasma) |

### Libraries
`sfile95` · `complex` · `exprint` · `TMCLibError` · `prepr` · `TMCIndan`

## What it looks like

```
$ tmc_rth --version
tmc_rth - counting kernel PlanarRT_H (H-mode), TMC Suite 4.1
built for Linux (x64), console version - no windows required

  site:   https://www.tamic.ru
  manual: TMC_Suite_User_Manual.pdf (next to this program)
          https://www.tamic.ru/docs/user-manual/index.html

$ tmc_rth /home/user/work/horn.tpl
Step:1;Error:no
$ ls
horn.tpl  horn.t  horn.tt  horn.s  horn.ex  horn.AMP  horn.FAZ  horn.DAT
```

## Installation

Both builds are published in [Releases](../../releases); nothing has to be installed.

The archive — the kernels, the licence and the user manual:

```
tar xzf tmc-suite-cli-linux-x86-64.tar.gz
cd tmc-suite-cli
./tmc_rth /full/path/task.tpl
```

The AppImage — the same thing as a single file, handy for a compute node or a container:

```
chmod +x TMC_Suite_CLI-1.0.0-x86_64.AppImage
./TMC_Suite_CLI-1.0.0-x86_64.AppImage tmc_rth /full/path/task.tpl
ln -s TMC_Suite_CLI-1.0.0-x86_64.AppImage tmc_rth && ./tmc_rth /full/path/task.tpl
```

Requirements: x64 and glibc 2.39 or newer — Ubuntu 24.04 LTS and compatible systems. No graphics
libraries are needed. If the system complains about FUSE, add `--appimage-extract-and-run`.

## Building from source

```
sudo apt install -y build-essential cmake ninja-build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DTMC_BUILD_KERNELS=ON
cmake --build build -j
```

The kernels appear in `dist/linux-x86-64/bin`.

> On a case-sensitive file system, run `python3 tools/check_case.py` before building: it compares
> every `#include` with the real file name on disk.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | the calculation completed |
| 1 | error: the task could not be opened or read, or the calculation stopped |
| 2 | the program was started without an argument |

## Input format

Simulations are described in **`.tpl`** text files (the TAMIC input language): geometry, materials,
sources, frequency sweeps and output requests. Always pass the **full path** to the task: the
directory it lives in is where included files and `.eps` distributions are looked for.

## Relation to the other ports

| Repository | What it is |
|---|---|
| [TMC Suite (Windows)](https://github.com/klimov-kn/TMC_Suite) | the original package, MFC / OpenGL |
| **this repository** | Linux, computation kernels without windows |
| [TMC Suite for Linux (graphical)](https://github.com/klimov-kn/TMC_Suite_Linux_GUI) | Linux, all six programs on Qt 6 |
| [TMC Suite for macOS](https://github.com/klimov-kn/TMC_Suite_MacOS) | macOS, universal build for Apple Silicon and Intel |

## Documentation

Full documentation is published on the project website: user manual, the `.tpl` task language
reference, headless operation, installation and architecture notes.

## Author & links

- **Author of the scientific code:** K. N. Klimov
- **Porting, build and documentation:** M. S. Matsayan
- **Website:** [www.tamic.ru](https://www.tamic.ru)

## License

See [`LICENSE`](LICENSE).

---
---

<div align="center">

# TMC Suite — счётные ядра для Linux

**Научный пакет для электродинамического моделирования планарных структур**

C++ · Linux (x64) · консоль, графика не нужна

[English](#tmc-suite--computation-kernels-for-linux) · **Русский**

</div>

---

## О пакете

**TMC Suite** — авторский научный пакет **К. Н. Климова** для электродинамического расчёта и
визуализации планарных структур (H-поляризация и X-мода). В этом репозитории — **перенос счётных
ядер на Linux**: только расчёт, без окон, для машин, где есть лишь терминал: счётного сервера,
узла кластера, контейнера.

Вычислительный код не переписывался. Это исходники Windows-версии, скомпилированные `g++` как есть;
обёртка MFC, в которую были завёрнуты ядра, заменена тонкой консольной точкой входа. Результаты
остаются совместимыми с Windows-версией: задание, посчитанное здесь, открывается в просмотрщиках
Windows-версии, и наоборот.

## Состав

| Программа | Назначение |
|---|---|
| `tmc_rth` | расчёт, H-поляризация |
| `tmc_rtx` | расчёт, X-мода (в том числе замагниченная плазма) |

### Библиотеки
`sfile95` · `complex` · `exprint` · `TMCLibError` · `prepr` · `TMCIndan`

## Как это выглядит

```
$ tmc_rth --version
tmc_rth - counting kernel PlanarRT_H (H-mode), TMC Suite 4.1
built for Linux (x64), console version - no windows required

  site:   https://www.tamic.ru
  manual: TMC_Suite_User_Manual.pdf (next to this program)
          https://www.tamic.ru/docs/user-manual/index.html

$ tmc_rth /home/user/work/horn.tpl
Step:1;Error:no
$ ls
horn.tpl  horn.t  horn.tt  horn.s  horn.ex  horn.AMP  horn.FAZ  horn.DAT
```

## Установка

Обе поставки опубликованы в разделе [Releases](../../releases), устанавливать ничего не нужно.

Архив — ядра, лицензия и руководство пользователя:

```
tar xzf tmc-suite-cli-linux-x86-64.tar.gz
cd tmc-suite-cli
./tmc_rth /полный/путь/задание.tpl
```

Образ AppImage — то же самое одним файлом, удобно для счётного узла или контейнера:

```
chmod +x TMC_Suite_CLI-1.0.0-x86_64.AppImage
./TMC_Suite_CLI-1.0.0-x86_64.AppImage tmc_rth /полный/путь/задание.tpl
ln -s TMC_Suite_CLI-1.0.0-x86_64.AppImage tmc_rth && ./tmc_rth /полный/путь/задание.tpl
```

Требования: x64 и glibc 2.39 или новее — Ubuntu 24.04 LTS и совместимые системы. Графические
библиотеки не нужны. Если система ругается на FUSE, добавьте ключ `--appimage-extract-and-run`.

## Сборка из исходников

```
sudo apt install -y build-essential cmake ninja-build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DTMC_BUILD_KERNELS=ON
cmake --build build -j
```

Ядра появятся в `dist/linux-x86-64/bin`.

> На файловой системе, различающей регистр имён, перед сборкой полезно выполнить
> `python3 tools/check_case.py`: инструмент сверяет каждый `#include` с настоящим именем файла.

## Коды возврата

| Код | Значение |
|---|---|
| 0 | расчёт прошёл |
| 1 | ошибка: задание не открылось, не прочиталось или счёт прервался |
| 2 | программа запущена без аргумента |

## Формат задания

Расчёт описывается текстовым файлом **`.tpl`** (входной язык TAMIC): геометрия, материалы,
источники, частотные свипы и состав выходных данных. Путь к заданию задавайте **полным**: по нему
определяется каталог, где лежат включаемые файлы и распределения `.eps`.

## Связь с другими сборками

| Репозиторий | Что это |
|---|---|
| [TMC Suite (Windows)](https://github.com/klimov-kn/TMC_Suite) | исходный пакет, MFC / OpenGL |
| **этот репозиторий** | Linux, счётные ядра без окон |
| [TMC Suite для Linux (графический)](https://github.com/klimov-kn/TMC_Suite_Linux_GUI) | Linux, все шесть программ на Qt 6 |
| [TMC Suite для macOS](https://github.com/klimov-kn/TMC_Suite_MacOS) | macOS, универсальная сборка для Apple Silicon и Intel |

## Документация

Полная документация опубликована на сайте проекта: руководство пользователя, справочник по языку
заданий `.tpl`, работа без окон, установка и описание архитектуры.

## Авторы и ссылки

- **Автор научного кода:** К. Н. Климов
- **Портирование, сборка, документация:** М. С. Мацаян
- **Сайт:** [www.tamic.ru](https://www.tamic.ru)

## Лицензия

См. [`LICENSE`](LICENSE).
