// Win32 API, который понадобился коду редакции 5.0, — на POSIX (Linux, macOS).
//
// Откуда вызовы:
//   * замер времени расчёта, ключ /Time (include/tmccalctiming.h): часы высокого
//     разрешения, процессорное время потока, пиковая память, сведения о машине
//     для шапки отчёта, критическая секция;
//   * расчёт на видеокарте (kernels/planarrt_?/tmcgpu?.h): ключ /GPU ищется в
//     командной строке процесса;
//   * поле в полосах вдоль границы (libs/tmcindan/fieldintegrated.cpp): сколько
//     свободной памяти сейчас;
//   * полный путь задания (A-16, libs/tmcindan/tmcrth_indan.cpp).
//
// Ни один из этих вызовов не участвует в вычислениях: это часы, сведения о
// машине и пути. Смысл каждой функции повторён так близко к Windows, как
// позволяет система; где точного аналога нет — сказано у функции.
//
// Под Windows (в том числе MinGW-сборка Qt6-версии) заголовок пуст: там
// настоящий windows.h.
#pragma once
#ifndef _WIN32

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
#include <sys/stat.h>
#include <sys/time.h>
#if defined(__APPLE__)
  #include <sys/sysctl.h>
  #include <mach-o/dyld.h>
#endif

// --- 64-битные целые из двух половин ----------------------------------------
// DWORD в слое — unsigned long (64 бита на Linux), поэтому половины объявлены
// явно 32-битными: иначе QuadPart не совпал бы с парой LowPart/HighPart.
union LARGE_INTEGER
{
    struct { uint32_t LowPart; int32_t HighPart; };
    long long QuadPart;
};
union ULARGE_INTEGER
{
    struct { uint32_t LowPart; uint32_t HighPart; };
    unsigned long long QuadPart;
};

// --- Часы высокого разрешения ------------------------------------------------
// QueryPerformanceCounter — монотонные часы; частота — 1 ГГц (наносекунды).
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* f)
{
    f->QuadPart = 1000000000LL;
    return TRUE;
}
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* t)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t->QuadPart = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    return TRUE;
}

// --- Местное время -----------------------------------------------------------
struct SYSTEMTIME
{
    WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
};
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
typedef pthread_mutex_t CRITICAL_SECTION;
inline void InitializeCriticalSection(CRITICAL_SECTION* cs)
{
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(cs, &a);
    pthread_mutexattr_destroy(&a);
}
inline void EnterCriticalSection(CRITICAL_SECTION* cs)  { pthread_mutex_lock(cs); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs)  { pthread_mutex_unlock(cs); }
inline void DeleteCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_destroy(cs); }

// --- Текущий поток и процесс, процессорное время -----------------------------
// Псевдоописатели, как в Windows: понимают только функции ниже.
inline void* GetCurrentThread()  { return (void*)(intptr_t)-2; }
inline void* GetCurrentProcess() { return (void*)(intptr_t)-1; }

inline void tmc_timeval_to_filetime(const struct timeval& tv, FILETIME* ft)
{
    // FILETIME — сотни наносекунд.
    const unsigned long long v = (unsigned long long)tv.tv_sec * 10000000ULL
                               + (unsigned long long)tv.tv_usec * 10ULL;
    ft->dwLowDateTime  = DWORD(v & 0xFFFFFFFFULL);
    ft->dwHighDateTime = DWORD(v >> 32);
}

// Время пользователя и ядра для ТЕКУЩЕГО потока (так его и зовут). Время
// создания и выхода не заполняются: отчёт их не читает.
inline BOOL GetThreadTimes(void* /*thread*/, FILETIME* creation, FILETIME* exitTime,
                           FILETIME* kernel, FILETIME* user)
{
    struct rusage ru;
#if defined(RUSAGE_THREAD)
    if (getrusage(RUSAGE_THREAD, &ru) != 0)
        return FALSE;
#else
    if (getrusage(RUSAGE_SELF, &ru) != 0)   // macOS: времени потока нет — процесс
        return FALSE;
#endif
    if (creation) creation->dwLowDateTime = creation->dwHighDateTime = 0;
    if (exitTime) exitTime->dwLowDateTime = exitTime->dwHighDateTime = 0;
    if (kernel) tmc_timeval_to_filetime(ru.ru_stime, kernel);
    if (user)   tmc_timeval_to_filetime(ru.ru_utime, user);
    return TRUE;
}

// --- Память процесса (psapi) -------------------------------------------------
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
// Пиковый рабочий набор — пиковая резидентная память процесса (ru_maxrss).
inline BOOL GetProcessMemoryInfo(void* /*process*/, PROCESS_MEMORY_COUNTERS* pmc, DWORD /*cb*/)
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0)
        return FALSE;
#if defined(__APPLE__)
    pmc->PeakWorkingSetSize = size_t(ru.ru_maxrss);          // байты
#else
    pmc->PeakWorkingSetSize = size_t(ru.ru_maxrss) * 1024u;  // килобайты
#endif
    pmc->PageFaultCount = DWORD(ru.ru_majflt + ru.ru_minflt);
    pmc->WorkingSetSize = 0;
#if !defined(__APPLE__)
    if (FILE* f = std::fopen("/proc/self/statm", "r")) {
        unsigned long size = 0, resident = 0;
        if (std::fscanf(f, "%lu %lu", &size, &resident) == 2)
            pmc->WorkingSetSize = size_t(resident) * size_t(sysconf(_SC_PAGESIZE));
        std::fclose(f);
    }
#endif
    return TRUE;
}

// --- Сведения о машине -------------------------------------------------------
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
inline void GetSystemInfo(SYSTEM_INFO* si)
{
    std::memset(si, 0, sizeof(*si));
    const long n = sysconf(_SC_NPROCESSORS_ONLN);
    si->dwNumberOfProcessors   = DWORD(n > 0 ? n : 1);
    si->dwPageSize             = DWORD(sysconf(_SC_PAGESIZE));
    si->dwAllocationGranularity = si->dwPageSize;
}

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
// Свободная физическая память — MemAvailable ядра Linux: то, что можно занять,
// не вытесняя процессы (ближе всего к ullAvailPhys Windows, куда тоже входит
// кэш файлов).
inline BOOL GlobalMemoryStatusEx(MEMORYSTATUSEX* ms)
{
    const DWORD len = ms->dwLength;
    std::memset(ms, 0, sizeof(*ms));
    ms->dwLength = len;
#if defined(__APPLE__)
    unsigned long long total = 0;
    size_t sz = sizeof(total);
    if (sysctlbyname("hw.memsize", &total, &sz, nullptr, 0) != 0)
        return FALSE;
    ms->ullTotalPhys = total;
    ms->ullAvailPhys = (unsigned long long)sysconf(_SC_AVPHYS_PAGES) * (unsigned long long)sysconf(_SC_PAGESIZE);
#else
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
#endif
    if (ms->ullTotalPhys > 0)
        ms->dwMemoryLoad = DWORD(100 - (ms->ullAvailPhys * 100ULL) / ms->ullTotalPhys);
    return TRUE;
}

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
typedef void* HKEY;
typedef BYTE* LPBYTE;
#define HKEY_LOCAL_MACHINE ((HKEY)(intptr_t)0x80000002)
#define KEY_READ           0x20019
#ifndef ERROR_SUCCESS
  #define ERROR_SUCCESS        0L
#endif
#ifndef ERROR_FILE_NOT_FOUND
  #define ERROR_FILE_NOT_FOUND 2L
#endif
#define REG_SZ 1

inline LONG RegOpenKeyExA(HKEY /*root*/, const char* subKey, DWORD, DWORD, HKEY* out)
{
    if (subKey && std::strstr(subKey, "CentralProcessor")) {
        *out = (HKEY)(intptr_t)1;
        return ERROR_SUCCESS;
    }
    return ERROR_FILE_NOT_FOUND;
}

inline std::string tmc_cpu_name()
{
    std::string name;
#if defined(__APPLE__)
    char buf[256] = "";
    size_t sz = sizeof(buf);
    if (sysctlbyname("machdep.cpu.brand_string", buf, &sz, nullptr, 0) == 0)
        name = buf;
#else
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
#endif
    return name;
}

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

inline LONG RegCloseKey(HKEY) { return ERROR_SUCCESS; }

// --- Командная строка процесса ------------------------------------------------
// Ядро ищет в ней ключ /GPU. Собираем строку из аргументов так, как её видит
// Windows: через пробел, слова с пробелами — в кавычках.
inline const char* GetCommandLineA()
{
    static std::string line;
    static bool done = false;
    if (done)
        return line.c_str();
    done = true;
#if !defined(__APPLE__)
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
#endif
    return line.c_str();
}

// --- Загрузка библиотеки во время работы (OpenCL) ----------------------------
#include "tmc_dlopen.h"

// --- Полный путь ---------------------------------------------------------------
// Как в Windows: файл может не существовать; относительный путь достраивается от
// текущего каталога, «.» и «..» убираются. Возвращает длину без завершающего
// нуля, а если буфер мал — нужный размер вместе с нулём; 0 — ошибка.
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
#define INFINITE        0xFFFFFFFFu
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0   0x00000000u
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT    0x00000102u
#endif
#ifndef THREAD_PRIORITY_BELOW_NORMAL
#define THREAD_PRIORITY_BELOW_NORMAL   (-1)
#endif

// Приоритет потока. В Windows ядро понижает его, чтобы окна оставались
// отзывчивыми. pthread_t не даёт задать «nice» отдельного потока — оставляем
// обычный приоритет; на результат расчёта это не влияет.
inline BOOL SetThreadPriority(void* /*thread*/, int /*priority*/) { return TRUE; }

// Ожидание ВСЕХ потоков (bWaitAll = TRUE — единственный вызов в коде).
// Признак завершения потока, не забирающий его (join после этого корректен):
// у завершившегося, но ещё не присоединённого потока glibc отвечает ESRCH на
// pthread_getcpuclockid — система обнуляет номер потока при его выходе.
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

// --- Главное окно ядра после пакетного расчёта ----------------------------------
// Ядро закрывает окно: ::PostMessage( hWnd, WM_CLOSE ) (PostRunClose,
// pl_iofor.cpp). Окно — объект Qt; сообщение передаёт обработчик программы
// (у ядер — planarrt_common/planrt_compat.cpp), реализация — compat/mfc_view.cpp.
BOOL TmcIsWindow(HWND h);
BOOL TmcPostMessage(HWND h, UINT message, WPARAM wParam, LPARAM lParam);
typedef BOOL (*TmcPostMessageHandler)(HWND h, UINT message, WPARAM wParam, LPARAM lParam);
void tmc_set_post_message_handler(TmcPostMessageHandler fn);
inline BOOL IsWindow(HWND h) { return TmcIsWindow(h); }
inline BOOL PostMessage(HWND h, UINT message, WPARAM wParam, LPARAM lParam)
{
    return TmcPostMessage(h, message, wParam, lParam);
}

// --- Поле редактирования -------------------------------------------------------
// Окно «Calc. time» снимает выделение текста отчёта: SendMessage( EM_SETSEL, 0, 0 ).
#ifndef EM_SETSEL
#define EM_SETSEL 0x00B1
#endif

// --- Консоль пакетного режима (/Ar, /Arb) ---------------------------------------
// Ядро пишет ход и итог пакетного расчёта в консоль, из которой его запустили
// (ConsolePrint, pl_iofor.cpp): в файл или канал — как есть, в окно консоли —
// через WriteConsoleW. Здесь консоль — стандартный вывод процесса; обе ветки
// выводят одни и те же байты.
typedef wchar_t WCHAR;
#define STD_OUTPUT_HANDLE     ((DWORD)-11)
#define FILE_TYPE_UNKNOWN     0x0000
#define FILE_TYPE_DISK        0x0001
#define FILE_TYPE_CHAR        0x0002
#define FILE_TYPE_PIPE        0x0003
#define ATTACH_PARENT_PROCESS ((DWORD)-1)
#ifndef GENERIC_WRITE
#define GENERIC_WRITE         0x40000000u
#endif
#ifndef FILE_SHARE_WRITE
#define FILE_SHARE_WRITE      0x00000002u
#endif
#define CP_ACP                0

// Описатель стандартного вывода: метка, по которой функции ниже узнают fd 1.
inline void* tmc_stdout_handle() { return (void*)(intptr_t)-101; }
inline int   tmc_handle_fd(void* h) { return (h == tmc_stdout_handle()) ? 1 : -1; }

inline void* GetStdHandle(DWORD which)
{
    return (which == STD_OUTPUT_HANDLE) ? tmc_stdout_handle() : nullptr;
}

inline DWORD GetFileType(void* h)
{
    const int fd = tmc_handle_fd(h);
    struct stat st;
    if (fd < 0 || fstat(fd, &st) != 0)
        return FILE_TYPE_UNKNOWN;
    if (S_ISREG(st.st_mode))
        return FILE_TYPE_DISK;
    if (S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode))
        return FILE_TYPE_PIPE;
    if (S_ISCHR(st.st_mode))
        return FILE_TYPE_CHAR;
    return FILE_TYPE_UNKNOWN;
}

// Отдельной консоли родителя на Linux нет: вывод уже идёт туда же, куда у
// запустившей оболочки.
inline BOOL AttachConsole(DWORD /*process*/) { return TRUE; }

// Нужен только «CONOUT$» — консоль; это стандартный вывод.
inline void* CreateFileA(const char* name, DWORD, DWORD, void*, DWORD, DWORD, void*)
{
    if (name && std::strcmp(name, "CONOUT$") == 0)
        return tmc_stdout_handle();
    return (void*)(intptr_t)-1;          // INVALID_HANDLE_VALUE
}

inline BOOL GetConsoleMode(void* h, DWORD* mode)
{
    const int fd = tmc_handle_fd(h);
    if (mode) *mode = 0;
    return (fd >= 0 && isatty(fd)) ? TRUE : FALSE;
}

// Перевод в «широкие» символы и обратно — байт в байт: текст уже в кодировке
// системы (UTF-8), и в консоль он должен попасть без изменений.
inline int MultiByteToWideChar(unsigned /*codePage*/, DWORD /*flags*/, const char* src, int n,
                               WCHAR* dst, int cap)
{
    if (!src)
        return 0;
    if (n < 0)
        n = int(std::strlen(src)) + 1;
    if (n > cap)
        n = cap;
    for (int k = 0; k < n; ++k)
        dst[k] = WCHAR((unsigned char)src[k]);
    return n;
}

inline BOOL WriteFile(void* h, const void* buf, DWORD n, DWORD* written, void* /*overlapped*/)
{
    const int fd = tmc_handle_fd(h);
    if (written) *written = 0;
    if (fd < 0)
        return FALSE;
    const ssize_t w = ::write(fd, buf, n);
    if (w < 0)
        return FALSE;
    if (written) *written = DWORD(w);
    return TRUE;
}

inline BOOL WriteConsoleW(void* h, const WCHAR* buf, DWORD n, DWORD* written, void* reserved)
{
    std::string bytes(n, '\0');
    for (DWORD k = 0; k < n; ++k)
        bytes[k] = char(buf[k] & 0xFF);
    return WriteFile(h, bytes.data(), DWORD(bytes.size()), written, reserved);
}

#endif // !_WIN32
