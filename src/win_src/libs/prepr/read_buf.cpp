/*
**
**  def_str.      : 12 function for includer/definer
**
**  function      :
**                  read_buf - gets string from buffer
**
**  HEDER FILE'S  :
**                  <stdio.h>
**                  <bufm.h>
**
**  LIB           : oindtX.lib
**
**  OVERLAY       : YES
**
*/

/// @file read_buf.cpp
/// @brief Чтение одной строки из связного буфера препроцессора (`read_buf`).
///
/// Часть модуля «Библиотека prepr». Функция входит в сборку, но в коде пакета не вызывается.
/// @see https://www.tamic.ru/docs/code/lib-prepr.html#buffer

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <tmclimit.h>
#include <typedef.h>
#include <mainwndw.h>
#include <error1.h>

#include "prepr1.h"







