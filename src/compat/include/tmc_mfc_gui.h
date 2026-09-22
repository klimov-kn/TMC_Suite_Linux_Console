// tmc_mfc_gui.h — скелет GUI-классов MFC для КОНСОЛЬНОЙ сборки ядра под Linux.
//
// Зачем нужен (см. ISSUES.md R3): счётные модули ядра PlanarRT_H
// (TmcRTHRectNode.cpp, TmcRTHNodeDiel.cpp) транзитивно включают GUI-заголовки
// (PlanRT_H.h -> CWinApp; Pl_iofor.h -> MainFrm.h/PlanRT_HDoc.h/PlanRT_HView.h
// -> CMDIFrameWnd/CDocument/CScrollView/CStatusBar/CDialog/...). Сами GUI-.cpp
// в Linux-сборку НЕ входят, но их ЗАГОЛОВКИ должны хотя бы парситься компилятором.
// Здесь — пустые «болванки» базовых MFC-классов и нейтральные макросы, достаточные
// для компиляции этих объявлений. Ни одно из этих окон на Linux не создаётся и не
// вызывается (точка входа — консольный main, который работает с CTmcRTHRectNode
// напрямую, без Doc/View).
//
// ВАЖНО:
//  * Это НЕ математика и НЕ формат файлов — только перенос/нейтрализация MFC-обвязки.
//  * Активен только вне Windows: на самой Windows (этап GUI 2027) используется
//    настоящий MFC.
//  * Расширяется по мере появления новых требований при сборке ядра.

#pragma once

#ifndef _WIN32

#include "tmc_mfc.h"   // базовые win-типы + CString

// PlanRT_H.h проверяет этот guard ("include 'stdafx.h' before ..."): объявляем,
// что «MFC-ядро уже подключено» — на Linux это наш шим.
#ifndef __AFXWIN_H__
#define __AFXWIN_H__
#endif

// --- Сообщения/типы окон (нужны только как типы-носители) -------------------
typedef unsigned int  WPARAM;
typedef long          LPARAM;
typedef long          LRESULT;
typedef unsigned long COLORREF;

#ifndef WM_CLOSE
#define WM_CLOSE 0x0010
#endif

struct CREATESTRUCT { int unused; };
typedef CREATESTRUCT* LPCREATESTRUCT;

struct PROCESS_INFORMATION
{
    void* hProcess;
    void* hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
};

// --- CRT/win32-периферия, которую MSVC давал ядру транзитивно ----------------
#include <climits>     // INT_MAX и пр. (на MSVC приходили через windows.h/CRT)

// _heapmin (MSVC: тримминг кучи CRT). На glibc прямого аналога нет и он не нужен —
// это лишь подсказка аллокатору освободить память, на результат счёта не влияет.
inline int _heapmin() { return 0; }

// GetFileAttributes — в ядре используется ТОЛЬКО в IsFieldFileRead() для
// GUI-синхронизации с внешним вьюером поля (расчёт ждал, пока вьюер прочитает
// очередной кадр поля, отслеживая ARCHIVE-бит файла). В headless-CLI вьюера нет,
// поэтому ожидание должно отсутствовать: возвращаем «существует -> ARCHIVE»,
// «нет файла -> INVALID», так что IsFieldFileRead() всегда FALSE (цикл ожидания
// не выполняется). На формат/содержимое выходных файлов не влияет.
#include <sys/stat.h>
#ifndef FILE_ATTRIBUTE_ARCHIVE
#define FILE_ATTRIBUTE_ARCHIVE 0x20
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFF
#endif
inline DWORD GetFileAttributes(const char* path)
{
    struct stat st;
    if (!path) return INVALID_FILE_ATTRIBUTES;
    return (stat(path, &st) == 0) ? (DWORD)FILE_ATTRIBUTE_ARCHIVE
                                  : (DWORD)INVALID_FILE_ATTRIBUTES;
}

// --- Нейтрализация MFC-макросов ClassWizard ---------------------------------
// На Linux карты сообщений/динамическое создание окон не нужны: раскрываем в
// пустоту, чтобы тела заголовков и .cpp компилировались без изменений исходников.
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
    // На Linux окон нет: PostMessage — пустышка (вызов из PutStepEndSound
    // достижим только при IsAutoRun()==TRUE, а в CLI он всегда FALSE).
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
