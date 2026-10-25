# Цели сборки: 6 библиотек-математика (.a) + счётные ядра tmc_rth / tmc_rtx.
#
# Источник математики — снимок Windows-версии 5.0 в win_src/ (импорт:
# tools/import_win_src.py, источник ../TMC_Suite/research/gpu/qt6/src/win_src).
# Математику НЕ трогаем (правила проекта) — CMake лишь компилирует те же файлы под
# g++/Linux. Состав каждой библиотеки — из соответствующего .vcxproj.
#
# Порядок и зависимости библиотек — docs/build-linux.md и ISSUES.md (R7..R10).

set(WIN_SRC     ${CMAKE_CURRENT_SOURCE_DIR}/win_src)
set(TMC_INCLUDE ${WIN_SRC}/include)
set(TMC_COMPAT  ${CMAKE_CURRENT_SOURCE_DIR}/compat)

# --- Слой совместимости MFC/win (src/compat) ---------------------------------
# MFC на Linux нет (ISSUES.md R3). compat/include содержит заглушки заголовков
# (afx.h, afxwin.h, windows.h, cstringt.h -> tmc_mfc.h) с минимальным CString,
# win-типами и Win32 API редакции 5.0 на POSIX (tmc_win32_posix.h). Каталог
# подключается ТОЛЬКО вне Windows, чтобы `#include <afx.h>` в исходниках ядра
# находил наш шим без правки самих исходников.
add_library(tmc_compat INTERFACE)
if(NOT WIN32)
  target_include_directories(tmc_compat INTERFACE ${TMC_COMPAT}/include)
  # Потоки EPSFAST2 (std::thread) и загрузка OpenCL во время работы (dlopen).
  find_package(Threads REQUIRED)
  target_link_libraries(tmc_compat INTERFACE Threads::Threads ${CMAKE_DL_LIBS})
endif()

# Преамбула для файлов win_src: то, что MSVC давал неявно (тип _complex,
# макросы __max/__min, разделитель каталогов TMC_IS_SEP/TMC_PATH_SEP, функции
# MSVC CRT кода 5.0). Подставляется компилятором в начало каждого такого файла,
# поэтому сами исходники править не приходится.
set(TMC_PRELUDE_FLAGS -include ${TMC_COMPAT}/include/tmc_win_prelude.h)

# Исходники содержат классические MSVC-«вольности», которые g++ по стандарту
# считает ошибками, но семантика от них не меняется (extra qualification,
# (int)(void*) при выравнивании — используется лишь mod 32, строковый литерал ->
# char*). -fpermissive лишь понижает эти диагностики до предупреждений; -w
# скрывает предупреждения чужого кода (их тысячи, все — наследие MSVC).
set(TMC_WIN_SRC_FLAGS -fpermissive -w ${TMC_PRELUDE_FLAGS})

# -----------------------------------------------------------------------------
# Вспомогательная функция:
#   tmc_add_library(<name> DIRS <каталог>... FILES <имя.cpp>...)
#
# Компилирует в статическую библиотеку ТОЛЬКО файлы, перечисленные в FILES —
# ровно те, что входят в соответствующий Windows-проект (.vcxproj). Сравнение
# имён — БЕЗ учёта регистра: в .vcxproj имена записаны иначе, чем файлы снимка
# (DEF_MACR.CPP против def_macr.cpp).
#
# Почему не «весь каталог» (GLOB): в папках libs/* лежат и вспомогательные/
# драйверные файлы (MAIN_S, DRV_*, альт-реализации, утилиты с dos.h/io.h/conio.h),
# которые НЕ входят в библиотеку и в Windows не компилируются.
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
    file(GLOB part CONFIGURE_DEPENDS "${WIN_SRC}/${dir}/*.cpp")
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
    message(FATAL_ERROR "${name}: найдено ${n_found} из ${n_want} файлов (.vcxproj). "
                        "Проверьте имена в FILES и снимок win_src (tools/import_win_src.py).")
  endif()

  add_library(${name} STATIC ${srcs})
  target_include_directories(${name} PUBLIC ${TMC_INCLUDE})
  foreach(dir ${A_DIRS})
    target_include_directories(${name} PUBLIC ${WIN_SRC}/${dir})
  endforeach()
  target_link_libraries(${name} PUBLIC tmc_compat)
  target_compile_options(${name} PRIVATE ${TMC_WIN_SRC_FLAGS})
endfunction()

# --- Шесть библиотек-математика ----------------------------------------------
# Состав FILES — из .vcxproj соответствующей библиотеки (Windows-проект).
tmc_add_library(complex     DIRS libs/complex      FILES CMPLXALL.CPP)

tmc_add_library(sfile95     DIRS libs/sfile95      FILES
    Errors.cpp Frqdata.cpp Read_s.cpp S_file.cpp S_memf.cpp S_remt.cpp
    S_table.cpp S_wemt.cpp Sadd.cpp Sofftbl.cpp Unite_s.cpp Write_s.cpp Wrrd_s.cpp)

tmc_add_library(exprint     DIRS libs/expr libs/inter FILES   # R10: Expr+Inter
    Expr_com.cpp Expr_err.cpp Expr_in1.cpp Expr_per.cpp Expremsg.cpp Get_err.cpp
    Inte_in1.cpp Inte_ope.cpp Inte_val.cpp)

tmc_add_library(TMCLibError DIRS libs/tmcliberror  FILES TmcLibError.cpp)

tmc_add_library(prepr       DIRS libs/prepr        FILES
    Def_dest.cpp Def_er.cpp Def_erd.cpp Def_macr.cpp Def_str.cpp Defmis.cpp
    Prep_ld.cpp Prep_mn.cpp Read_buf.cpp)

tmc_add_library(TMCIndan    DIRS libs/tmcindan     FILES
    FieldIntegrated.cpp TmcRTH_BolckList.cpp TmcRTH_Indan.cpp TmcRTH_IndanOutput.cpp
    TmcRTH_IndanParam.cpp TmcRTH_IndanTopology.cpp TmcRTH_Input.cpp TmcRTH_InputNode.cpp)

# --- Зависимости между библиотеками (ISSUES.md R7) ---------------------------
target_link_libraries(prepr    PUBLIC TMCLibError)
target_link_libraries(TMCIndan PUBLIC prepr exprint TMCLibError)

# Между prepr и sfile95 есть ВЗАИМНАЯ зависимость (alloc_mem_err/free_mem в
# sfile95 зовут prepr, put_error_messege в prepr зовёт sfile95); многочастотный
# расчёт (tmcwideband.cpp, в ядре) и TMCIndan тоже ссылаются друг на друга.
# Однопроходный GNU ld цикл не разрешает, поэтому набор оборачивается в
# --start-group/--end-group. complex — явно (R9: ядро зовёт его напрямую).
set(KERNEL_LIBS sfile95 prepr exprint TMCIndan TMCLibError complex)

# -----------------------------------------------------------------------------
# Счётные ядра tmc_rth / tmc_rtx.
#
# Состав — ТОЛЬКО вычислительные модули ядра (как в PlanRT_H.vcxproj без
# MFC-обвязки): расчётные узлы, S-матрица, «время -> S», многочастотный расчёт.
# GUI-модули (mainfrm, childfrm, *doc, *view, диалоги, звук) и pl_glfun.cpp /
# pl_iofor.cpp (строка состояния, флаги окна) в сборку не входят: функции,
# нужные счёту, реализованы консольно в cli/tmc_kernel_console.cpp, а цикл
# «все шаги задания» (CPlanRT_HDoc::RunAll) — в cli/tmc_kernel_main.cpp.
#
#   tmc_add_kernel(<цель> <каталог ядра в win_src> <H|X> [макросы...])
# -----------------------------------------------------------------------------
option(TMC_BUILD_KERNELS "Собирать ядра tmc_rth/tmc_rtx" ON)

function(tmc_add_kernel name kernel_dir mode)
  set(KDIR ${WIN_SRC}/${kernel_dir})
  add_executable(${name}
      ${KDIR}/tmcrthnodediel.cpp
      ${KDIR}/tmcrthrectnode.cpp
      ${WIN_SRC}/viewers/tmcrtout/tmcttos.cpp       # время -> S-параметры
      ${WIN_SRC}/viewers/tmcrtout/tmcsmatrix.cpp    # S-матрица -> файл .s
      ${WIN_SRC}/viewers/tmcrtout/tmcwideband.cpp   # многочастотный расчёт (5.0)
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_kernel_main.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_kernel_console.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_about.cpp
      # Текстовый режим CRT Windows (CRLF<->LF на границе ввода-вывода).
      ${TMC_COMPAT}/tmc_textmode.cpp)
  # ВАЖНО: каталог ядра идёт ПЕРЕД общим — чтобы planrt_h.h/pl_iofor.h/
  # resource.h/tmc*.h резолвились в версию именно этого ядра (у H и X они свои).
  target_include_directories(${name} PRIVATE
      ${KDIR} ${TMC_INCLUDE} ${WIN_SRC}/viewers/tmcrtout ${CMAKE_CURRENT_SOURCE_DIR}/cli)
  target_compile_options(${name} PRIVATE ${TMC_WIN_SRC_FLAGS})
  target_compile_definitions(${name} PRIVATE
      TMC_KERNEL_MODE="${mode}" TMC_KERNEL_EXE="${name}"
      TMC_VERSION="${PROJECT_VERSION}" ${ARGN})
  # --wrap=fopen: все вызовы fopen (в т.ч. внутри .a) идут в __wrap_fopen
  # (compat/tmc_textmode.cpp) — текстовый режим Windows. Бинарные 'b'
  # пропускаются в __real_fopen без изменений.
  target_link_libraries(${name} PRIVATE
      -Wl,--wrap=fopen
      -Wl,--start-group ${KERNEL_LIBS} -Wl,--end-group
      tmc_compat)
endfunction()

if(TMC_BUILD_KERNELS)
  # Руководство пользователя — рядом с ядрами: его показывают --about и --manual.
  set(TMC_MANUAL ${CMAKE_SOURCE_DIR}/docs/TMC_Suite_User_Manual.pdf)
  if(EXISTS ${TMC_MANUAL})
    file(COPY ${TMC_MANUAL} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  endif()

  tmc_add_kernel(tmc_rth kernels/planarrt_h H)

  # 6 X-макросов — ТОЛЬКО на цели tmc_rtx (правила проекта; из PlanarRT_X/PlanRT_H.vcxproj,
  # PreprocessorDefinitions). Применяются ко ВСЕМ TU цели (как в .vcxproj). Это
  # переключатели X-схемы в общей математике. _PREC_DOUBLE закреплён глобально
  # (Баг #11 — X в float нестабилен).
  #
  # Библиотеки TMCIndan и др. собираются ОДИН раз, без X-макросов, — так же, как
  # в Windows-проекте (библиотеки общие, макросы стоят только в проекте ядра X).
  tmc_add_kernel(tmc_rtx kernels/planarrt_x X
      ELECTRON_Q___ ELECTRON_M___
      CTMCRTH_INDANBLCK_FILEY     CTMCRTH_INDANBLCK_RECTSTATY
      CTMCRTH_INDANBLCK_CIRCSTATY CTMCRTH_INDANBLCK_POLYGSTTY)
endif()

# --- Средство проверки (в поставку не входит) ---------------------------------
# Печать матрицы рассеяния из файла .s — тем же кодом, каким её читают программы
# пакета. Нужна регрессии (tools/reg/reg_cli.py), чтобы сверять S-матрицы по
# числам. Тот же инструмент, что в Linux-версии с интерфейсом.
add_executable(sdump ${CMAKE_SOURCE_DIR}/tools/dev/src/sdump.cpp)
target_include_directories(sdump PRIVATE ${TMC_INCLUDE})
target_compile_options(sdump PRIVATE ${TMC_WIN_SRC_FLAGS})
target_link_libraries(sdump PRIVATE
    -Wl,--wrap=fopen -Wl,--start-group ${KERNEL_LIBS} -Wl,--end-group tmc_compat)
target_sources(sdump PRIVATE ${TMC_COMPAT}/tmc_textmode.cpp)
