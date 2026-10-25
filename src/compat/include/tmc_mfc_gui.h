// tmc_mfc_gui.h — скелет GUI-классов MFC для КОНСОЛЬНОЙ сборки ядра под Linux.
//
// Зачем нужен (см. ISSUES.md R3): счётные модули ядра PlanarRT_H
// (tmcrthrectnode.cpp, tmcrthnodediel.cpp) транзитивно включают GUI-заголовки
// (planrt_h.h -> CWinApp; pl_iofor.h -> mainfrm.h/planrt_hdoc.h/planrt_hview.h
// -> CMDIFrameWnd/CDocument/CScrollView/CStatusBar/CDialog/...). Сами GUI-.cpp
// в Linux-сборку НЕ входят, но их ЗАГОЛОВКИ должны хотя бы парситься компилятором.
// Здесь — пустые «болванки» базовых MFC-классов и нейтральные макросы, достаточные
// для компиляции этих объявлений. Ни одно из этих окон на Linux не создаётся и не
// вызывается (точка входа — консольный main, который работает с CTmcRTHRectNode
// напрямую, без Doc/View).
//
// Строки, win-типы и Win32 API кода редакции 5.0 (часы, память, потоки EPSFAST2,
// OpenCL, признак ARCHIVE) — в tmc_mfc.h и tmc_win32_posix.h, общих с
// Linux-версией с интерфейсом. Здесь только то, что нужно консольной сборке.
//
// ВАЖНО:
//  * Это НЕ математика и НЕ формат файлов — только перенос/нейтрализация MFC-обвязки.
//  * Активен только вне Windows.

#pragma once

#ifndef _WIN32

#include "tmc_mfc.h"   // базовые win-типы + CString + Win32 API редакции 5.0

// planrt_h.h проверяет этот guard ("include 'stdafx.h' before ..."): объявляем,
// что «MFC-ядро уже подключено» — на Linux это наш шим.
#ifndef __AFXWIN_H__
#define __AFXWIN_H__
#endif

// --- Сообщения/типы окон (нужны только как типы-носители) -------------------
typedef unsigned long COLORREF;
typedef void*         HANDLE;
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

#ifndef WM_CLOSE
#define WM_CLOSE 0x0010
#endif

struct CREATESTRUCT { int unused; };
typedef CREATESTRUCT* LPCREATESTRUCT;

struct PROCESS_INFORMATION
{
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
};

// --- CRT/win32-периферия, которую MSVC давал ядру транзитивно ----------------
#include <climits>     // INT_MAX и пр. (на MSVC приходили через windows.h/CRT)

// _heapmin (MSVC: тримминг кучи CRT). На glibc прямого аналога нет и он не нужен —
// это лишь подсказка аллокатору освободить память, на результат счёта не влияет.
inline int _heapmin() { return 0; }

// GetTickCount — миллисекунды монотонных часов. Код 5.0 (Т-9, EPSFAST) выводит
// по ним ход заполнения eps раз в 250 мс и ограничивает ожидание по времени.
#include <ctime>
inline DWORD GetTickCount()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return DWORD((unsigned long long)t.tv_sec * 1000ull + (unsigned long long)t.tv_nsec / 1000000ull);
}

// --- Исключения MFC ------------------------------------------------------------
// Код расчёта 5.0 (А-37) ловит catch( CException *e ). Как в Linux-версии с
// интерфейсом (tmc_mfc_doc.h); нехватка памяти в консоли приходит обычным
// std::bad_alloc, который код ловит отдельной веткой.
class CException
{
public:
    virtual ~CException() {}
    virtual BOOL GetErrorMessage(char* buf, UINT max, UINT* help = 0) const
    {
        if (help) *help = 0;
        if (buf && max) { std::strncpy(buf, what(), max - 1); buf[max - 1] = 0; }
        return TRUE;
    }
    virtual const char* what() const { return "exception"; }
    void Delete() { delete this; }
};
class CMemoryException : public CException
{
public:
    const char* what() const override { return "Not enough memory"; }
};

// --- Нейтрализация MFC-макросов ClassWizard ---------------------------------
// На Linux карты сообщений/динамическое создание окон не нужны: раскрываем в
// пустоту, чтобы тела заголовков компилировались без изменений исходников.
#define afx_msg
#define DECLARE_MESSAGE_MAP()
#define BEGIN_MESSAGE_MAP(theClass, baseClass)
#define END_MESSAGE_MAP()
#define DECLARE_DYNAMIC(class_name)
#define DECLARE_DYNCREATE(class_name)
#define DECLARE_SERIAL(class_name)
#define IMPLEMENT_DYNAMIC(class_name, base_class_name)
#define IMPLEMENT_DYNCREATE(class_name, base_class_name)
#define IMPLEMENT_SERIAL(class_name, base_class_name, wSchema)

// --- Предобъявления типов, используемых только как указатели/ссылки ---------
class CDC;
class CPrintInfo;
class CArchive;
class CDumpContext;
class CDataExchange;

// --- Скелет иерархии MFC-классов --------------------------------------------
// Все «болванки»: данных и поведения нет (на Linux эти объекты не используются),
// важны лишь имена и отношения наследования, чтобы заголовки парсились.
class CObject
{
public:
    CObject() {}
    virtual ~CObject() {}
};

class CCmdTarget : public CObject {};

class CCmdUI
{
public:
    void Enable(BOOL = TRUE) {}
    void SetCheck(int = 1) {}
    void SetText(LPCTSTR) {}
};

class CWnd : public CCmdTarget
{
public:
    CWnd* m_pMainWnd = nullptr;
    CWnd() {}
    virtual ~CWnd() {}
    // Окон нет: PostMessage — пустышка.
    BOOL PostMessage(UINT /*msg*/, WPARAM = 0, LPARAM = 0) { return TRUE; }
    BOOL RedrawWindow() { return TRUE; }
};

class CControlBar : public CWnd {};
class CStatusBar   : public CControlBar {};
class CToolBar     : public CControlBar {};

class CFrameWnd    : public CWnd {};
class CMDIFrameWnd : public CFrameWnd {};
class CMDIChildWnd : public CFrameWnd {};

class CDocument : public CCmdTarget {};

class CView : public CWnd
{
public:
    CDocument* m_pDocument = nullptr;
};
class CScrollView : public CView {};

class CDialog       : public CWnd {};
class CPropertyPage : public CDialog {};
class CPropertySheet: public CWnd {};

class CWinThread : public CCmdTarget {};
class CWinApp : public CWinThread
{
public:
    CWnd* m_pMainWnd = nullptr;
};

#endif // !_WIN32
