/// @file pl_iofor.h
/// @brief Форматы чисел в выходных файлах ядра, код завершения процесса и вывод в консоль.
///
/// Часть модуля «Счётное ядро X». Функции реализованы в `src/cli/tmc_kernel_console.cpp`:
/// консольная версия работает как пакетный запуск Windows-версии.
/// @see https://www.tamic.ru/docs/code/kernel-x.html#flags


#ifndef _TMCRTH_IOFORMAT__H
#define _TMCRTH_IOFORMAT__H

/// Длина буфера строки формата вывода (символов, с завершающим нулём).
/// @name Форматы вывода по умолчанию (строки printf)
/// Номер такта, время (с), номер блока, падающая и отражённая волны в `.t`;
/// пустой формат поля — значения пишутся в двоичном виде.
///@{
/// Формат номера такта в `.t` по умолчанию.
#define FORMAT_OUT_FILE_NTFORMAT		"%9d"
/// Формат времени подтакта (с) в `.t` по умолчанию.
#define FORMAT_OUT_FILE_DTCurrentFORMAT "%15.7g"
/// Формат номера блока-порта в `.t` по умолчанию.
#define FORMAT_OUT_FILE_NBFORMAT		"%4d"
/// Формат падающей волны Up в `.t` по умолчанию.
#define FORMAT_OUT_FILE_DINPFORMAT		"%15.7g"
/// Формат отражённой волны Uo в `.t` по умолчанию.
#define FORMAT_OUT_FILE_DOUTFORMAT		"%15.7g"
/// Формат значений поля и топологии по умолчанию: пусто — двоичная запись.
#define FORMAT_OUT_FILE_DFLDOUTFORMAT	""

///@}

/// @brief Возвращает формат номера такта.
/// @return Строка формата (статический буфер).
char *GetnTFormat( void );
/// @brief Возвращает формат времени подтакта.
/// @return Строка формата (статический буфер).
char *GetdTcurrentFormat( void );
/// @brief Возвращает формат номера блока.
/// @return Строка формата (статический буфер).
char *GetnBFormat( void );
/// @brief Возвращает формат падающей волны.
/// @return Строка формата (статический буфер).
char *GetdInpFormat( void );
/// @brief Возвращает формат отражённой волны.
/// @return Строка формата (статический буфер).
char *GetdOutFormat( void );
/// @brief Возвращает формат значений поля и топологии.
/// @return Строка формата; пустая — двоичный вывод.
char *GetdFieldOutFormat( void );

// RUN STATUS: process exit code and console messages for the command line run
// (keys /Ar, /Arb). The exit code is returned by CPlanRT_HApp::ExitInstance.
/// @name Коды завершения процесса ядра (ERRORLEVEL)
///@{
/// Код 0: все шаги задания посчитаны.
#define TMC_EXIT_OK				0	// all steps of the .tpl are calculated
/// Код 1: задание не найдено или ошибка в задании.
#define TMC_EXIT_TPL_ERROR		1	// .tpl not found or error in .tpl (preprocessor, data)
/// Код 2: ошибка при расчёте шага.
#define TMC_EXIT_CALC_ERROR		2	// error during the calculation of a step
/// Код 3: расчёт остановлен пользователем или окно закрыто до конца.
#define TMC_EXIT_ABORTED		3	// stopped by the user / window closed before the end
///@}

/// @brief Задаёт код завершения процесса.
/// @param[in] nCode один из `TMC_EXIT_*`
void SetRunExitCode( int nCode );
/// @brief Возвращает код завершения процесса.
/// @return Один из `TMC_EXIT_*`.
int  GetRunExitCode( void );
/// @brief Запоминает последнее предупреждение расчёта (показывается снова в конце).
/// @param[in] pszText текст; NULL или "" — очистить
void SetRunWarning( const char *pszText );	// the last warning of the run ("" - clear)
/// @brief Возвращает последнее предупреждение расчёта.
/// @return Текст; "" — предупреждений не было.
const char *GetRunWarning( void );			// "" - the run had no warnings
/// @brief Пишет строку «[имя программы] текст» в консоль или перенаправленный вывод (только при автозапуске).
/// @param[in] pszFormat формат printf
/// @param[in] ... аргументы формата
void ConsolePrint( const char *pszFormat, ... );
/// @brief Пишет в консоль итог «RESULT n: ...» по коду завершения.
void ConsoleReportResult( void );

#endif