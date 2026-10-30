/*
**  TAMIC_soft(R) 1990 v1.00
**
** Declarations of labels for E-programms
**
** FILE: a\include\label_e.h
**
*/

/// @file rth.h
/// @brief Строки-метки программы Rt-H для заголовка файла S-матрицы и окон TAMIC.
///
/// Общий заголовок наследия TAMIC (каталог `include/label`). В этой версии
/// подключается библиотекой SFILE95 (`write_s.cpp`): `MAIN_LABEL` и `TWP_LABEL`
/// записываются в заголовок файла `.s`. Строки — данные формата, их текст
/// не переводится и не меняется.
/// @see https://www.tamic.ru/docs/code/lib-tmcindan.html#limits

#ifndef _LABEL_E_H
  #define _LABEL_E_H

/// Строка авторских прав TAMIC; используется библиотекой SFILE95.
#define COPYRIGHT_TITLE  "Copyright  (C) TAMIC-soft.   All rights reserved."

/// Метка программы Rt-H, записываемая в заголовок файла `.s`.
#define TWP_LABEL        "TAMIC_soft group. TAMIC Rt-H Planar Analyzer. 2000"

/// Ветка `_WINDOWS` — метки старой Windows-программы E-plane; иначе — метки Rt-H.
/// `MAIN_LABEL` записывается в заголовок файла `.s`, `TITLE_STR` используется SFILE95,
/// остальные строки в этой версии не используются.
   #define TITLE_STR "TAMIC-soft (R) Rt-H Planar Analyzer.             Version 2.00."
   /// Метка программы Rt-H, записываемая в заголовок файла `.s` (`write_s.cpp`).
   #define MAIN_LABEL "TAMIC-RTH V 2.00"


#endif
