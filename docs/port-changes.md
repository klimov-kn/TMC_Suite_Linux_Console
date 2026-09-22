# Сверка исходников Windows ↔ Linux (рабочая запись)

Назначение файла — **точная запись всех различий** между Windows-исходниками и
Linux-версией. Это исходные данные для отчёта
**`Изменения_исходников_Windows-Linux.docx`** (п. 2 задания) и для
**`Инструкция_сборки_Linux_WSL2.docx`** (п. 3). Здесь — только факты сверки и
зависимости сборки, без повествования.

---

## 1. Метод сверки (как это проверено, чтобы ничего не упустить)

- **Эталон Windows:** `..\TMC_Suite\src`. **Linux-дерево:** `src\win_src` (тот же код,
  импортированный для порта).
- Проверка не «по памяти», а **сплошным хеш-сравнением** каждого файла обоих деревьев
  (MD5 по всем файлам, сравнение по относительному пути без учёта регистра). Метод
  ловит и невидимые правки (управляющие байты `0x1A`, замена `\`→`/`), которые
  текстовый `diff` показывает плохо.
- **Воспроизвести самому:** `diff -u ..\TMC_Suite\src\<файл> src\win_src\<файл>` для
  каждого файла из таблицы ниже.

**Результат сверки:** по содержимому отличаются **ровно 11 файлов** (список ниже).
Все прочие файлы, присутствующие только в Linux-дереве, — служебные артефакты
(`kernels\PlanarRT_H\graphify-out\*`, `*.prc` от прогона) и **исходным кодом не
являются** — в сборке не участвуют, в отчёт не входят.

> **Актуальность:** `src\win_src` пересинхронизирован с Windows-проектом **2026-07-16**
> (перенесены фиксы багов #12, #13, #15, #17 и проверка ε>0 — 10 файлов скопированы из
> `..\TMC_Suite\src` без изменений). Синхронизация касалась только файлов, которых наши
> правки не трогали. Метод разделения «Windows обновил» ↔ «мы правили» и полный список
> перенесённого — раздел «Синхронизация с Windows-проектом» в [../ISSUES.md](../ISSUES.md).
>
> **⚠️ Таблица ниже (11 файлов) описывает правки ПОРТА и остаётся верной, но с 2026-07-16
> это уже не полный список расхождений с Windows.** Актуальная картина:
>
> | Категория | Кол-во | Где описано |
> |---|---|---|
> | Правки порта (таблица ниже) | **11** | этот файл |
> | Правки регистра `#include` (`Sadd.cpp`, `TmcRTH_Indan.cpp`, 3 файла в `d3d`) | **+5** | ISSUES.md, «Регистр имён файлов» |
> | **Переименования** заголовков и каталогов (содержимое не менялось) | **68** | ISSUES.md, «Регистр имён файлов» |
> | GUI-обвязка ядер, где в Windows есть `.pre_closefix`, а мы её осознанно НЕ переносим (`MainFrm`, `ChildFrm`, `*Doc`, `*View` в H и X) | **12** | ISSUES.md, «Синхронизация» |
>
> Из-за переименований **сверять деревья нужно без учёта регистра пути** (метод в §1 это уже
> делает). Проверка регистра — `python3 tools/check_case.py src/win_src` (см. ISSUES.md).

---

## 2. Полный список изменённых файлов (11)

| # | Файл (путь от `src\`) | Библиотека/ядро, для которой нужен | CP1251 | Тип правки |
|---|---|---|:---:|---|
| 1 | `Include\Typedef.h` | complex (и все, общий заголовок) | — | тип `_complex` + макросы `__max`/`__min` |
| 2 | `Include\LISTING.H` | sfile95 (общий заголовок) | — | удалены байты `0x1A` (стр. 52, 53) |
| 3 | `Include\LABEL\RTH.H` | sfile95 (общий заголовок) | — | удалён байт `0x1A` (стр. 34) |
| 4 | `Include\S_file.h` | sfile95 | — | прототип `write_S` → `const void*` |
| 5 | `libs\SFILE95\S_FILE.CPP` | sfile95 | — | определение `write_S` → `const void*` |
| 6 | `libs\SFILE95\Sofftbl.cpp` | sfile95 | — | `fpos_t` → `long` (+ `ftell`) |
| 7 | `libs\SFILE95\Read_s.cpp` | sfile95 | — | `fpos_t lOffset` → `long` |
| 8 | `libs\SFILE95\Write_s.cpp` | sfile95 | 🅒 | `#include <label\…>` → `<label/…>` |
| 9 | `libs\PREPR\DEF_STR.CPP` | prepr | 🅒 | 4 защиты от чтения `lsplex[-1]` при EOF |
| 10 | `libs\TMCIndan\TmcRTH_IndanOutput.cpp` | TMCIndan | 🅒 | guard пустого пути в `AddPath` |
| 11 | `libs\TMCLibError\TmcLibError.cpp` | TMCLibError | — | `Format("%s",CString)` → присваивание |

🅒 — файл в кодировке **CP1251** (русские комментарии). Править **только** через
PowerShell в Latin1 (`String.Replace`), **не** Edit/perl/sed (иначе портится
кириллица). См. память проекта `edit-cp1251-files-safely`.

> Библиотеки **exprint**, **complex** (кроме общего `Typedef.h`) и сами **ядра**
> (`PlanarRT_H`, `PlanarRT_X`) — **в исходниках не правились**. Их совместимость
> достигнута новыми файлами слоя `src\compat` и настройками сборки (раздел 5).

---

## 3. Изменения по файлам (строка · было · стало · почему)

Данные строк ниже — прямой ввод для таблиц отчёта п. 2.

### 3.1 `Include\Typedef.h` (для complex; общий заголовок)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| после 54 (за `#include <float.h>`) | *(нет — `__max`/`__min` давал CRT MSVC)* | `#ifndef __max` / `#define __max(a,b) ((a)>(b)?(a):(b))` / `#endif` (и `__min` так же) | g++ не имеет `__max`/`__min`; макрос раскрывается в то же тернарное выражение. |
| после ~139 (ветка `__cplusplus`, за `#include <math.h>`) | *(нет — тип `_complex` приходил неявно из `<complex.h>` MSVC)* | `#ifndef __COMPLEX_DEFINED` / `#define __COMPLEX_DEFINED` / `typedef struct tag_complex { _real x; _real y; } _complex;` / `#endif` | На g++/Linux системного `_complex` нет; поля `x`/`y` взяты из C-ветки этого же файла и из `CMPLXALL.CPP`. |

### 3.2 `Include\LISTING.H` (для sfile95; общий заголовок)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| 52 | текст заголовка содержит управляющий байт `0x1A` (DOS-EOF) | байт `0x1A` удалён | g++: `stray '\32' in program` — внутри текста `0x1A` не EOF. |
| 53 | то же (`0x1A`) | байт `0x1A` удалён | то же. |

### 3.3 `Include\LABEL\RTH.H` (для sfile95; общий заголовок)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| 34 | управляющий байт `0x1A` | байт `0x1A` удалён | как 3.2 (вскрылось после правки 3.6). |

### 3.4 `Include\S_file.h` (sfile95)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| 97 | `int _far _fortran write_S( void _far *ptr, size_t size );` | `int _far _fortran write_S( const void _far *ptr, size_t size );` | g++ не приводит `const char*`→`void*`; `write_S` только читает буфер. |

### 3.5 `libs\SFILE95\S_FILE.CPP` (sfile95)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| 92 | `int _far _fortran write_S( void _far *ptr, size_t size )` | `int _far _fortran write_S( const void _far *ptr, size_t size )` | то же, что 3.4 (определение функции). Тело не менялось. |

### 3.6 `libs\SFILE95\Sofftbl.cpp` (sfile95)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| 34 | `fpos_t   fPos_t;` | `long     fPos_t;` | на glibc `fpos_t` — непрозрачная структура, нельзя складывать/кастовать. |
| 41 | `if( fgetpos( sf, &fPos_t ) != 0 ) return -1;` | `if( (fPos_t = ftell( sf )) == -1L ) return -1;` | получить целочисленное смещение конца файла. |
| 43 | `lOffset = (fpos_t)(8*sizeof(char)+3*sizeof(INT_1));` | `lOffset = (long)(8*sizeof(char)+3*sizeof(INT_1));` | приведение к целому, как требует g++. |

### 3.7 `libs\SFILE95\Read_s.cpp` (sfile95)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| 99 | `fpos_t  lNextPoint, lOffset;` | `fpos_t  lNextPoint;` + `long    lOffset;` | `lOffset` участвует в арифметике/`int`, поэтому тип разделён; `lNextPoint` остаётся `fpos_t` для `fgetpos`/`fsetpos`. |

### 3.8 🅒 `libs\SFILE95\Write_s.cpp` (sfile95)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| 108 | `#include <label\3d.h>` | `#include <label/3d.h>` | g++/POSIX требует `/` как разделитель пути в `#include`. |
| 112 | `#include <label\ic.h>` | `#include <label/ic.h>` | то же. |
| 116 | `#include <label\m.h>` | `#include <label/m.h>` | то же. |
| 120 | `#include <label\h.h>` | `#include <label/h.h>` | то же. |
| 124 | `#include <label\e.h>` | `#include <label/e.h>` | то же. |
| 128 | `#include <label\w.h>` | `#include <label/w.h>` | то же. |
| 131 | `#include <label\rth.h>` | `#include <label/rth.h>` | то же (единственная активная строка, не под `#ifdef`). |

### 3.9 🅒 `libs\PREPR\DEF_STR.CPP` (prepr)

После каждой строки `buf->inp_p = buf->ilex[buf->inp_p];` / `p = buf->ilex[p];`
индекс мог стать `-1` (EOF), и далее читался `lsplex[-1]`.

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| после 232 | *(нет проверки)* | `if( buf->inp_p < 0 ) { mac1[i] = '\0'; return( BUF_EOF ); }` | защита от чтения по индексу −1. |
| после 268 | *(нет проверки)* | `if( buf->inp_p < 0 ) { put_error_messege( "Small buffer" ); return( BUF_EOF ); }` | то же. |
| после 309 | *(нет проверки)* | `if( p < 0 ) return( BUF_EOF );` | то же. |
| после 326 | *(нет проверки)* | `if( p < 0 ) return( BUF_EOF );` | то же. |

Причина одна на 4 правки: на Windows чтение `lsplex[-1]` давало мусор соседней
памяти (обычно совпадал с `BUF_EOF`), на Linux — heap-overflow (ASan) / расхождение.

### 3.10 🅒 `libs\TMCIndan\TmcRTH_IndanOutput.cpp` (TMCIndan)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| после 355 (за `if( IsFullPath() ) return;`) | *(нет проверки)* | `if( csPath.GetLength() == 0 ) return;` | на Linux в пути нет `'\'` → `csPath` пуст → имя выхода становилось `"\poly.t"`. Guard оставляет чистое имя в CWD. |

### 3.11 `libs\TMCLibError\TmcLibError.cpp` (TMCLibError)

| Строка | Windows | Linux | Почему |
|---|---|---|---|
| ~100 (`operator=`) | `csError.Format( "%s", cError1.GetErrorMessage() );` | `csError = cError1.GetErrorMessage();` | передача объекта `CString` в варарг `%s` — UB на g++ (SysV ABI); присваивание эквивалентно по логике. |

> Во всех 11 правках **математика, формулы, константы, порядок вычислений и формат
> файлов TMC не затронуты** — менялись только типы в памяти, пути включения,
> управляющие байты и защиты от выхода за границу буфера.

---

## 4. Новые файлы для Linux (исходники Windows не трогаются)

Слой совместимости `src\compat\include\` (UTF-8). Подключается **только при `NOT
WIN32`** через INTERFACE-цель `tmc_compat` → `#include <afx.h>` и т.п. в исходниках
находят наш шим без правки самих исходников. На Windows используется настоящий MFC.

| Файл | Назначение | Нужен для |
|---|---|---|
| `tmc_mfc.h` | `CString` (тривиально-копируемый, один `char*`); win-типы `BOOL/UINT/DWORD/WORD/BYTE/ULONG/LPVOID/TCHAR/LP*`; `FILETIME`; пустые `far/_far/near/fortran/_pascal/_cdecl`; CRT (`<cstring>/<cstdlib>/<cstddef>`); `Sleep`, `SetFileAttributes`(no-op), `AfxMessageBox`→stderr | TMCLibError, TMCIndan, sfile95, ядра |
| `afx.h`, `afxwin.h`, `cstringt.h` | перенаправители на `tmc_mfc.h` | TMCLibError, TMCIndan, ядра |
| `windows.h` | → `tmc_mfc.h` (нужны лишь `BOOL/TRUE/FALSE`) | sfile95 |
| `conio.h`, `io.h`, `TMCGROUT.h` | пустые заглушки «мёртвых» include | exprint (conio), sfile95 (io), ядра (TMCGROUT) |
| `afxext.h`, `afxdisp.h`, `afxcmn.h`, `afxmt.h` | заглушки, тянет `StdAfx.h` ядра | ядра |
| `tmc_mfc_gui.h` | «болванки» GUI-классов MFC (CObject→…→CWnd→CFrameWnd/CView/CDialog/CWinApp + макросы) — чтобы GUI-заголовки парсились | ядра |

Слой `src\compat\` (реализация) и `src\cli\` (точки входа):

| Файл | Назначение | Нужен для |
|---|---|---|
| `compat\tmc_textmode.cpp` | `__wrap_fopen`: текстовый режим CRT Windows (чтение срезает `\r`, запись LF→CRLF; бинарные `"b"` — без изменений) | оба ядра |
| `cli\tmc_rth_main.cpp` | `main()` ядра H (повторяет `RunAll()` без MFC Doc/View) | tmc_rth |
| `cli\tmc_rtx_main.cpp` | `main()` ядра X | tmc_rtx |
| `cli\tmc_kernel_console.cpp` | общий: консольные замены GUI-колбэков + перенос `IsPointInTr` (байт-в-байт) | оба ядра |

---

## 5. Зависимости сборки (состав и порядок линковки)

### 5.1 Состав библиотек (какие `.cpp` входят в каждую `.a`)

Источник истины — `.vcxproj` соответствующей библиотеки (повторён в
`src\CMakeLists.txt`, функция `tmc_add_library`).

| Библиотека (`.a`) | Каталог(и) `win_src\` | Файлы `.cpp` |
|---|---|---|
| `complex` | `libs\complex` | `CMPLXALL.CPP` |
| `sfile95` | `libs\SFILE95` | `Errors, Frqdata, Read_s, S_file, S_memf, S_remt, S_table, S_wemt, Sadd, Sofftbl, Unite_s, Write_s, Wrrd_s` |
| `exprint` | `libs\Expr` + `libs\Inter` | `Expr_com, Expr_err, Expr_in1, Expr_per, Expremsg, Get_err, Inte_in1, Inte_ope, Inte_val` |
| `TMCLibError` | `libs\TMCLibError` | `TmcLibError` |
| `prepr` | `libs\PREPR` | `Def_dest, Def_er, Def_erd, Def_macr, Def_str, Defmis, Prep_ld, Prep_mn, Read_buf` |
| `TMCIndan` | `libs\TMCIndan` | `FieldIntegrated, TmcRTH_BolckList, TmcRTH_Indan, TmcRTH_IndanOutput, TmcRTH_IndanParam, TmcRTH_IndanTopology, TmcRTH_Input, TmcRTH_InputNode` |

Все библиотеки используют общие заголовки из `win_src\Include` и слой `tmc_compat`.

### 5.2 Зависимости между библиотеками

```
TMCIndan  ──►  prepr, exprint, TMCLibError
prepr     ──►  TMCLibError
complex, sfile95 — листовые (ни от кого из набора не зависят при сборке .a)
```

CMake выстраивает порядок сам. **Но при линковке ядра** обнаружен **цикл
`prepr ↔ sfile95`**: `S_MEMF.CPP` (sfile95) зовёт `put_error_messege` из prepr, а
`Prep_mn.cpp` (prepr) использует `alloc_mem_err`/`free_mem` из sfile95 (их
определения в `Prep_mn.cpp` закомментированы). Однопроходный GNU ld цикл не
разрешает → набор библиотек оборачивается в `-Wl,--start-group … --end-group`.

### 5.3 Состав и зависимости ядер

| Ядро | Исходники (компилируются в exe) | Линкует |
|---|---|---|
| `tmc_rth` | `kernels\PlanarRT_H\{TmcRTHNodeDiel, TmcRTHRectNode}.cpp` + `viewers\Tmcrtout\{TmcTtoS, TmcSMatrix}.cpp` + `cli\{tmc_rth_main, tmc_kernel_console}.cpp` + `compat\tmc_textmode.cpp` | все 6 библиотек (в `--start-group … complex --end-group`) + `tmc_compat` |
| `tmc_rtx` | `kernels\PlanarRT_X\{TmcRTHNodeDiel, TmcRTHRectNode}.cpp` + те же `viewers` + `cli\{tmc_rtx_main, tmc_kernel_console}.cpp` + `compat\tmc_textmode.cpp` | то же |

GUI/MFC-файлы (`MainFrm`, `*Doc`, `*View`, `Dialog*`, `Sound*`, `StdAfx`, `*.rc`,
`PL_GLFUN.CPP`, `Pl_iofor.cpp`) в сборку ядер **не входят**.

### 5.4 Настройки сборки (флаги — без правки исходников)

| Настройка | Где | Зачем |
|---|---|---|
| `add_compile_definitions(_PREC_DOUBLE)` | глобально (`cmake\TmcSettings.cmake`) | Баг #11: X-схема в float нестабильна. `_PREC_FLOAT` запрещён. |
| `-fpermissive` | цели `tmc_rth`, `tmc_rtx` | MSVC-«вольности» (extra-qualification, `(int)(void*)` для `mod 32`, литерал→`char*`) — понизить до предупреждений; семантика не меняется. |
| `-Wl,--start-group … complex --end-group` | линковка ядер | цикл `prepr ↔ sfile95`; `complex` нужен ядру напрямую. |
| `-Wl,--wrap=fopen` | линковка ядер | текстовый режим CRT Windows (CRLF↔LF) через `__wrap_fopen`. |
| 6 макросов X (`ELECTRON_Q___ ELECTRON_M___ CTMCRTH_INDANBLCK_FILEY CTMCRTH_INDANBLCK_RECTSTATY CTMCRTH_INDANBLCK_CIRCSTATY CTMCRTH_INDANBLCK_POLYGSTTY`) | только цель `tmc_rtx` | переключатели X-схемы; ровно как в `PlanarRT_X\PlanRT_H.vcxproj`. |

---

## 6. Окружение, в котором собирали (факт)

Сборка велась **в WSL2 (Ubuntu-24.04)** на хосте Windows 11 Pro 22631 — НЕ в Visual
Studio (VS — только для Windows-проекта) и НЕ в отдельной VM/Docker. Тулчейн внутри
WSL: g++ 13.3.0, cmake 3.28.3, ninja 1.11.1, glibc 2.39 — совпадает с целевым
сервером заказчика (Ubuntu 24.04.4). Подробности — `docs\target-environment.md`,
память проекта `wsl2-setup-state`. Пошаговая инструкция — в
`Инструкция_сборки_Linux_WSL2.docx`.
