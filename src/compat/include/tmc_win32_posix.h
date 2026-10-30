/// @file tmc_win32_posix.h
/// @brief Win32 API на POSIX: часы, время потока, память, сведения о машине,
///        командная строка, полный путь, ожидание потоков EPSFAST2.
///
/// Откуда вызовы: замер времени (`tmccalctiming.h`), расчёт на видеокарте (`tmcgpuh.h`,
/// `tmcgpux.h`), поле в полосах вдоль границы (`fieldintegrated.cpp`), полный путь задания
/// (`tmcrth_indan.cpp`). Ни один из этих вызовов не участвует в вычислениях; где точного
/// аналога нет, сказано у функции.
/// @see https://www.tamic.ru/docs/code/compat.html

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <pthread.h>
#include <strings.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>

// --- 64-битные целые из двух половин ----------------------------------------
// DWORD в слое — unsigned long (64 бита на Linux), поэтому половины объявлены
// явно 32-битными: иначе QuadPart не совпал бы с парой LowPart/HighPart.
/// 64-битное целое из двух половин со знаком.
union LARGE_INTEGER
{
    struct { uint32_t LowPart; int32_t HighPart; };   ///< Половины.
    long long QuadPart;                               ///< Всё число.
};
/// 64-битное целое из двух половин без знака.
union ULARGE_INTEGER
{
    struct { uint32_t LowPart; uint32_t HighPart; };  ///< Половины.
    unsigned long long QuadPart;                      ///< Всё число.
};

// --- Часы высокого разрешения ------------------------------------------------
// QueryPerformanceCounter — монотонные часы; частота — 1 ГГц (наносекунды).
/// @brief Частота часов `QueryPerformanceCounter`.
/// @param[out] f частота: 1 ГГц (наносекунды)
/// @return TRUE.
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* f)
{
    f->QuadPart = 1000000000LL;
    return TRUE;
}
/// @brief Значение монотонных часов.
/// @param[out] t наносекунды
/// @return TRUE.
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* t)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t->QuadPart = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    return TRUE;
}

// --- Местное время -----------------------------------------------------------
/// Местное время по частям.
struct SYSTEMTIME
{
    WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;   ///< Части даты и времени.
};
/// @brief Возвращает местное время.
/// @param[out] st время
inline void GetLocalTime(SYSTEMTIME* st)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tmv;
    localtime_r(&ts.tv_sec, &tmv);
    st->wYear         = WORD(tmv.tm_year + 1900);
    st->wMonth        = WORD(tmv.tm_mon + 1);
    st->wDayOfWeek    = WORD(tmv.tm_wday);
    st->wDay          = WORD(tmv.tm_mday);
    st->wHour         = WORD(tmv.tm_hour);
    st->wMinute       = WORD(tmv.tm_min);
    st->wSecond       = WORD(tmv.tm_sec);
    st->wMilliseconds = WORD(ts.tv_nsec / 1000000);
}

// --- Критическая секция ------------------------------------------------------
// В Windows критическая секция повторно входима тем же потоком — мьютекс
// рекурсивный.
/// Критическая секция: повторно входима тем же потоком, поэтому мьютекс рекурсивный.
typedef pthread_mutex_t CRITICAL_SECTION;
/// @brief Создаёт критическую секцию.
/// @param[out] cs секция
inline void InitializeCriticalSection(CRITICAL_SECTION* cs)
{
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(cs, &a);
    pthread_mutexattr_destroy(&a);
}
/// @brief Входит в секцию.
/// @param[in,out] cs секция
inline void EnterCriticalSection(CRITICAL_SECTION* cs)  { pthread_mutex_lock(cs); }
/// @brief Выходит из секции.
/// @param[in,out] cs секция
inline void LeaveCriticalSection(CRITICAL_SECTION* cs)  { pthread_mutex_unlock(cs); }
/// @brief Удаляет секцию.
/// @param[in,out] cs секция
inline void DeleteCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_destroy(cs); }

// --- Текущий поток и процесс, процессорное время -----------------------------
// Псевдоописатели, как в Windows: понимают только функции ниже.
/// @brief Псевдоописатель текущего потока: его понимают только функции ниже.
/// @return Описатель.
inline void* GetCurrentThread()  { return (void*)(intptr_t)-2; }
/// @brief Псевдоописатель текущего процесса.
/// @return Описатель.
inline void* GetCurrentProcess() { return (void*)(intptr_t)-1; }

/// @brief Переводит время в сотни наносекунд.
/// @param[in] tv время
/// @param[out] ft результат
inline void tmc_timeval_to_filetime(const struct timeval& tv, FILETIME* ft)
{
    /// FILETIME — сотни наносекунд.
    const unsigned long long v = (unsigned long long)tv.tv_sec * 10000000ULL
                               + (unsigned long long)tv.tv_usec * 10ULL;
    ft->dwLowDateTime  = DWORD(v & 0xFFFFFFFFULL);
    ft->dwHighDateTime = DWORD(v >> 32);
}

/// Время пользователя и ядра для ТЕКУЩЕГО потока (так его и зовут). Время
/// создания и выхода не заполняются: отчёт их не читает.
inline BOOL GetThreadTimes(void* /*thread*/, FILETIME* creation, FILETIME* exitTime,
                           FILETIME* kernel, FILETIME* user)
{
    struct rusage ru;
    if (getrusage(RUSAGE_THREAD, &ru) != 0)
        return FALSE;
    if (creation) creation->dwLowDateTime = creation->dwHighDateTime = 0;
    if (exitTime) exitTime->dwLowDateTime = exitTime->dwHighDateTime = 0;
    if (kernel) tmc_timeval_to_filetime(ru.ru_stime, kernel);
    if (user)   tmc_timeval_to_filetime(ru.ru_utime, user);
    return TRUE;
}

// --- Память процесса (psapi) -------------------------------------------------
/// Сведения о памяти процесса (поля как в psapi; заполняются только читаемые отчётом).
struct PROCESS_MEMORY_COUNTERS
{
    DWORD  cb;
    DWORD  PageFaultCount;
    size_t PeakWorkingSetSize;
    size_t WorkingSetSize;
    size_t QuotaPeakPagedPoolUsage;
    size_t QuotaPagedPoolUsage;
    size_t QuotaPeakNonPagedPoolUsage;
    size_t QuotaNonPagedPoolUsage;
    size_t PagefileUsage;
    size_t PeakPagefileUsage;
};
/// Пиковый рабочий набор — пиковая резидентная память процесса (ru_maxrss).
inline BOOL GetProcessMemoryInfo(void* /*process*/, PROCESS_MEMORY_COUNTERS* pmc, DWORD /*cb*/)
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0)
        return FALSE;
    pmc->PeakWorkingSetSize = size_t(ru.ru_maxrss) * 1024u;  // килобайты
    pmc->PageFaultCount = DWORD(ru.ru_majflt + ru.ru_minflt);
    pmc->WorkingSetSize = 0;
    if (FILE* f = std::fopen("/proc/self/statm", "r")) {
        unsigned long size = 0, resident = 0;
        if (std::fscanf(f, "%lu %lu", &size, &resident) == 2)
            pmc->WorkingSetSize = size_t(resident) * size_t(sysconf(_SC_PAGESIZE));
        std::fclose(f);
    }
    return TRUE;
}

// --- Сведения о машине -------------------------------------------------------
/// Сведения о машине (поля как в Windows; заполняются только читаемые отчётом).
struct SYSTEM_INFO
{
    WORD  wProcessorArchitecture;
    WORD  wReserved;
    DWORD dwPageSize;
    void* lpMinimumApplicationAddress;
    void* lpMaximumApplicationAddress;
    DWORD dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD  wProcessorLevel;
    WORD  wProcessorRevision;
};
/// @brief Число процессоров и размер страницы.
/// @param[out] si сведения
inline void GetSystemInfo(SYSTEM_INFO* si)
{
    std::memset(si, 0, sizeof(*si));
    const long n = sysconf(_SC_NPROCESSORS_ONLN);
    si->dwNumberOfProcessors   = DWORD(n > 0 ? n : 1);
    si->dwPageSize             = DWORD(sysconf(_SC_PAGESIZE));
    si->dwAllocationGranularity = si->dwPageSize;
}

/// Состояние памяти машины (поля как в Windows).
struct MEMORYSTATUSEX
{
    DWORD dwLength;
    DWORD dwMemoryLoad;
    unsigned long long ullTotalPhys;
    unsigned long long ullAvailPhys;
    unsigned long long ullTotalPageFile;
    unsigned long long ullAvailPageFile;
    unsigned long long ullTotalVirtual;
    unsigned long long ullAvailVirtual;
    unsigned long long ullAvailExtendedVirtual;
};
/// Свободная физическая память — MemAvailable ядра Linux: то, что можно занять,
/// не вытесняя процессы (ближе всего к ullAvailPhys Windows, куда тоже входит
/// кэш файлов).
inline BOOL GlobalMemoryStatusEx(MEMORYSTATUSEX* ms)
{
    const DWORD len = ms->dwLength;
    std::memset(ms, 0, sizeof(*ms));
    ms->dwLength = len;
    FILE* f = std::fopen("/proc/meminfo", "r");
    if (!f)
        return FALSE;
    char line[256];
    unsigned long long totalKb = 0, availKb = 0, swapTotalKb = 0, swapFreeKb = 0;
    while (std::fgets(line, sizeof(line), f)) {
        unsigned long long v = 0;
        if (std::sscanf(line, "MemTotal: %llu", &v) == 1)          totalKb = v;
        else if (std::sscanf(line, "MemAvailable: %llu", &v) == 1) availKb = v;
        else if (std::sscanf(line, "SwapTotal: %llu", &v) == 1)    swapTotalKb = v;
        else if (std::sscanf(line, "SwapFree: %llu", &v) == 1)     swapFreeKb = v;
    }
    std::fclose(f);
    if (totalKb == 0)
        return FALSE;
    ms->ullTotalPhys     = totalKb * 1024ULL;
    ms->ullAvailPhys     = availKb * 1024ULL;
    ms->ullTotalPageFile = (totalKb + swapTotalKb) * 1024ULL;
    ms->ullAvailPageFile = (availKb + swapFreeKb) * 1024ULL;
    if (ms->ullTotalPhys > 0)
        ms->dwMemoryLoad = DWORD(100 - (ms->ullAvailPhys * 100ULL) / ms->ullTotalPhys);
    return TRUE;
}

/// @brief Имя машины.
/// @param[out] buf буфер
/// @param[in,out] size размер буфера / длина имени
/// @return TRUE — имя записано.
inline BOOL GetComputerNameA(char* buf, DWORD* size)
{
    char name[256] = "";
    if (gethostname(name, sizeof(name) - 1) != 0)
        return FALSE;
    const size_t n = std::strlen(name);
    if (n + 1 > *size) {
        *size = DWORD(n + 1);
        return FALSE;
    }
    std::memcpy(buf, name, n + 1);
    *size = DWORD(n);
    return TRUE;
}

// --- Реестр: только название процессора --------------------------------------
// Отчёт о времени расчёта берёт название процессора из
// HKLM\HARDWARE\DESCRIPTION\System\CentralProcessor\0, ProcessorNameString.
// Это единственный ключ, который здесь «есть»; значение — строка «model name»
// из /proc/cpuinfo (macOS — machdep.cpu.brand_string).
/// Описатель ключа реестра.
typedef void* HKEY;
/// Указатель на байты.
typedef BYTE* LPBYTE;
/// Раздел реестра.
#define HKEY_LOCAL_MACHINE ((HKEY)(intptr_t)0x80000002)
/// Права чтения ключа.
#define KEY_READ           0x20019
#ifndef ERROR_SUCCESS
  /// Успех.
  #define ERROR_SUCCESS        0L
#endif
#ifndef ERROR_FILE_NOT_FOUND
  /// Ключ или значение не найдены.
  #define ERROR_FILE_NOT_FOUND 2L
#endif
/// Тип значения: строка.
#define REG_SZ 1

/// @brief Открывает ключ: «есть» только ключ процессора.
/// @param[in] subKey имя ключа
/// @param[out] out описатель
/// @return `ERROR_SUCCESS` или `ERROR_FILE_NOT_FOUND`.
inline LONG RegOpenKeyExA(HKEY /*root*/, const char* subKey, DWORD, DWORD, HKEY* out)
{
    if (subKey && std::strstr(subKey, "CentralProcessor")) {
        *out = (HKEY)(intptr_t)1;
        return ERROR_SUCCESS;
    }
    return ERROR_FILE_NOT_FOUND;
}

/// @brief Название процессора из `/proc/cpuinfo`.
/// @return Название или пустая строка.
inline std::string tmc_cpu_name()
{
    std::string name;
    if (FILE* f = std::fopen("/proc/cpuinfo", "r")) {
        char line[512];
        while (std::fgets(line, sizeof(line), f)) {
            if (std::strncmp(line, "model name", 10) == 0) {
                const char* p = std::strchr(line, ':');
                if (p) {
                    ++p;
                    while (*p == ' ' || *p == '\t') ++p;
                    name = p;
                    while (!name.empty() && (name.back() == '\n' || name.back() == '\r'))
                        name.pop_back();
                }
                break;
            }
        }
        std::fclose(f);
    }
    return name;
}

/// @brief Читает `ProcessorNameString`.
/// @param[in] key описатель
/// @param[in] valueName имя значения
/// @param[out] type тип
/// @param[out] data буфер
/// @param[in,out] size размер буфера / длина значения
/// @return `ERROR_SUCCESS`, `ERROR_FILE_NOT_FOUND` или 234 (мал буфер).
inline LONG RegQueryValueExA(HKEY key, const char* valueName, DWORD* /*reserved*/,
                             DWORD* type, LPBYTE data, DWORD* size)
{
    if (key != (HKEY)(intptr_t)1 || !valueName || std::strcmp(valueName, "ProcessorNameString") != 0)
        return ERROR_FILE_NOT_FOUND;
    const std::string name = tmc_cpu_name();
    if (name.empty())
        return ERROR_FILE_NOT_FOUND;
    if (type) *type = REG_SZ;
    const size_t n = name.size() + 1;
    if (!data || !size || *size < n) {
        if (size) *size = DWORD(n);
        return 234L;                     // ERROR_MORE_DATA
    }
    std::memcpy(data, name.c_str(), n);
    *size = DWORD(n);
    return ERROR_SUCCESS;
}

/// @brief Закрывает ключ.
/// @return `ERROR_SUCCESS`.
inline LONG RegCloseKey(HKEY) { return ERROR_SUCCESS; }

// --- Командная строка процесса ------------------------------------------------
/// Ядро ищет в ней ключ /GPU. Собираем строку из аргументов так, как её видит
/// Windows: через пробел, слова с пробелами — в кавычках.
inline const char* GetCommandLineA()
{
    static std::string line;
    static bool done = false;
    if (done)
        return line.c_str();
    done = true;
    if (FILE* f = std::fopen("/proc/self/cmdline", "rb")) {
        std::string raw;
        char buf[4096];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
            raw.append(buf, n);
        std::fclose(f);
        size_t i = 0;
        while (i < raw.size()) {
            const size_t e = raw.find('\0', i);
            const std::string arg = raw.substr(i, (e == std::string::npos ? raw.size() : e) - i);
            if (!line.empty())
                line += ' ';
            if (arg.find_first_of(" \t") != std::string::npos)
                line += '"' + arg + '"';
            else
                line += arg;
            if (e == std::string::npos)
                break;
            i = e + 1;
        }
    }
    return line.c_str();
}

// --- Загрузка библиотеки во время работы (OpenCL) ----------------------------
#include "tmc_dlopen.h"

// --- Полный путь ---------------------------------------------------------------
/// Как в Windows: файл может не существовать; относительный путь достраивается от
/// текущего каталога, «.» и «..» убираются. Возвращает длину без завершающего
/// нуля, а если буфер мал — нужный размер вместе с нулём; 0 — ошибка.
inline DWORD GetFullPathNameA(const char* fileName, DWORD bufLen, char* buf, char** filePart)
{
    if (!fileName || !fileName[0])
        return 0;
    std::string path = fileName;
    if (path[0] != '/') {
        char cwd[4096];
        if (!getcwd(cwd, sizeof(cwd)))
            return 0;
        path = std::string(cwd) + "/" + path;
    }
    // Нормализация: пустые части, «.» и «..».
    std::string out;
    size_t i = 0;
    while (i < path.size()) {
        size_t e = path.find('/', i);
        if (e == std::string::npos)
            e = path.size();
        const std::string part = path.substr(i, e - i);
        if (part.empty() || part == ".") {
            // пропуск
        } else if (part == "..") {
            const size_t k = out.rfind('/');
            out.erase(k == std::string::npos ? 0 : k);
        } else {
            out += '/';
            out += part;
        }
        i = e + 1;
    }
    if (out.empty())
        out = "/";
    if (!path.empty() && path.back() == '/' && out != "/")
        out += '/';

    if (size_t(bufLen) < out.size() + 1)
        return DWORD(out.size() + 1);
    std::memcpy(buf, out.c_str(), out.size() + 1);
    if (filePart) {
        char* slash = std::strrchr(buf, '/');
        *filePart = (slash && slash[1]) ? slash + 1 : nullptr;
    }
    return DWORD(out.size());
}

// --- Потоки EPSFAST2 (заполнение eps блока в несколько потоков) ---------------
// Ядро запускает std::thread и ждёт их по native_handle() — на Linux это
// pthread_t. Пока потоки работают, ядро раз в 250 мс выводит, сколько узлов
// готово.
#ifndef INFINITE
/// Ждать без ограничения.
#define INFINITE        0xFFFFFFFFu
#endif
#ifndef WAIT_OBJECT_0
/// Ожидание завершилось: все потоки вышли.
#define WAIT_OBJECT_0   0x00000000u
#endif
#ifndef WAIT_TIMEOUT
/// Ожидание завершилось по времени.
#define WAIT_TIMEOUT    0x00000102u
#endif
#ifndef THREAD_PRIORITY_BELOW_NORMAL
/// Приоритет ниже обычного.
#define THREAD_PRIORITY_BELOW_NORMAL   (-1)
#endif

/// Приоритет потока. В Windows ядро понижает его, чтобы окна оставались
/// отзывчивыми. pthread_t не даёт задать «nice» отдельного потока — оставляем
/// обычный приоритет; на результат расчёта это не влияет.
inline BOOL SetThreadPriority(void* /*thread*/, int /*priority*/) { return TRUE; }

/// Ожидание ВСЕХ потоков (bWaitAll = TRUE — единственный вызов в коде).
/// Признак завершения потока, не забирающий его (join после этого корректен):
/// у завершившегося, но ещё не присоединённого потока glibc отвечает ESRCH на
/// pthread_getcpuclockid — система обнуляет номер потока при его выходе.
inline DWORD WaitForMultipleObjects(DWORD count, void* const* handles, BOOL /*waitAll*/, DWORD ms)
{
    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (;;) {
        bool all = true;
        for (DWORD k = 0; k < count && all; ++k) {
            clockid_t cid;
            if (pthread_getcpuclockid((pthread_t)(uintptr_t)handles[k], &cid) == 0)
                all = false;
        }
        if (all)
            return WAIT_OBJECT_0;
        struct timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        const long long waited = (t.tv_sec - t0.tv_sec) * 1000LL + (t.tv_nsec - t0.tv_nsec) / 1000000LL;
        if (ms != INFINITE && waited >= (long long)ms)
            return WAIT_TIMEOUT;
        usleep(5000);
    }
}
