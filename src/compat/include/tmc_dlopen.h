// Загрузка библиотеки во время работы (LoadLibraryA/GetProcAddress/FreeLibrary)
// на Linux и macOS — через dlopen.
//
// Расчёт на видеокарте подгружает OpenCL сам (include/tmcocl.h), чтобы программа
// запускалась и там, где драйвера нет. Windows-имя «OpenCL.dll» переводится в
// имя загрузчика OpenCL системы: на Linux — libOpenCL.so.1 (пакет
// ocl-icd-libopencl1), на macOS — системная рамка OpenCL.
//
// С редакции 5.0 (исправление Т-33 Windows-версии) ядро берёт OpenCL.dll только
// из системного каталога: GetSystemDirectoryA() + "\OpenCL.dll", чтобы не
// подхватить одноимённый файл из папки задания. Поэтому имя сравнивается без
// каталога, а системный каталог — тот, где загрузчик ищет библиотеки системы
// (сам dlopen по короткому имени в текущую папку не смотрит).
//
// Самостоятельный заголовок без слоя MFC (тот же, что в Linux-версии с
// интерфейсом, плюс поддержка полного пути Т-33).
#pragma once
#ifndef _WIN32

#include <dlfcn.h>
#include <string.h>
#include <strings.h>

typedef void* HMODULE;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// Имя файла без каталога (разделитель любой — / или \).
inline const char* tmc_dl_basename(const char* name)
{
    const char* b = name;
    for (const char* p = name; *p; ++p)
        if (*p == '/' || *p == '\\')
            b = p + 1;
    return b;
}

// Системный каталог библиотек. Ядро лишь склеивает его с именем OpenCL.dll, а
// LoadLibraryA ниже узнаёт OpenCL по имени файла и грузит системный загрузчик.
inline unsigned GetSystemDirectoryA(char* buf, unsigned size)
{
    static const char kDir[] = "/usr/lib";
    const unsigned n = (unsigned)(sizeof(kDir) - 1);
    if (!buf || size <= n)
        return n + 1;                 // как в Windows: нужный размер вместе с нулём
    memcpy(buf, kDir, n + 1);
    return n;
}

inline HMODULE LoadLibraryA(const char* name)
{
    if (!name)
        return nullptr;
    if (strcasecmp(tmc_dl_basename(name), "OpenCL.dll") == 0) {
#if defined(__APPLE__)
        return dlopen("/System/Library/Frameworks/OpenCL.framework/OpenCL", RTLD_NOW | RTLD_LOCAL);
#else
        HMODULE h = dlopen("libOpenCL.so.1", RTLD_NOW | RTLD_LOCAL);
        return h ? h : dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL);
#endif
    }
    return dlopen(name, RTLD_NOW | RTLD_LOCAL);
}

inline void* GetProcAddress(HMODULE h, const char* name) { return h ? dlsym(h, name) : nullptr; }

// BOOL Windows — int.
inline int FreeLibrary(HMODULE h) { return (h && dlclose(h) == 0) ? 1 : 0; }

#endif // !_WIN32
