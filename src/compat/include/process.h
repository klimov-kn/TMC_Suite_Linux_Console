// Заглушка MSVC <process.h> для сборки под Linux.
//
// С редакции 5.0 (исправление С-16 Windows-версии, 25.10.2026) препроцессор
// добавляет номер процесса в имя временного файла макросов ($$vr$$s<pid>.prc),
// чтобы одновременные запуски одного задания не мешали друг другу. Номер
// процесса MSVC даёт функцией _getpid из <process.h>; в POSIX это getpid().
#pragma once
#include <unistd.h>

#ifndef _getpid
#define _getpid getpid
#endif
