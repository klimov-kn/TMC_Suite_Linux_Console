/*
 *  TAMIC_soft(R) 1990,1991,1992 vR1.02
 *
 *  put_S_table - print out multimode scattering matrix to listing
 *                in the form of table
 *
 *  Usage -  put_S_table( lpxS, int inpnb, int _far *modnb );
 *
 *  Arguments - lpxS - scattering matrix
 *             inpnb - number of inputs of multiport
 *             modnb - array of number of modes at all inputs
 *
 *  Remark - scattering matrix is assumed to be stored
 *           rowwise in general storage mode
 *
 *  Reqd. routines - cabsv, cphase ( in cmplx library )
 *
 *  FILE : \source\tamic\post_pro\s_table.c
 *
 *  LIB  : ooutpXY.lib
 *
 *  OVERLAY: YES
 */

/// @file s_table.cpp
/// @brief Печать многомодовой S-матрицы текущей частоты в листинг таблицей (`put_S_table`).
///
/// Часть модуля «Библиотека sfile95». Наследие TAMIC: в программах этой версии
/// не вызывается, функции листинга в сборке не определены. Функция объявлена
/// и описана в `proc_s.h`.
/// @see https://www.tamic.ru/docs/code/lib-sfile95.html#legacy

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <typedef.h>
#include <frqdata.h>
#include <mainwndw.h>


/// Единица для сравнения модулей.
#define ONE       1.0f
/// Ноль для сравнения модулей.
#define ZERO      0.0f
/// Порог: модуль в [1, GEN_THR) считается равным 1 (ошибка округления).
#define GEN_THR   1.000001f


