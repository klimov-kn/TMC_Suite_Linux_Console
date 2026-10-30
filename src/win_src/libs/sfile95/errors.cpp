/*
 *  TAMIC_soft(R) 1992 vR1.02
 *
 *  put_error_messege - dummy error processing routine.
 *
 *  FILE: ..\errors.c
 *
 *  OVERLAY: NO
*/

/// @file errors.cpp
/// @brief Заглушка `put_error_messege(int, ...)` и тексты сообщений об ошибках S-файлов.
///
/// Часть модуля «Библиотека sfile95». Тело функции закомментировано: в сборке
/// этой версии сообщения не выводятся, функция всегда возвращает 0. Об ошибках
/// вызывающий узнаёт только по кодам возврата.
/// @see https://www.tamic.ru/docs/code/lib-sfile95.html#pitfalls


#include <stdio.h>
#include <stdarg.h>

#include <typedef.h>
#include <error1.h>



int  put_error_messege( int err_num, ... )
  {
   return 0;
  }
