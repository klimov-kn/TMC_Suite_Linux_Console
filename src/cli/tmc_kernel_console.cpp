// tmc_kernel_console.cpp — консольные реализации функций, которые в Windows-сборке
// жили в GUI-файлах PL_GLFUN.CPP и Pl_iofor.cpp ядер PlanarRT_H и PlanarRT_X.
//
// ОБЩИЙ для обоих ядер: интерфейс (имена/сигнатуры PutTrace/Beep*/Is*Run/формат-
// геттеров/IsPointInTr) у H и X одинаков (X — копипаст H, R10), а нужные заголовки
// (<PlanRT_H.h>/<Pl_iofor.h>/<typerth.h>) резолвятся в каталог соответствующего
// ядра по include-пути цели. Поэтому один и тот же файл компилируется в обе цели
// (tmc_rth и tmc_rtx) — без дублирования.
//
// Эти .cpp в Linux-сборку НЕ входят (PL_GLFUN.CPP — статусбар/звук/потоки MFC,
// Pl_iofor.cpp — разбор GUI-флагов и хранилище форматов, выставляемых из диалога).
// Однако счётные модули (TmcRTHRectNode.cpp, TmcRTHNodeDiel.cpp) вызывают часть
// объявленных там функций. Здесь — их консольные эквиваленты:
//   * PutTrace/PutStatistics — прогресс в stderr вместо статусбара (диагностика
//     не теряется; на содержимое выходных файлов не влияет);
//   * PutModel/PutSinchronizFlag — косметические индикаторы статусбара -> no-op;
//   * BeepAllEnd/BeepStepEnd — звуковые сигналы окончания -> no-op;
//   * IsAutoRun/IsDemoRun/IsBatchRun — режимы автозапуска GUI -> FALSE (в CLI
//     счёт и так всегда полный прогон, окон нет);
//   * GetMainFramePointer__ — указатель на гл. окно -> nullptr (достижим лишь
//     при IsAutoRun()==TRUE для закрытия окна, что в CLI не выполняется);
//   * Get*Format — форматы вывода: возвращаем ДЕФОЛТЫ из Pl_iofor.h. Это важно
//     для совместимости вывода: в GUI они по умолчанию ровно такие, меняются
//     только пользователем через диалог, которого в CLI нет.
//
// ВНИМАНИЕ — единственная вычислительная функция: IsPointInTr (предикат «точка
// внутри треугольника») перенесена БАЙТ-В-БАЙТ из PL_GLFUN.CPP (строки 120-176),
// формула/eps/порядок не изменены (правила проекта: математику не трогаем). Закомменти-
// рованные в оригинале отладочные AfxMessageBox опущены (они и так были комментариями).

#include <afxwin.h>     // MFC-шим (CString/CWnd/CMainFrame и пр.) — только Linux
#include <PlanRT_H.h>   // -> resource.h (IDD_*), CPlanRT_HApp (только парсинг)
#include <Pl_iofor.h>   // авторитетные объявления формат-геттеров, Is*Run, GetMainFramePointer__
#include <typerth.h>    // авторитетные объявления PutTrace/PutStatistics/PutModel/Beep*/IsPointInTr

#include <cmath>
#include <cstdio>

// --- Прогресс/статус: в Windows писалось в статусбар, здесь — в stderr/no-op ---
void PutTrace( CString csTrace )
{
    std::fprintf( stderr, "%s\n", (const char*)csTrace );
}

void PutTrace( char *szTrace )
{
    std::fprintf( stderr, "%s\n", szTrace ? szTrace : "" );
}

void PutStatistics( CString csTrace )
{
    std::fprintf( stderr, "%s\n", (const char*)csTrace );
}

void PutStatistics( char *szTrace )
{
    std::fprintf( stderr, "%s\n", szTrace ? szTrace : "" );
}

void PutModel( void )
{
    // Косметический индикатор модели (Float/Double) в статусбаре — в CLI не нужен.
}

void PutSinchronizFlag( BOOL /*bTopologyFlag*/, BOOL /*bFieldFlag1*/, BOOL /*bSinchFlag1*/ )
{
    // Косметический индикатор флагов вывода в статусбаре — в CLI не нужен.
}

// --- Звуковые сигналы окончания (периферия) -> no-op ------------------------
void BeepStepEnd( int /*i*/ ) {}
void BeepAllEnd( void ) {}

// --- Режимы автозапуска GUI: в консоли всегда «полный прогон без окон» -------
BOOL IsAutoRun( void )  { return FALSE; }
BOOL IsDemoRun( void )  { return FALSE; }
BOOL IsBatchRun( void ) { return FALSE; }

// Указатель на главное окно: в CLI окон нет. Используется в PutStepEndSound()
// только внутри ветки IsAutoRun(), которая здесь не выполняется.
CMainFrame *GetMainFramePointer__( void ) { return nullptr; }

// Запуск показа поля во внешнем GUI-вьюере — в headless-CLI не нужен -> no-op.
void RunOnViewField1( void ) {}

// --- Форматы вывода: дефолты из Pl_iofor.h (FORMAT_OUT_FILE_*) ---------------
// Возвращаем изменяемые статические буферы (сигнатуры геттеров — char*),
// инициализированные значениями по умолчанию. Значения те же, что в GUI до
// любых правок пользователем, поэтому формат выходных файлов совпадает с Windows.
char *GetnTFormat( void )
{
    static char buf[] = FORMAT_OUT_FILE_NTFORMAT;
    return buf;
}
char *GetdTcurrentFormat( void )
{
    static char buf[] = FORMAT_OUT_FILE_DTCurrentFORMAT;
    return buf;
}
char *GetnBFormat( void )
{
    static char buf[] = FORMAT_OUT_FILE_NBFORMAT;
    return buf;
}
char *GetdInpFormat( void )
{
    static char buf[] = FORMAT_OUT_FILE_DINPFORMAT;
    return buf;
}
char *GetdOutFormat( void )
{
    static char buf[] = FORMAT_OUT_FILE_DOUTFORMAT;
    return buf;
}
char *GetdFieldOutFormat( void )
{
    static char buf[] = FORMAT_OUT_FILE_DFLDOUTFORMAT;
    return buf;
}

// --- Вычислительный предикат: ПЕРЕНЁС БАЙТ-В-БАЙТ из PL_GLFUN.CPP ------------
// (математика не изменена; см. шапку файла и ISSUES.md).
BOOL IsPointInTr( double x1, double y1, double x2, double y2, double x3, double y3, double x0, double y0 )
{
    double s120, s230, s310, s123, eps;

    s123 = fabs( (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1) );
    s120 = fabs( (x1-x0)*(y2-y0) - (x2-x0)*(y1-y0) );
    s230 = fabs( (x2-x0)*(y3-y0) - (x3-x0)*(y2-y0) );
    s310 = fabs( (x3-x0)*(y1-y0) - (x1-x0)*(y3-y0) );

    eps = s123*(1.e-10);
    if( (s120 + s230 + s310) > (s123 + eps) )
    {
        return FALSE;
    };

    return TRUE;
}
