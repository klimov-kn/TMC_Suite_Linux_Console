/// @file typedef.h
/// @brief Базовые типы и константы пакета: `_real`, `_complex`, размеры имён, физические константы.
///
/// Подключается почти всеми файлами MFC-версии. Задаёт `_HIGH_ACCURACY`, поэтому
/// `_real` — `double`. Пустые макросы `_far`, `_near`, `_huge`, `_fortran`, `_pascal`
/// остались от 16-битных компиляторов и ничего не меняют. Тип `_complex` в C++ берётся
/// из `<math.h>` MinGW или из преамбулы `compat/include/tmc_win_prelude.h`.
/// @see https://www.tamic.ru/docs/code/shared-headers.html#typedef
#ifndef _TYPDEF_H
  #define _TYPDEF_H




/// Двойная точность: `_real` = `double`.
#ifndef _HIGH_ACCURACY
  #define _HIGH_ACCURACY
#endif

/// Тип элемента матрицы Y в коде TAMIC (`_real`); `_ELEM_COMPLEX` — признак
/// комплексных элементов (используется в run.h).
#ifndef _ELEM_VAL
 #define _ELEM_VAL _real
 #ifndef _ELEM_COMPLEX
  /// Признак комплексных элементов матрицы Y (проверяется в run.h и заголовках модели M).
  #define _ELEM_COMPLEX
 #endif
#endif

#include <float.h>

/// Беззнаковый байт (как в Windows).
typedef unsigned char BYTE;

/// Беззнаковое целое.
typedef unsigned int   Uint;
/// Беззнаковое 16-битное целое.
typedef unsigned short WORD;
/// Беззнаковое 32-битное целое (как в Windows; в 64-битном Linux — 64 бита).
typedef unsigned long DWORD;

/// Длина имени файла DOS 8.3 с завершающим нулём, символов.
#ifndef FILENAME
 #define FILENAME           13
#endif

/// @brief Имя файла, разбитое на части: имя, путь, диск (DOS-версия).
typedef struct { char name[FILENAME];
                 char path[FILENAME_MAX];
                 char drive;
                 } FILE_LIST;
/// @var FILE_LIST::name
/// Имя файла с расширением (DOS 8.3), строка с нулём в конце.
/// @var FILE_LIST::path
/// Путь к каталогу файла, строка с нулём в конце.
/// @var FILE_LIST::drive
/// Буква диска.

  #define REAL_MAX  DBL_MAX    /* max value */
  #ifndef _real
    /// Вещественный тип пакета: `double` (без `_HIGH_ACCURACY` — `float`).
    typedef double _real;
  #endif
///@}

// typedef _real _far * r_array;

/// Для C-файлов здесь определяется `_complex` (`x` — действительная часть,
/// `y` — мнимая). Для C++ тип приходит извне (см. описание файла).
 #ifndef __COMPLEX_DEFINED_TMC
   #define __COMPLEX_DEFINED_TMC

	#include <math.h>

#endif



/// Число pi (если не задано ранее).
#ifndef PI
 #define PI                   (_real)3.141592653589793
#endif

/// @brief Проверяет, может ли символ входить в запись числа (цифра, `+`, `-`, `.`).
/// Параметр (без имени в объявлении) — проверяемый символ.
/// @return Ненулевое — может. Реализовано макросом ниже.
int isnumber( char );
/// @brief Макрос-реализация `isnumber`: ненулевое, если `_c` — цифра, `+`, `-` или `.`.
/// @param _c  проверяемый символ
#define isnumber(_c) (isdigit( _c )||( _c == '+' )||( _c == '-' )||( _c == '.' ))

/// @brief Возвращает знак числа: 1 для `rn >= 0`, иначе -1. Реализовано макросом ниже.
/// Параметр (без имени в объявлении) — число типа `_real`.
/// @return 1 или -1.
int rsign( _real );
/// @brief Макрос-реализация `rsign`: 1 для `rn >= 0`, иначе -1.
/// @param rn  число
#define rsign( rn )  ( (rn >= 0)?1:-1 )

/// Короткое целое для таблиц входов (`INPUT_DATA`).
#define INT_1 short

#endif
