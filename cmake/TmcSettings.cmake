# Общие настройки сборки TMC Suite под Linux.

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
# Как в Windows-проекте: готовые файлы — в dist/<платформа>/{lib,bin}, отдельно от
# исходников (src/) и промежуточных файлов сборки (build/).
set(TMC_PLATFORM "linux-x86-64" CACHE STRING "Папка платформы внутри dist/")
set(TMC_DIST_DIR ${CMAKE_SOURCE_DIR}/dist/${TMC_PLATFORM})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TMC_DIST_DIR}/lib)   # статические .a
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TMC_DIST_DIR}/bin)   # tmc_rth / tmc_rtx
message(STATUS "Артефакты: ${TMC_DIST_DIR}/{lib,bin}")

# --- Глобальные определения ---------------------------------------------------
# Точность расчёта задаёт typerth.h (_PREC_DOUBLE): X-схема в float нестабильна.

# Версия пакета для «О программе».
add_compile_definitions(TMC_VERSION_SHORT="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

# --- Арифметика с плавающей точкой ---------------------------------------------
# g++ по умолчанию имеет право объединять умножение со сложением в одну операцию
# (FMA). Результат при этом чуть точнее, но ОТЛИЧАЕТСЯ в последних битах от того,
# что считает Windows-версия: MSVC такого объединения не делает. Нужны
# совпадающие числа, а не более точные, поэтому объединение запрещаем — так же,
# как в других сборках для Linux. Формулы и порядок операций не меняются.
add_compile_options(-ffp-contract=off)
