/*
**  TAMIC_soft(R) 1990,1991 v1.00
**
**  tmclimit.h
**
**  FILE : a:\include\tmclimit.h
**
**  LIB  :
**
**  OVERLAY:
**
*/

/// @file tmclimit.h
/// @brief Предельные размеры таблиц и буферов пакета TAMIC (число частот, входов, окон).
///
/// Общий заголовок наследия TAMIC. Из него в этой версии реально используются
/// `FREQ_MAX_NUM` (библиотека SFILE95: предел числа частот в файле `.s`),
/// `MAX_INPUT_NUM` (SFILE95 и общие заголовки: предел числа входов) и
/// `MAX_WINDOW_NUM` (заголовки описания топологии). Остальные макросы
/// сохранены для совместимости и в сборке этой версии не используются.
/// @see https://www.tamic.ru/docs/code/lib-tmcindan.html#limits

#ifndef _TMCLIMIT_H
 #define _TMCLIMIT_H




/// Ёмкость таблицы частот файла `.s`: в одном файле S-матрицы помещается
/// не более `FREQ_MAX_NUM - 1` = 99 частот (проверки — в SFILE95 и `CTmcRTH_Indan::CalcNStepMax`).
#define FREQ_MAX_NUM      100                    /* maximal frequency number*/


  /// Наибольшее число входов (портов) S-матрицы, 20. Проверяется при чтении файла `.s`.
  #define MAX_INPUT_NUM     20


#ifndef MAX_WINDOW_NUM
 /// Наибольшее число окон (блоков описания топологии) в старых заголовках TAMIC.
 #define MAX_WINDOW_NUM    20
#endif


/// Размер массива конечных элементов (наследие; в этой версии не используется).
#define FE_ARRAY_DIM      10000
/*#define FE_ARRAY_DIM	    300 */


#endif
