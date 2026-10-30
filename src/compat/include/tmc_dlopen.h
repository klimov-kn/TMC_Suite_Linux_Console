/// @file tmc_dlopen.h
/// @brief `LoadLibraryA`, `GetProcAddress`, `FreeLibrary` через `dlopen`: загрузка OpenCL во время работы.
///
/// Расчёт на видеокарте подгружает OpenCL сам (`tmcocl.h`), чтобы программа запускалась и там,
/// где драйвера нет. Windows-имя `OpenCL.dll` переводится в `libOpenCL.so.1` (пакет
/// ocl-icd-libopencl1). Ядро просит библиотеку только из системного каталога (Т-33), поэтому
/// имя сравнивается без каталога.
/// @see https://www.tamic.ru/docs/code/compat.html

#pragma once
#include <dlfcn.h>
#include <string.h>
#include <strings.h>

/// Описатель загруженной библиотеки.
typedef void* HMODULE;

#ifndef MAX_PATH
/// Наибольшая длина пути Windows.
#define MAX_PATH 260
#endif

/// Имя файла без каталога (разделитель любой — / или \).
inline const char* tmc_dl_basename(const char* name)
{
    const char* b = name;
    for (const char* p = name; *p; ++p)
        if (*p == '/' || *p == '\\')
            b = p + 1;
    return b;
}

/// Системный каталог библиотек. Ядро лишь склеивает его с именем OpenCL.dll, а
/// LoadLibraryA ниже узнаёт OpenCL по имени файла и грузит системный загрузчик.
inline unsigned GetSystemDirectoryA(char* buf, unsigned size)
{
    static const char kDir[] = "/usr/lib";
    const unsigned n = (unsigned)(sizeof(kDir) - 1);
    if (!buf || size <= n)
        return n + 1;                 // как в Windows: нужный размер вместе с нулём
    memcpy(buf, kDir, n + 1);
    return n;
}

/// @brief Загружает библиотеку; `OpenCL.dll` — системный загрузчик OpenCL.
/// @param[in] name имя или путь
/// @return Описатель или `nullptr`.
inline HMODULE LoadLibraryA(const char* name)
{
    if (!name)
        return nullptr;
    if (strcasecmp(tmc_dl_basename(name), "OpenCL.dll") == 0) {
        HMODULE h = dlopen("libOpenCL.so.1", RTLD_NOW | RTLD_LOCAL);
        return h ? h : dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL);
    }
    return dlopen(name, RTLD_NOW | RTLD_LOCAL);
}

/// @brief Находит функцию библиотеки.
/// @param[in] h описатель
/// @param[in] name имя функции
/// @return Адрес или `nullptr`.
inline void* GetProcAddress(HMODULE h, const char* name) { return h ? dlsym(h, name) : nullptr; }

/// @brief Выгружает библиотеку.
/// @param[in] h описатель
/// @return 1 — выгружена.
inline int FreeLibrary(HMODULE h) { return (h && dlclose(h) == 0) ? 1 : 0; }

