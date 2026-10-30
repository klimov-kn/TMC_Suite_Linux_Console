/// @file tmc_win_prelude.h
/// @brief Преамбула файлов снимка: подставляется компилятором (`-include`) в начало каждой единицы трансляции.
///
/// Исходники писались под MSVC, и часть вещей компилятор и CRT давали им сами. На g++
/// их нет, поэтому недостающее объявляется снаружи, а не в самих исходниках: слой
/// CString и типов Windows (`tmc_mfc.h`), тип `_complex` с той же раскладкой, что в
/// C-ветке `typedef.h`, макросы и функции MSVC CRT, разделитель каталогов.
/// Формулы и константы здесь не затрагиваются.
/// @see https://www.tamic.ru/docs/code/compat.html

#pragma once

#include "tmc_mfc.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

// typedef.h защищён #ifndef, поэтому повторное включение из исходников ничего не
// меняет. Здесь он нужен ради типа _real: без него нечем объявить _complex.
#include <typedef.h>

#ifndef __COMPLEX_DEFINED
  /// Признак того, что `_complex` уже объявлен (так же в `typedef.h`).
  #define __COMPLEX_DEFINED
  /// Комплексное число: под MSVC приходило из заголовка CRT, раскладка та же.
  typedef struct tag_complex
  {
      _real x;   ///< Действительная часть.
      _real y;   ///< Мнимая часть.
  } _complex;
#endif

/// Разделитель каталогов для новых путей: на Linux — прямой слэш.
#define TMC_PATH_SEP  '/'
/// Разделитель в разбираемых путях: задание с Windows хранит пути с обратным слэшем, принимаются оба.
#define TMC_IS_SEP(c) ( ((c) == '/') || ((c) == '\\') )

#ifndef __max
  /// Макрос старого MSVC CRT: большее из двух.
  #define __max(a,b) ( (a) > (b) ? (a) : (b) )
#endif
#ifndef __min
  /// Макрос старого MSVC CRT: меньшее из двух.
  #define __min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

/// Запись в буфер с ограничением. При усечении `snprintf` тоже ставит завершающий ноль.
#define _snprintf snprintf
/// То же со списком доводов. MSVC при усечении вернёт -1, glibc — нужную длину; код проверяет оба случая.
#define _vsnprintf vsnprintf
/// Строка -> 64-битное целое.
#define _atoi64(s) atoll(s)
/// Сравнение строк без учёта регистра, не длиннее заданного.
#define _strnicmp strncasecmp
/// Сравнение строк без учёта регистра.
#define _stricmp strcasecmp
/// Номер процесса (`<process.h>` MSVC): входит в имя временного файла макросов.
#define _getpid getpid
/// 64-битное целое MSVC.
#define __int64 long long
