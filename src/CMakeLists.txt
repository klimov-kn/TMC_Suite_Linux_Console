# Цели сборки: 6 библиотек-математика (.a) + каркас счётных ядер.
#
# Источники — импортированное дерево Windows в win_src/ (источник истины:
# ../TMC_Suite/src). Математику НЕ трогаем (правила проекта) — CMake лишь компилирует
# те же файлы под g++/Linux.
#
# Порядок и зависимости библиотек — docs/build-linux.md и ISSUES.md (R7..R10).

set(WIN_SRC     ${CMAKE_CURRENT_SOURCE_DIR}/win_src)
set(TMC_INCLUDE ${WIN_SRC}/Include)

# --- Слой совместимости MFC/win (src/compat) ---------------------------------
# MFC на Linux нет (ISSUES.md R3). compat/include содержит заглушки заголовков
# (afx.h, afxwin.h, cstringt.h -> tmc_mfc.h) с минимальным CString/BOOL. Каталог
# подключается ТОЛЬКО вне Windows, чтобы `#include <afx.h>` в исходниках ядра
# находил наш шим без правки самих исходников. На Windows используется реальный MFC.
add_library(tmc_compat INTERFACE)
if(NOT WIN32)
  target_include_directories(tmc_compat INTERFACE
      ${CMAKE_CURRENT_SOURCE_DIR}/compat/include)
endif()

# -----------------------------------------------------------------------------
# Вспомогательная функция:
#   tmc_add_library(<name> DIRS <каталог>... FILES <имя.cpp>...)
#
# Компилирует в статическую библиотеку ТОЛЬКО файлы, перечисленные в FILES —
# ровно те, что входят в соответствующий Windows-проект (.vcxproj). Сравнение
# имён — БЕЗ учёта регистра: в .vcxproj имена записаны иначе, чем реальные файлы
# на диске (DEF_MACR.CPP vs Def_macr.cpp), а Linux к регистру чувствителен — поэтому
# берём реальные файлы из glob, но оставляем лишь те, чьё имя есть в allowlist.
#
# Почему не «весь каталог» (GLOB): в папках libs/* лежат и вспомогательные/
# драйверные файлы (MAIN_S, DRV_*, альт-реализации, утилиты с dos.h/io.h/conio.h),
# которые НЕ входят в библиотеку и в Windows не компилируются. Их подхват ломал бы
# сборку без всякой пользы. Источник истины по составу — .vcxproj каждой библиотеки.
# -----------------------------------------------------------------------------
function(tmc_add_library name)
  cmake_parse_arguments(A "" "" "DIRS;FILES" ${ARGN})

  set(allow "")
  foreach(f ${A_FILES})
    string(TOLOWER "${f}" lf)
    list(APPEND allow "${lf}")
  endforeach()

  set(srcs "")
  foreach(dir ${A_DIRS})
    file(GLOB part CONFIGURE_DEPENDS
         "${WIN_SRC}/${dir}/*.cpp"
         "${WIN_SRC}/${dir}/*.CPP")
    foreach(p ${part})
      get_filename_component(bn "${p}" NAME)
      string(TOLOWER "${bn}" lbn)
      if(lbn IN_LIST allow)
        list(APPEND srcs "${p}")
      endif()
    endforeach()
  endforeach()

  list(LENGTH srcs n_found)
  list(LENGTH allow n_want)
  if(NOT n_found EQUAL n_want)
    message(WARNING "${name}: найдено ${n_found} из ${n_want} файлов (.vcxproj). "
                    "Проверьте имена в FILES и наличие файлов на диске.")
  endif()

  add_library(${name} STATIC ${srcs})
  target_include_directories(${name} PUBLIC ${TMC_INCLUDE})
  target_link_libraries(${name} PUBLIC tmc_compat)   # шим MFC/win (см. выше)
endfunction()

# --- Ф1: шесть библиотек-математика ------------------------------------------
# Состав FILES — из .vcxproj соответствующей библиотеки (Windows-проект).
tmc_add_library(complex     DIRS libs/complex      FILES CMPLXALL.CPP)

tmc_add_library(sfile95     DIRS libs/SFILE95      FILES
    Errors.cpp Frqdata.cpp Read_s.cpp S_file.cpp S_memf.cpp S_remt.cpp
    S_table.cpp S_wemt.cpp Sadd.cpp Sofftbl.cpp Unite_s.cpp Write_s.cpp Wrrd_s.cpp)

tmc_add_library(exprint     DIRS libs/Expr libs/Inter FILES   # R10: Expr+Inter
    Expr_com.cpp Expr_err.cpp Expr_in1.cpp Expr_per.cpp Expremsg.cpp Get_err.cpp
    Inte_in1.cpp Inte_ope.cpp Inte_val.cpp)

tmc_add_library(TMCLibError DIRS libs/TMCLibError  FILES TmcLibError.cpp)

tmc_add_library(prepr       DIRS libs/PREPR        FILES
    Def_dest.cpp Def_er.cpp Def_erd.cpp Def_macr.cpp Def_str.cpp Defmis.cpp
    Prep_ld.cpp Prep_mn.cpp Read_buf.cpp)

tmc_add_library(TMCIndan    DIRS libs/TMCIndan     FILES
    FieldIntegrated.cpp TmcRTH_BolckList.cpp TmcRTH_Indan.cpp TmcRTH_IndanOutput.cpp
    TmcRTH_IndanParam.cpp TmcRTH_IndanTopology.cpp TmcRTH_Input.cpp TmcRTH_InputNode.cpp)

# --- Зависимости между библиотеками (ISSUES.md R7) ---------------------------
# Направление «зависящая -> зависимость». Циклов нет, поэтому link group не нужен;
# CMake сам выстроит корректный для GNU ld порядок линковки.
target_link_libraries(prepr    PUBLIC TMCLibError)
target_link_libraries(TMCIndan PUBLIC prepr exprint TMCLibError)
# complex и sfile95 — листовые (ни от кого из набора не зависят).

# -----------------------------------------------------------------------------
# Ф3: счётные ядра tmc_rth / tmc_rtx — КАРКАС.
#
# Цели пока НЕ активны: исходники ядер содержат MFC-обвязку (MainFrm, ChildFrm,
# *Doc, *View, Dialog*, Sound*, StdAfx) и не имеют консольного main. Список
# вычислительных модулей и точки входа src/cli/* появятся после:
#   1) mfc-decoupler  — отделить расчёт от MFC,
#   2) linux-porter   — починить компиляцию под g++,
#   3) src/cli/        — консольные точки входа (argv: вход/выход).
#
# Структура линковки и X-макросы зафиксированы заранее (ниже), чтобы при
# активации осталось только подставить список .cpp ядра.
#
# Общий набор библиотек для обоих ядер (ISSUES.md R7; complex — транзитивно,
# при необходимости добавить явно — R9):
set(KERNEL_LIBS sfile95 prepr exprint TMCIndan TMCLibError)

option(TMC_BUILD_KERNELS "Собирать ядра tmc_rth/tmc_rtx (доступно после портирования)" OFF)
if(TMC_BUILD_KERNELS)
  set(KERNEL_H_DIR ${WIN_SRC}/kernels/PlanarRT_H)

  # --- tmc_rth: счётное ядро H (консольный порт) -----------------------------
  # Состав — ТОЛЬКО вычислительные модули ядра. GUI/MFC-обвязка исключена:
  # MainFrm/ChildFrm/*Doc/*View/Dialog*/Sound*/StdAfx/*.rc, а также PL_GLFUN.CPP
  # (статусбар/звук/потоки) и Pl_iofor.cpp (GUI-флаги/хранилище форматов) —
  # их функции, нужные счёту, заменены консольными в src/cli/tmc_kernel_console.cpp
  # (+ перенесённый байт-в-байт IsPointInTr). Точка входа — src/cli/tmc_rth_main.cpp
  # (повторяет RunAll() напрямую через CTmcRTHRectNode, без Doc/View). См. ISSUES.md.
  add_executable(tmc_rth
      ${KERNEL_H_DIR}/TmcRTHNodeDiel.cpp
      ${KERNEL_H_DIR}/TmcRTHRectNode.cpp
      # Вычисление/сериализация S-матрицы (время -> S-параметры -> файл S).
      # Входят в .vcxproj ядра; не GUI (CTmcTtoS/CTmcSMatrix не наследуют MFC).
      ${WIN_SRC}/viewers/Tmcrtout/TmcTtoS.cpp
      ${WIN_SRC}/viewers/Tmcrtout/TmcSMatrix.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_rth_main.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_kernel_console.cpp
      # Текстовый режим CRT Windows (CRLF<->LF на границе ввода-вывода). См. ISSUES.md.
      ${CMAKE_CURRENT_SOURCE_DIR}/compat/tmc_textmode.cpp)
  # Заголовки ядра — в Include/, плюс собственный каталог ядра (resource.h и
  # локальные .h, включаемые через кавычки/угловые скобки).
  target_include_directories(tmc_rth PRIVATE ${TMC_INCLUDE} ${KERNEL_H_DIR})
  # -fpermissive: исходники ядра содержат классические MSVC-«вольности», которые
  # g++ по стандарту считает ошибками, но MSVC принимал, и СЕМАНТИКА от этого не
  # меняется (математику/исходники не правим, ISSUES.md раздел H):
  #   * extra qualification в объявлении внутри класса (TmcRTHNodeDiel.h:61-62);
  #   * (int)(void*) при вычислении выравнивания на 32 байта — используется лишь
  #     mod 32 (низкие биты); на Windows x64 int тоже 32-битный, усечка идентична;
  #   * строковый литерал -> char* в PutErrorMessage(...).
  # Флаг лишь понижает эти конкретные диагностики до предупреждений, не маскируя
  # настоящие ошибки типов.
  target_compile_options(tmc_rth PRIVATE -fpermissive)
  # R7 (уточнён): между prepr и sfile95 ОБНАРУЖЕНА ВЗАИМНАЯ зависимость (цикл):
  #   * sfile95/S_MEMF.CPP::alloc_mem_err/free_mem  <- использует prepr;
  #   * prepr/Prep_mn.cpp::put_error_messege        <- использует sfile95.
  # (определения free_mem/alloc_mem_err в Prep_mn.cpp ЗАКОММЕНТИРОВАНЫ — реальные
  # в S_MEMF.CPP.) Однопроходный порядок GNU ld цикл не разрешает, поэтому
  # оборачиваем набор в --start-group/--end-group (многопроходное разрешение).
  # R9: complex добавлен ЯВНО — ядро использует complex-функции напрямую.
  # --wrap=fopen: все вызовы fopen (в т.ч. внутри .a) идут в __wrap_fopen
  # (src/compat/tmc_textmode.cpp) — текстовый режим Windows. Бинарные 'b' внутри
  # обёртки пропускаются в __real_fopen без изменений.
  target_link_libraries(tmc_rth PRIVATE
      -Wl,--wrap=fopen
      -Wl,--start-group ${KERNEL_LIBS} complex -Wl,--end-group
      tmc_compat)

  # --- tmc_rtx: счётное ядро X (консольный порт) -----------------------------
  # Структура отделения от MFC идентична H (классы/файлы те же — X это копипаст H,
  # R10): те же вычислит. модули + viewers + ОБЩИЙ tmc_kernel_console.cpp (интерфейс
  # колбэков у H и X совпадает; нужные заголовки берутся из KERNEL_X_DIR по include-
  # пути). Точка входа — отдельный tmc_rtx_main.cpp (свой баннер). См. ISSUES.md.
  set(KERNEL_X_DIR ${WIN_SRC}/kernels/PlanarRT_X)
  add_executable(tmc_rtx
      ${KERNEL_X_DIR}/TmcRTHNodeDiel.cpp
      ${KERNEL_X_DIR}/TmcRTHRectNode.cpp
      ${WIN_SRC}/viewers/Tmcrtout/TmcTtoS.cpp
      ${WIN_SRC}/viewers/Tmcrtout/TmcSMatrix.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_rtx_main.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_kernel_console.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/compat/tmc_textmode.cpp)
  # ВАЖНО: include-путь ядра X идёт ПЕРЕД общим — чтобы PlanRT_H.h/Pl_iofor.h/
  # resource.h/Tmc*.h резолвились в версии X, а не H.
  target_include_directories(tmc_rtx PRIVATE ${KERNEL_X_DIR} ${TMC_INCLUDE})
  target_compile_options(tmc_rtx PRIVATE -fpermissive)   # те же MSVC-вольности, что в H
  target_link_libraries(tmc_rtx PRIVATE
      -Wl,--wrap=fopen
      -Wl,--start-group ${KERNEL_LIBS} complex -Wl,--end-group
      tmc_compat)
  # 6 X-макросов — ТОЛЬКО на этой цели (правила проекта; из PlanarRT_X/PlanRT_H.vcxproj,
  # PreprocessorDefinitions). Применяются ко ВСЕМ TU цели (как в .vcxproj). Это
  # переключатели X-схемы в общей математике (TMCIndan/ядро). _PREC_DOUBLE уже
  # закреплён в Typerth.h (Баг #11 — X в float нестабилен).
  target_compile_definitions(tmc_rtx PRIVATE
      ELECTRON_Q___ ELECTRON_M___
      CTMCRTH_INDANBLCK_FILEY     CTMCRTH_INDANBLCK_RECTSTATY
      CTMCRTH_INDANBLCK_CIRCSTATY CTMCRTH_INDANBLCK_POLYGSTTY)
endif()
