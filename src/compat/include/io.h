// compat/include/io.h
// Linux-заглушка для мёртвого `#include <io.h>` (MSVC low-level I/O header)
// в исходниках SFILE95. Проверено: open/close/read/write/lseek встречаются
// в этих файлах только в комментариях, реальных вызовов нет. На Windows
// подключается настоящий io.h (см. src/CMakeLists.txt: tmc_compat
// подключается только при NOT WIN32).
// Если обнаружится реальный вызов — эскалировать, не реализовывать наугад.
#pragma once
