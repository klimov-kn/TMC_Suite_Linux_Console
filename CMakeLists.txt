# Корневой CMake-файл порта счётных ядер TMC Suite на Linux.
# Цель этапа 2026: консольные tmc_rth / tmc_rtx (файл -> расчёт -> файл).
# Подробности: docs/build-linux.md, docs/architecture.md, docs/target-environment.md.

cmake_minimum_required(VERSION 3.22)   # Ubuntu 24.04 поставляет cmake 3.28

project(TMC_Suite_Linux
        VERSION 0.1.0
        DESCRIPTION "Консольный порт счётных ядер TMC Suite (tmc_rth/tmc_rtx) на Linux"
        LANGUAGES CXX)

# Общие настройки (стандарт, _PREC_DOUBLE, каталоги результатов) — вынесены.
include(${CMAKE_SOURCE_DIR}/cmake/TmcSettings.cmake)

# Библиотеки-математика и каркас целей ядер.
add_subdirectory(src)
