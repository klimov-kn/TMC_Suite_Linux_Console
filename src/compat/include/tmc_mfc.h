// tmc_mfc.h — минимальный слой совместимости MFC/win (общий с Linux-версией 5.0 с интерфейсом).
//
// MFC на Linux нет. Вычислительные библиотеки TMC (SFILE95, PREPR, TMCIndan,
// Expr/Inter, complex, TMCLibError) используют из MFC по сути только строковый
// тип CString и несколько win-типов (BOOL/TRUE/FALSE). Здесь — их минимальные
// эквиваленты, достаточные для сборки под g++.
//
// ВАЖНО:
//  * Это НЕ математика — только перенос типов и утилит. Формулы и числа
//    не затрагиваются.
//  * CString — ТРИВИАЛЬНО-КОПИРУЕМЫЙ тип с единственным членом char* (как MFC
//    CString, внутри которого один указатель на буфер). Это критично: исходники
//    передают CString в printf/Format через "%s" (varargs). На MSVC объект
//    кладётся на стек как один указатель, и "%s" читает именно его. Нетривиально
//    копируемый тип (обёртка над std::string) g++ передавал бы по скрытой ссылке,
//    и "%s" прочитал бы мусор. Инвариант закреплён static_assert ниже.
//  * Буферы CString выделяются malloc и не освобождаются (COW через
//    переаллокацию): освобождающий деструктор сделал бы тип нетривиальным.
//  * Сообщения об ошибках библиотеки шлют через AfxMessageBox. В GUI-версии
//    консольный вывод не годится, поэтому здесь точка расширения:
//    tmc_set_message_handler() ставит обработчик, который GUI подменяет на
//    QMessageBox. Библиотеки при этом остаются БЕЗ зависимости от Qt.
//
// Активен только вне Windows: на самой Windows используется настоящий MFC.

#pragma once

#if !defined(_WIN32) || defined(TMC_QT_WIN)

#include <string>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <cerrno>
#ifdef _WIN32
#include "tmc_winapi.h"
#else
#include <sys/types.h>
#include <sys/stat.h>
#include "tmc_fileflag.h"
#endif
#include <type_traits>

#ifndef _WIN32 // Windows API (под Windows — настоящий windows.h)
// --- Базовые win-типы -------------------------------------------------------
typedef int            BOOL;
typedef char           TCHAR;
typedef const char*    LPCTSTR;
typedef char*          LPTSTR;
typedef const char*    LPCSTR;
typedef char*          LPSTR;
typedef unsigned int   UINT;
typedef void*          LPVOID;
typedef unsigned long  DWORD;
typedef unsigned short WORD;
typedef unsigned char  BYTE;
typedef unsigned long  ULONG;
// LONG — тот же знаковый 32/64-битный тип, что и long. Встречается в
// объявлениях GDI (описание растра, размеры), например в tmcgltext.h FieldView.
typedef long           LONG;
// Параметры и результат оконного сообщения. Нужны в подписях обработчиков
// (ON_MESSAGE) и методов CWnd::SendMessage/PostMessage; объявлены здесь,
// а не в mfc_dialog.h, потому что ими пользуется уже слой окон (mfc_view.h).
typedef unsigned long  WPARAM;
typedef long           LPARAM;
typedef long           LRESULT;
// Описатель окна. Настоящих окон Windows здесь нет: слой возвращает вместо
// него указатель на объект Qt, а код Windows-версии сверяет его только с нулём
// (см. CWnd::GetSafeHwnd).
typedef void*          HWND;
// Целое размером с указатель. В подписях обработчиков Windows-версии
// (например, OnTimer) стоит именно оно: если объявить обработчик с UINT,
// переопределения не получится и метод не будет вызываться никогда.
typedef unsigned long  UINT_PTR;
typedef long           INT_PTR;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#endif // !_WIN32 (Windows API)

// --- Сегментные MSVC-ключевые слова (наследие DOS и 16 бит) -----------------
// На win32/win64 MSVC они уже пустые макросы (см. нейтрализацию far/near/fortran
// в win_src/Include/Typedef.h под _NDP2). Здесь тот же приём для файлов,
// которые используют их, не подключая Typedef.h.
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

#ifndef _WIN32 // Windows API (под Windows — настоящий windows.h)
// --- FILETIME (win32-структура; на Linux нужна только как тип-носитель) -----
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

// --- SetFileAttributes ------------------------------------------------------
// На Windows снимала read-only с выходных файлов. На содержимое и формат файла
// не влияет, поэтому под Linux — пустышка с успешным кодом.
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x80
#endif
#ifndef FILE_ATTRIBUTE_ARCHIVE
#define FILE_ATTRIBUTE_ARCHIVE 0x20
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu
#endif
// --- Set/GetFileAttributes ---------------------------------------------------
// Признак ARCHIVE — протокол «кадр поля прочитан» между ядром и FieldView.
// Эмуляция на расширенных атрибутах файла — в tmc_fileflag.h (её же берёт
// FieldView 5.0, который со слоем MFC не собирается).
inline BOOL SetFileAttributes(const char* lpFileName, DWORD dwFileAttributes)
{
    return tmc_fileflag::set(lpFileName, dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE) ? TRUE : FALSE;
}

inline DWORD GetFileAttributes(const char* fileName)
{
    switch (tmc_fileflag::get(fileName)) {
    case tmc_fileflag::Missing: return DWORD(INVALID_FILE_ATTRIBUTES);
    case tmc_fileflag::Normal:  return FILE_ATTRIBUTE_NORMAL;
    default:                    return FILE_ATTRIBUTE_ARCHIVE;
    }
}

// --- Строковые функции Windows API ------------------------------------------
// lstrcpy/lstrlen — те же strcpy/strlen; в однобайтовой сборке Windows это
// буквально они. Зовутся при заполнении LOGFONT (tmcgltext.cpp,
// tmcdialogplace.cpp в FieldView).
inline char* lstrcpy(char* dst, const char* src) { return std::strcpy(dst, src ? src : ""); }
inline int   lstrlen(const char* s) { return s ? int(std::strlen(s)) : 0; }

#endif // !_WIN32 (Windows API)

// --- Сообщения из библиотек -------------------------------------------------
// Обработчик по умолчанию пишет в stderr (годится для прогонов из консоли).
// GUI ставит свой через tmc_set_message_handler и показывает QMessageBox.
typedef void (*TmcMessageHandler)(const char* text);

inline TmcMessageHandler& tmc_message_handler_slot()
{
    static TmcMessageHandler h = nullptr;
    return h;
}

inline void tmc_set_message_handler(TmcMessageHandler h)
{
    tmc_message_handler_slot() = h;
}

inline void AfxMessageBox(const char* msg)
{
    const char* text = msg ? msg : "";
    if (TmcMessageHandler h = tmc_message_handler_slot())
        h(text);
    else
        std::fprintf(stderr, "%s\n", text);
}

// --- CString (тривиально-копируемый эквивалент MFC; внутри один char*) ------
// Ни деструктора, ни пользовательских copy/move — иначе тип перестанет быть
// trivially_copyable и сломается передача в "%s" varargs (см. шапку файла).
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
    void Empty()           { m_p = tmc_cstr_empty(); }

    const char* GetString() const { return m_p; }
    operator const char*()  const { return m_p; }

    // Разбор строк, как в MFC (нужны коду многочастотности и ДН, снимок 22.10).
    // Позиции — в байтах; -1 — не найдено.
    int Find(char ch, int start = 0) const
    {
        if (start < 0 || start > GetLength()) return -1;
        const char* q = std::strchr(m_p + start, ch);
        return q ? int(q - m_p) : -1;
    }
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
    CString Right(int n) const
    {
        const int len = GetLength();
        if (n < 0) n = 0;
        if (n > len) n = len;
        return CString(m_p + (len - n));
    }
    // Как в MFC: Mid(first) — до конца строки; Mid(first, n) при n < 0 — пустая
    // строка (идиома s.Mid(a, s.Find(x) - a), когда Find вернул −1), при n больше
    // остатка — остаток. Сравнение без переполнения first + n (Т-24, Windows 5.0).
    CString Mid(int first) const
    {
        const int len = GetLength();
        if (first < 0) first = 0;
        if (first > len) first = len;
        return CString(m_p + first);
    }
    CString Mid(int first, int n) const
    {
        const int len = GetLength();
        if (first < 0) first = 0;
        if (first > len) first = len;
        if (n < 0) n = 0;
        if (n > len - first) n = len - first;
        return CString(std::string(m_p + first, size_t(n)).c_str());
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
    CString& TrimRight()
    {
        int n = GetLength();
        while (n > 0 && (m_p[n - 1] == ' ' || m_p[n - 1] == '\t' || m_p[n - 1] == '\r' || m_p[n - 1] == '\n'))
            --n;
        if (n != GetLength()) m_p = tmc_cstr_dup(std::string(m_p, size_t(n)).c_str());
        return *this;
    }

    CString& operator+=(const char* s)    { m_p = tmc_cstr_cat(m_p, s ? s : ""); return *this; }
    CString& operator+=(const CString& o) { m_p = tmc_cstr_cat(m_p, o.m_p);      return *this; }

    friend CString operator+(const CString& a, const char* b)    { CString r(a); r += b; return r; }
    friend CString operator+(const char* a, const CString& b)    { CString r(a); r += b; return r; }
    friend CString operator+(const CString& a, const CString& b) { CString r(a); r += b; return r; }

    bool operator==(const CString& o) const { return std::strcmp(m_p, o.m_p) == 0; }
    bool operator!=(const CString& o) const { return std::strcmp(m_p, o.m_p) != 0; }
    bool operator==(const char* s) const { return std::strcmp(m_p, s ? s : "") == 0; }
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

// Win32 API, который понадобился коду редакции 5.0 (замер времени, GPU, память,
// полный путь), — на POSIX. Под Windows пусто.
#include "tmc_win32_posix.h"

#endif // !_WIN32
