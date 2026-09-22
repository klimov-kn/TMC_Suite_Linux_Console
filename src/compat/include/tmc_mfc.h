// tmc_mfc.h — минимальный слой совместимости MFC для КОНСОЛЬНОГО порта под Linux.
//
// MFC на Linux нет и не будет (см. ISSUES.md R3). Счётные библиотеки/ядра TMC
// используют из MFC по сути только строковый тип CString и несколько win-типов
// (BOOL/TRUE/FALSE). Здесь — их минимальные эквиваленты, достаточные для сборки
// консольной версии. Расширяется по мере появления новых требований в модулях.
//
// ВАЖНО:
//  * Это НЕ математика — только перенос типов/утилит. Формулы/числа не затрагиваются.
//  * CString — ТРИВИАЛЬНО-КОПИРУЕМЫЙ тип с единственным членом char* (как у MFC
//    CString, у которого внутри один указатель на буфер). Это критично: исходники
//    ядра передают CString в printf/Format через "%s" (varargs). На 32-битном MSVC
//    объект кладётся на стек как один указатель, и "%s" читает именно его. На g++
//    (SysV x86-64) нетривиально-копируемый тип (обёртка std::string) передавался бы
//    "по скрытой ссылке" -> "%s" прочитал бы мусор (UB). Поэтому CString должен
//    оставаться trivially_copyable (см. static_assert ниже). Память буферов НЕ
//    освобождается (COW через переаллокацию, как refcount-буфер MFC, только без
//    free): тривиальная копируемость запрещает освобождающий деструктор. Для режима
//    "файл -> расчёт -> файл" утечка ограничена одним прогоном и возвращается ОС.
//  * Сохранён привычный MFC-интерфейс (Format, операторы, неявная конверсия в
//    const char*), чтобы исходники ядра компилировались без правок их логики.
//  * Задел под GUI 2027: позже шим можно расширить, не переписывая ядро.
//
// Активен только вне Windows: на самой Windows используется настоящий MFC.

#pragma once

#ifndef _WIN32

#include <string>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <type_traits>

// --- Базовые win-типы -------------------------------------------------------
typedef int           BOOL;
typedef char          TCHAR;
typedef const char*   LPCTSTR;
typedef char*         LPTSTR;
typedef const char*   LPCSTR;
typedef char*         LPSTR;
typedef unsigned int  UINT;
typedef void*         LPVOID;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char  BYTE;
typedef unsigned long  ULONG;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// --- Сегментные MSVC-ключевые слова (DOS/16-битное наследие) ---------------
// На win32/win64 MSVC они уже пустые макросы (см. аналогичную нейтрализацию
// far/_far/near/_near/fortran в win_src/Include/Typedef.h под _NDP2). Здесь —
// тот же приём для файлов, которые их используют без подключения Typedef.h.
#ifndef far
#define far
#endif
#ifndef _far
#define _far
#endif
#ifndef near
#define near
#endif
#ifndef _near
#define _near
#endif
#ifndef huge
#define huge
#endif
#ifndef _huge
#define _huge
#endif
#ifndef fortran
#define fortran
#endif
#ifndef _fortran
#define _fortran
#endif
#ifndef _pascal
#define _pascal
#endif
#ifndef _cdecl
#define _cdecl
#endif

// --- FILETIME (win32-структура; на Linux нужна только как тип-носитель) ----
struct FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

// --- Sleep (периферия, не математика) ---------------------------------------
#include <thread>
#include <chrono>
inline void Sleep(unsigned long ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// --- SetFileAttributes (на Linux не нужно: снятие read-only с выходных
// файлов — не влияет на содержимое/формат файла) ----------------------------
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x80
#endif
inline BOOL SetFileAttributes(const char* /*lpFileName*/, DWORD /*dwFileAttributes*/)
{
    return TRUE;
}

// --- AfxMessageBox (консольная замена GUI-окна, см. ISSUES.md R3) ----------
#include <cstdio>
inline void AfxMessageBox(const char* msg)
{
    std::fprintf(stderr, "%s\n", msg ? msg : "");
}

// --- CString (тривиально-копируемый эквивалент MFC; внутри один char*) ------
// ВАЖНО: ни деструктора, ни пользовательских copy/move — иначе тип перестанет
// быть trivially_copyable и сломается передача в "%s" varargs (см. шапку файла).
// Буферы выделяются malloc и НЕ освобождаются (см. tmc_cstr_dup/cat/fmt).
class CString
{
public:
    CString() : m_p(tmc_cstr_empty()) {}
    CString(const char* s) : m_p(tmc_cstr_dup(s ? s : "")) {}
    // copy/move/assign/деструктор — НЕ объявлены: компилятор генерирует
    // тривиальные (поверхностное копирование указателя). Это и требуется.

    // Присваивание C-строки: выделяем новый буфер (старый не трогаем — COW).
    CString& operator=(const char* s) { m_p = tmc_cstr_dup(s ? s : ""); return *this; }

    // MFC CString::Format — printf-стиль. Новый буфер, прежний остаётся (утечка).
    void Format(const char* fmt, ...)
    {
        va_list ap;  va_start(ap, fmt);
        va_list ap2; va_copy(ap2, ap);
        int n = std::vsnprintf(nullptr, 0, fmt, ap);
        va_end(ap);
        if (n < 0) { m_p = tmc_cstr_empty(); va_end(ap2); return; }
        char* buf = static_cast<char*>(std::malloc(static_cast<size_t>(n) + 1));
        std::vsnprintf(buf, static_cast<size_t>(n) + 1, fmt, ap2);
        va_end(ap2);
        m_p = buf;
    }

    int  GetLength() const { return static_cast<int>(std::strlen(m_p)); }
    BOOL IsEmpty()   const { return (m_p[0] == '\0') ? TRUE : FALSE; }
    void Empty()           { m_p = tmc_cstr_empty(); }

    // Неявная конверсия в C-строку — как MFC operator LPCTSTR.
    const char* GetString() const { return m_p; }
    operator const char*()  const { return m_p; }

    CString& operator+=(const char* s)    { m_p = tmc_cstr_cat(m_p, s ? s : ""); return *this; }
    CString& operator+=(const CString& o) { m_p = tmc_cstr_cat(m_p, o.m_p);      return *this; }

    friend CString operator+(const CString& a, const char* b)   { CString r(a); r += b; return r; }
    friend CString operator+(const char* a, const CString& b)   { CString r(a); r += b; return r; }
    friend CString operator+(const CString& a, const CString& b){ CString r(a); r += b; return r; }

    bool operator==(const CString& o) const { return std::strcmp(m_p, o.m_p) == 0; }
    bool operator!=(const CString& o) const { return std::strcmp(m_p, o.m_p) != 0; }
    bool operator==(const char* s) const { return std::strcmp(m_p, s ? s : "") == 0; }
    bool operator!=(const char* s) const { return std::strcmp(m_p, s ? s : "") != 0; }

private:
    // --- Управление буферами (без free: тривиальная копируемость это требует) ---
    static char* tmc_cstr_empty()
    {
        static char e[1] = { '\0' };   // общий пустой буфер; никогда не пишется на месте
        return e;
    }
    static char* tmc_cstr_dup(const char* s)
    {
        size_t n = std::strlen(s) + 1;
        char* p = static_cast<char*>(std::malloc(n));
        std::memcpy(p, s, n);
        return p;
    }
    static char* tmc_cstr_cat(const char* a, const char* b)
    {
        size_t na = std::strlen(a), nb = std::strlen(b);
        char* p = static_cast<char*>(std::malloc(na + nb + 1));
        std::memcpy(p, a, na);
        std::memcpy(p + na, b, nb + 1);   // +1: копируем и '\0'
        return p;
    }

    char* m_p;   // единственный член -> объект ABI-эквивалентен указателю
};

// Гарантия инварианта, от которого зависит передача CString в "%s" varargs.
static_assert(std::is_trivially_copyable<CString>::value,
              "CString must be trivially copyable (varargs %s relies on it)");

#endif // !_WIN32
