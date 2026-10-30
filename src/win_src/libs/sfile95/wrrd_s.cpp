/*
**  TAMIC_soft(R) 1989,1991,1992
**
**  make_S_matrix_name   - make temporary S-file names
**  copy_S_matrix        - copy temporary file to output file if need
**
**  FILE : ..\tamic\post_pro\wrrd_S.c
**
**  LIB  : ooutpXY.lib
**
**  OVERLAY: YES
**
*/

/// @file wrrd_s.cpp
/// @brief Имя S-файла по заданию и шагу (`make_S_matrix_name`), копирование S-файла (`copy_S_matrix`).
///
/// Часть модуля «Библиотека sfile95». Наследие TAMIC: обе функции в программах
/// этой версии не вызываются, а нужные им функции задания (`get_tpl_file_name`,
/// `get_tpl_step`, `change_file_extension`, `copy_file`) в сборке не определены.
/// Объявлены и описаны в `s_file.h`.
/// @see https://www.tamic.ru/docs/code/lib-sfile95.html#legacy

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <typedef.h>
#include <tmclimit.h>
#include <tmcgraph.h>
#include <tmc_lib.h>
#include <frqdata.h>
#include <postpro.h>




/// Имя последнего S-файла, построенного `make_S_matrix_name`.
char S_filename[FILENAME_MAX];






