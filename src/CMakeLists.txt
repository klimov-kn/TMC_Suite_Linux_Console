# Цели сборки: 6 библиотек-математика (.a) и счётные ядра tmc_rth / tmc_rtx.
#
# Математика — в win_src/ (код версии для Windows без оболочки). CMake компилирует
# те же файлы под g++/Linux. Ключей -fpermissive и -w нет: сборка идёт без ошибок
# и предупреждений компилятора.

set(WIN_SRC     ${CMAKE_CURRENT_SOURCE_DIR}/win_src)
set(TMC_INCLUDE ${WIN_SRC}/include)
set(TMC_COMPAT  ${CMAKE_CURRENT_SOURCE_DIR}/compat)

# --- Слой совместимости -------------------------------------------------------
# Типы Windows, CString и Win32 API на POSIX (compat/include).
add_library(tmc_compat INTERFACE)
target_include_directories(tmc_compat INTERFACE ${TMC_COMPAT}/include)
# Потоки EPSFAST2 (std::thread) и загрузка OpenCL во время работы (dlopen).
find_package(Threads REQUIRED)
target_link_libraries(tmc_compat INTERFACE Threads::Threads ${CMAKE_DL_LIBS})

# Преамбула для файлов win_src: то, что MSVC давал неявно (тип _complex, макросы
# __max/__min, разделитель каталогов, функции MSVC CRT). Подставляется компилятором
# в начало каждого такого файла.
set(TMC_WIN_SRC_FLAGS -include ${TMC_COMPAT}/include/tmc_win_prelude.h)

# tmc_add_library(<имя> <каталог> [<каталог>...] FILES <файл>...)
function(tmc_add_library name)
  cmake_parse_arguments(A "" "" "FILES" ${ARGN})
  set(dirs ${A_UNPARSED_ARGUMENTS})
  list(GET dirs 0 first)
  set(srcs "")
  foreach(f ${A_FILES})
    # Файл лежит в первом каталоге либо в указанном явно: inter/inte_in1.cpp.
    if(f MATCHES "/")
      list(APPEND srcs ${WIN_SRC}/libs/${f})
    else()
      list(APPEND srcs ${WIN_SRC}/libs/${first}/${f})
    endif()
  endforeach()
  add_library(${name} STATIC ${srcs})
  target_include_directories(${name} PUBLIC ${TMC_INCLUDE})
  foreach(d ${dirs})
    target_include_directories(${name} PUBLIC ${WIN_SRC}/libs/${d})
  endforeach()
  target_link_libraries(${name} PUBLIC tmc_compat)
  target_compile_options(${name} PRIVATE ${TMC_WIN_SRC_FLAGS})
endfunction()

# --- Шесть библиотек-математика -----------------------------------------------
tmc_add_library(complex complex FILES cmplxall.cpp)

tmc_add_library(sfile95 sfile95 FILES
    errors.cpp frqdata.cpp read_s.cpp s_file.cpp s_memf.cpp s_remt.cpp
    s_table.cpp s_wemt.cpp sadd.cpp sofftbl.cpp unite_s.cpp write_s.cpp wrrd_s.cpp)

tmc_add_library(exprint expr inter FILES
    expr_com.cpp expr_err.cpp expr_in1.cpp expr_per.cpp expremsg.cpp get_err.cpp
    inter/inte_in1.cpp inter/inte_ope.cpp inter/inte_val.cpp)

tmc_add_library(TMCLibError tmcliberror FILES tmcliberror.cpp)

tmc_add_library(prepr prepr FILES
    def_dest.cpp def_er.cpp def_erd.cpp def_macr.cpp def_str.cpp defmis.cpp
    prep_ld.cpp prep_mn.cpp read_buf.cpp)

tmc_add_library(TMCIndan tmcindan FILES
    fieldintegrated.cpp tmcrth_bolcklist.cpp tmcrth_indan.cpp tmcrth_indanoutput.cpp
    tmcrth_indanparam.cpp tmcrth_indantopology.cpp)

target_link_libraries(prepr    PUBLIC TMCLibError)
target_link_libraries(TMCIndan PUBLIC prepr exprint TMCLibError)

# Между prepr и sfile95 есть взаимная зависимость; многочастотный расчёт и TMCIndan
# тоже ссылаются друг на друга. Однопроходный GNU ld цикл не разрешает, поэтому
# набор оборачивается в --start-group/--end-group.
set(KERNEL_LIBS sfile95 prepr exprint TMCIndan TMCLibError complex)

# --- Счётные ядра -------------------------------------------------------------
# --wrap=fopen: все вызовы fopen, в том числе внутри .a, идут в __wrap_fopen
# (compat/tmc_textmode.cpp) — текстовый режим Windows. Двоичные режимы проходят
# в настоящий fopen без изменений.
set(TMC_LINK -Wl,--wrap=fopen -Wl,--start-group ${KERNEL_LIBS} -Wl,--end-group tmc_compat)

# tmc_add_kernel(<цель> <каталог ядра в win_src> <H|X>)
function(tmc_add_kernel name kernel_dir mode)
  set(KDIR ${WIN_SRC}/${kernel_dir})
  add_executable(${name}
      ${KDIR}/tmcrthnodediel.cpp
      ${KDIR}/tmcrthrectnode.cpp
      ${WIN_SRC}/viewers/tmcrtout/tmcttos.cpp       # время -> S-параметры
      ${WIN_SRC}/viewers/tmcrtout/tmcsmatrix.cpp    # S-матрица -> файл .s
      ${WIN_SRC}/viewers/tmcrtout/tmcwideband.cpp   # многочастотный расчёт
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_kernel_main.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_kernel_console.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/cli/tmc_about.cpp
      ${TMC_COMPAT}/tmc_textmode.cpp)
  # Каталог ядра идёт перед общим: у H и X свои pl_iofor.h и tmc*.h.
  target_include_directories(${name} PRIVATE
      ${KDIR} ${TMC_INCLUDE} ${WIN_SRC}/viewers/tmcrtout ${CMAKE_CURRENT_SOURCE_DIR}/cli)
  target_compile_options(${name} PRIVATE ${TMC_WIN_SRC_FLAGS})
  target_compile_definitions(${name} PRIVATE TMC_KERNEL_MODE="${mode}" TMC_KERNEL_EXE="${name}")
  target_link_libraries(${name} PRIVATE ${TMC_LINK})
endfunction()

tmc_add_kernel(tmc_rth kernels/planarrt_h H)
tmc_add_kernel(tmc_rtx kernels/planarrt_x X)

# Средство сверки результатов; в поставку не входит, в репозитории его нет.
set(TMC_SDUMP ${CMAKE_SOURCE_DIR}/tools/dev/src/sdump.cpp)
if(EXISTS ${TMC_SDUMP})
  add_executable(sdump ${TMC_SDUMP} ${TMC_COMPAT}/tmc_textmode.cpp)
  target_include_directories(sdump PRIVATE ${TMC_INCLUDE})
  target_compile_options(sdump PRIVATE ${TMC_WIN_SRC_FLAGS})
  target_link_libraries(sdump PRIVATE ${TMC_LINK})
endif()
