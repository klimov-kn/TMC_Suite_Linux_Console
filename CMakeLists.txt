# Корневой CMake-файл порта счётных ядер TMC Suite на Linux.
# Консольные tmc_rth / tmc_rtx (файл -> расчёт -> файл).
# Сборка: https://www.tamic.ru/docs/platforms/linux-cli.html#build

cmake_minimum_required(VERSION 3.22)   # Ubuntu 24.04 поставляет cmake 3.28

project(TMC_Suite_Linux
        VERSION 2.0.0
        DESCRIPTION "TMC Suite 2.0: счётные ядра tmc_rth/tmc_rtx для Linux (консольная версия)"
        LANGUAGES CXX)

# Общие настройки: стандарт, каталоги результатов, арифметика.
include(${CMAKE_SOURCE_DIR}/cmake/TmcSettings.cmake)

# Библиотеки-математика и ядра.
add_subdirectory(src)
