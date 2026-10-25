# Корневой CMake-файл порта счётных ядер TMC Suite на Linux.
# Консольные tmc_rth / tmc_rtx редакции 5.0 (файл -> расчёт -> файл).
# Подробности: docs/build-linux.md, docs/architecture.md, docs/target-environment.md.

cmake_minimum_required(VERSION 3.22)   # Ubuntu 24.04 поставляет cmake 3.28

project(TMC_Suite_Linux
        VERSION 5.0.0
        DESCRIPTION "TMC Suite 5.0: счётные ядра tmc_rth/tmc_rtx для Linux (консольная версия)"
        LANGUAGES CXX)

# Общие настройки (стандарт, _PREC_DOUBLE, каталоги результатов) — вынесены.
include(${CMAKE_SOURCE_DIR}/cmake/TmcSettings.cmake)

# Библиотеки-математика и каркас целей ядер.
add_subdirectory(src)
