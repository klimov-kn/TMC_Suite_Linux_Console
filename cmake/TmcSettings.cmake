# Общие настройки сборки TMC Suite под Linux.
# Источник требований: docs/build-linux.md, docs/target-environment.md, правила проекта.

# --- Стандарт языка -----------------------------------------------------------
# Целевой компилятор — g++ 13 (Ubuntu 24.04). C++17 — безопасный выбор:
# C++20 доступен, но без необходимости не завязываемся (target-environment.md).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Тип сборки по умолчанию --------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Тип сборки" FORCE)
  message(STATUS "CMAKE_BUILD_TYPE не задан — по умолчанию Release")
endif()

# --- Каталоги результатов (dist) ----------------------------------------------
# Как в Windows-проекте (build/LibOutput.props, ExeOutput.props): готовые
# артефакты раскладываются в dist/<платформа>/{lib,bin} — отдельно от исходников
# (src/) и от промежуточных объектников сборки (build/).
# Платформы Windows-проекта: win32 / win64. На Linux в скоупе одна — x86-64;
# уровень <платформа> сохранён ради той же структуры и задела на будущее.
set(TMC_PLATFORM "linux-x86-64" CACHE STRING "Папка платформы внутри dist/")
set(TMC_DIST_DIR ${CMAKE_SOURCE_DIR}/dist/${TMC_PLATFORM})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TMC_DIST_DIR}/lib)   # статические .a
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TMC_DIST_DIR}/bin)   # tmc_rth / tmc_rtx
message(STATUS "Артефакты: ${TMC_DIST_DIR}/{lib,bin}")

# --- Глобальные определения ---------------------------------------------------
# Баг #11 (общий с Windows-проектом): X-схема в float нестабильна — только double.
# Запрет на _PREC_FLOAT — см. правила проекта.
add_compile_definitions(_PREC_DOUBLE)

# ВНИМАНИЕ: 6 X-макросов (ELECTRON_Q___ и т.д.) задаются ТОЛЬКО на цели tmc_rtx
# через target_compile_definitions (см. src/CMakeLists.txt), НЕ глобально и НЕ в
# заголовках (правила проекта, architecture.md).

# --- Параллелизм (опционально) ------------------------------------------------
# OpenMP идёт с g++ (libgomp). MPI НЕ используется (docs/parallelism.md).
# Подключение к целям ядер выполняется при портировании, здесь только детект.
find_package(OpenMP QUIET COMPONENTS CXX)
if(OpenMP_CXX_FOUND)
  message(STATUS "OpenMP найден (${OpenMP_CXX_VERSION}) — будет подключён к ядрам при портировании")
else()
  message(STATUS "OpenMP не найден — продолжаем без него")
endif()
