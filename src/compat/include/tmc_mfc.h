/// @file tmc_mfc.h
/// @brief Минимальный слой типов Windows и MFC для кода снимка: `CString`, целые типы, `Sleep`, `GetTickCount`.
///
/// MFC на Linux нет. Вычислительные библиотеки TMC берут из неё по сути только строку
/// `CString` и несколько типов. Формулы и числа здесь не затрагиваются.
///
/// `CString` — тривиально копируемый тип с единственным членом `char*`, как в MFC: код
/// передаёт его в `printf`-функции. Буферы выделяются `malloc` и не освобождаются:
/// освобождающий деструктор сделал бы тип нетривиальным. Инвариант закреплён `static_assert`.
/// @see https://www.tamic.ru/docs/code/compat.html

#pragma once

#include <chrono>
#include <climits>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <new>
#include <string>
#include <thread>
#include <type_traits>

/// Логический тип Windows.
typedef int            BOOL;
/// Символ.
typedef char           TCHAR;
/// Указатель на неизменяемую строку.
typedef const char*    LPCTSTR;
/// Указатель на изменяемую строку.
typedef char*          LPTSTR;
/// Указатель на неизменяемую однобайтовую строку.
typedef const char*    LPCSTR;
/// Указатель на изменяемую однобайтовую строку.
typedef char*          LPSTR;
/// Беззнаковое целое.
typedef unsigned int   UINT;
/// Нетипизированный указатель.
typedef void*          LPVOID;
/// Беззнаковое «двойное слово» (здесь 64 бита; в файлы не пишется).
typedef unsigned long  DWORD;
/// Беззнаковое 16-битное слово.
typedef unsigned short WORD;
/// Байт.
typedef unsigned char  BYTE;
/// Беззнаковое длинное целое.
typedef unsigned long  ULONG;
/// Знаковое длинное целое.
typedef long           LONG;
/// Описатель объекта системы.
typedef void*          HANDLE;
/// Знаковое целое размером с указатель.
typedef long           INT_PTR;

#ifndef TRUE
/// Истина.
#define TRUE  1
#endif
#ifndef FALSE
/// Ложь.
#define FALSE 0
#endif

/// Время файла Windows, сотни наносекунд: две половины.
struct FILETIME
{
    DWORD dwLowDateTime;    ///< Младшая половина.
    DWORD dwHighDateTime;   ///< Старшая половина.
};

/// @brief Приостанавливает поток.
/// @param[in] ms длительность, мс
inline void Sleep(unsigned long ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

/// @brief Миллисекунды монотонных часов: ход заполнения eps и ограничения ожидания.
/// @return Значение часов, мс.
inline DWORD GetTickCount()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return DWORD((unsigned long long)t.tv_sec * 1000ull + (unsigned long long)t.tv_nsec / 1000000ull);
}

/// Строка с интерфейсом MFC. Ни деструктора, ни своих копирования и перемещения:
/// тип обязан оставаться тривиально копируемым (см. описание файла).
class CString
{
public:
    CString() : m_p(tmc_cstr_empty()) {}
    CString(const char* s) : m_p(tmc_cstr_dup(s ? s : "")) {}

    CString& operator=(const char* s) { m_p = tmc_cstr_dup(s ? s : ""); return *this; }

    // MFC CString::Format — printf-стиль. Новый буфер, прежний остаётся.
    // Размер «I64» (%I64d, %I64u …) — расширение MSVC; glibc прочла бы «I» как
    // свой флаг, а «64» — как ширину поля. Переводим в стандартное «ll».
    void Format(const char* fmt0, ...)
    {
        std::string fixed;
        const char* fmt = fmt0;
        if (fmt0 && std::strstr(fmt0, "I64")) {
            fixed = fmt0;
            for (size_t i = 0; (i = fixed.find('%', i)) != std::string::npos; ) {
                size_t j = i + 1;
                while (j < fixed.size() && std::strchr("-+ #0123456789.*", fixed[j])) ++j;
                if (fixed.compare(j, 3, "I64") == 0)
                    fixed.replace(j, 3, "ll");
                i = j + 1;
            }
            fmt = fixed.c_str();
        }
        va_list ap;  va_start(ap, fmt0);
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
    BOOL IsEmpty()   const { return (m_p[0] == 0) ? TRUE : FALSE; }

    operator const char*()  const { return m_p; }

    // Разбор строк, как в MFC (нужны коду многочастотности и ДН, снимок 22.10).
    // Позиции — в байтах; -1 — не найдено.
    int Find(const char* sub, int start = 0) const
    {
        if (start < 0 || start > GetLength()) return -1;
        const char* q = std::strstr(m_p + start, sub ? sub : "");
        return q ? int(q - m_p) : -1;
    }
    int ReverseFind(char ch) const
    {
        const char* q = std::strrchr(m_p, ch);
        return q ? int(q - m_p) : -1;
    }
    int FindOneOf(const char* set) const
    {
        const size_t n = std::strcspn(m_p, set ? set : "");
        return m_p[n] ? int(n) : -1;
    }
    CString Left(int n) const
    {
        const int len = GetLength();
        if (n < 0) n = 0;
        if (n > len) n = len;
        return CString(std::string(m_p, size_t(n)).c_str());
    }
    int Replace(const char* from, const char* to)
    {
        const size_t nf = from ? std::strlen(from) : 0;
        if (nf == 0) return 0;
        std::string s(m_p), t(to ? to : "");
        int count = 0;
        for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += t.size(), ++count)
            s.replace(pos, nf, t);
        if (count) m_p = tmc_cstr_dup(s.c_str());
        return count;
    }
    CString& operator+=(const char* s)    { m_p = tmc_cstr_cat(m_p, s ? s : ""); return *this; }
    CString& operator+=(const CString& o) { m_p = tmc_cstr_cat(m_p, o.m_p);      return *this; }

    friend CString operator+(const CString& a, const char* b)    { CString r(a); r += b; return r; }
    friend CString operator+(const char* a, const CString& b)    { CString r(a); r += b; return r; }
    friend CString operator+(const CString& a, const CString& b) { CString r(a); r += b; return r; }

    bool operator!=(const char* s) const { return std::strcmp(m_p, s ? s : "") != 0; }

private:
    static char* tmc_cstr_empty()
    {
        static char e[1] = { 0 };   // общий пустой буфер, на месте не пишется
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
        std::memcpy(p + na, b, nb + 1);   // +1: копируем и завершающий ноль
        return p;
    }

    char* m_p;   // единственный член -> объект ABI-эквивалентен указателю
};

static_assert(std::is_trivially_copyable<CString>::value,
              "CString must be trivially copyable (varargs %s relies on it)");

#include "tmc_win32_posix.h"
