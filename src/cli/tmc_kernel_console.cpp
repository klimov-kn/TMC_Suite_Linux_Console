// tmc_kernel_console.cpp — консольные реализации функций, которые в Windows-сборке
// жили в GUI-файлах pl_glfun.cpp и pl_iofor.cpp ядер PlanarRT_H и PlanarRT_X.
//
// ОБЩИЙ для обоих ядер: интерфейс (имена/сигнатуры PutTrace/Beep*/Is*Run/формат-
// геттеров/ConsolePrint/IsPointInTr) у H и X одинаков, а нужные заголовки
// (<planrt_h.h>/<pl_iofor.h>/<typerth.h>) резолвятся в каталог соответствующего
// ядра по пути включения цели. Один и тот же файл компилируется в обе цели.
//
// Эти .cpp в Linux-сборку НЕ входят (pl_glfun.cpp — строка состояния, звук,
// потоки MFC; pl_iofor.cpp — флаги окна и хранилище форматов, выставляемых из
// диалога). Однако счётные модули (tmcrthrectnode.cpp, tmcrthnodediel.cpp)
// вызывают часть объявленных там функций. Здесь — их консольные эквиваленты:
//   * PutTrace/PutStatistics — строка состояния окна -> терминал (stderr):
//     в терминале — одна обновляемая строка, в файле журнала — сообщения и
//     раз в 10 с ход расчёта (ключи -q и --verbose меняют подробность);
//   * PutModel/PutSinchronizFlag — значки строки состояния -> ничего;
//   * BeepAllEnd/BeepStepEnd — звук окончания -> ничего (пакетный режим);
//   * IsAutoRun/IsBatchRun — TRUE: консольная версия всегда работает как
//     пакетный запуск /Arb Windows-версии 5.0 (нет ожидания FieldView, нет звука,
//     сообщения в консоль); IsDemoRun — FALSE;
//   * ConsolePrint/ConsoleReportResult/Set|GetRunExitCode — как в pl_iofor.cpp
//     5.0, но всегда в stdout (окна, к консоли которого надо подключаться, нет);
//   * Get*Format — форматы вывода: ДЕФОЛТЫ из pl_iofor.h. В GUI они по
//     умолчанию ровно такие и меняются только из диалога, которого в CLI нет;
//   * TmcIsWindow/TmcPostMessage — окон нет.
//
// ВНИМАНИЕ — единственная вычислительная функция: IsPointInTr (предикат «точка
// внутри треугольника») перенесена БАЙТ-В-БАЙТ из pl_glfun.cpp, формула/eps/
// порядок не изменены (правила проекта: математику не трогаем).

#include <afxwin.h>     // слой MFC (CString/CWnd/CMainFrame и пр.) — только Linux
#include <planrt_h.h>   // -> resource.h (IDD_*), CPlanRT_HApp (только разбор)
#include <pl_iofor.h>   // объявления формат-геттеров, Is*Run, ConsolePrint, TMC_EXIT_*
#include <typerth.h>    // объявления PutTrace/PutStatistics/PutModel/Beep*/IsPointInTr

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include <unistd.h>

#ifndef TMC_KERNEL_EXE
#define TMC_KERNEL_EXE "tmc_rth"
#endif

// 0 — тихо (-q), 1 — обычный, 2 — подробный (--verbose). Задаёт tmc_kernel_main.cpp.
extern int g_nTmcTraceLevel;

// --- Строка состояния -> терминал ------------------------------------------------
namespace {

bool   g_bStatusShown = false;   // в терминале стоит обновляемая строка
double g_dStatusLast  = -1.0;    // когда её обновляли (с)
double g_dLogLast     = -1.0;    // когда ход расчёта в последний раз писали в журнал (с)

double NowSec()
{
    struct timespec t;
    clock_gettime( CLOCK_MONOTONIC, &t );
    return t.tv_sec + t.tv_nsec * 1e-9;
}

bool StderrIsTerminal()
{
    static const int tty = ::isatty( STDERR_FILENO );
    return tty != 0;
}

// Строка состояния окна: ход расчёта (каждые 100 тактов), «Load prepr.»,
// «Set block …» и т. п. Отдельной строкой выводятся только важные сообщения:
// ошибки, предупреждения и на чём идёт счёт (процессор или видеокарта).
bool IsProgress( const char *s )
{
    if( std::strstr( s, "rror" ) || std::strstr( s, "WARN" ) || std::strstr( s, "arning" ) ||
        std::strstr( s, "cancel" ) || std::strncmp( s, "GPU", 3 ) == 0 ||
        std::strncmp( s, "CPU", 3 ) == 0 )
        return false;
    return true;
}

// Убрать обновляемую строку перед обычным выводом.
void ClearStatus()
{
    if( g_bStatusShown )
    {
        std::fputs( "\r\033[K", stderr );
        std::fflush( stderr );
        g_bStatusShown = false;
    }
}

void ShowStatus( const char *s, bool bStatistics )
{
    if( g_nTmcTraceLevel <= 0 || s == nullptr ) return;

    if( g_nTmcTraceLevel >= 2 )                       // всё, построчно (как версия 1.0)
    {
        if( s[0] ) std::fprintf( stderr, "%s\n", s );
        return;
    }

    const bool progress = bStatistics || IsProgress( s );
    if( StderrIsTerminal() )
    {
        if( !progress )
        {
            ClearStatus();
            std::fprintf( stderr, "%s\n", s );
            return;
        }
        if( !s[0] || std::strcmp( s, " " ) == 0 ) return;
        const double t = NowSec();
        if( g_bStatusShown && t - g_dStatusLast < 0.2 ) return;
        g_dStatusLast = t;
        std::fprintf( stderr, "\r%s\033[K", s );
        std::fflush( stderr );
        g_bStatusShown = true;
        return;
    }

    // Журнал (вывод в файл или канал): сообщения — все, ход — раз в 10 с.
    if( bStatistics ) return;
    if( progress )
    {
        if( !s[0] || std::strcmp( s, " " ) == 0 ) return;
        const double t = NowSec();
        if( g_dLogLast >= 0.0 && t - g_dLogLast < 10.0 ) return;
        g_dLogLast = t;
    }
    std::fprintf( stderr, "%s\n", s );
}

} // namespace

void PutTrace( CString csTrace )        { ShowStatus( (const char*)csTrace, false ); }
void PutTrace( char *szTrace )          { ShowStatus( szTrace ? szTrace : "", false ); }
void PutStatistics( CString csTrace )   { ShowStatus( (const char*)csTrace, true ); }
void PutStatistics( char *szTrace )     { ShowStatus( szTrace ? szTrace : "", true ); }

void PutModel( void )
{
    // Значок модели (Float/Double) в строке состояния — в CLI не нужен.
}

void PutSinchronizFlag( BOOL /*bTopologyFlag*/, BOOL /*bFieldFlag1*/, BOOL /*bSinchFlag1*/ )
{
    // Значки флагов вывода в строке состояния — в CLI не нужны.
}

// --- Звуковые сигналы окончания -> ничего (как /Arb, N-11) ------------------
void BeepStepEnd( int /*i*/ ) {}
void BeepAllEnd( void ) {}

// --- Режим запуска: всегда пакетный (/Arb Windows-версии) -------------------
BOOL IsAutoRun( void )  { return TRUE; }
BOOL IsBatchRun( void ) { return TRUE; }
BOOL IsDemoRun( void )  { return FALSE; }

// Указатель на главное окно: в CLI окон нет.
CMainFrame *GetMainFramePointer__( void ) { return nullptr; }

// Запуск показа поля во внешнем вьювере (только демонстрационный режим /d).
void RunOnViewField1( void ) {}

// --- Окна (PostRunClose Windows-версии): их нет -----------------------------
BOOL TmcIsWindow( HWND ) { return FALSE; }
BOOL TmcPostMessage( HWND, UINT, WPARAM, LPARAM ) { return FALSE; }

// --- Код завершения и сообщения пакетного режима (как pl_iofor.cpp 5.0) -----
static volatile int nRunExitCode = TMC_EXIT_OK;

void SetRunExitCode( int nCode ) { nRunExitCode = nCode; }
int  GetRunExitCode( void )      { return nRunExitCode; }

// Сообщение в stdout с именем программы: «[tmc_rth] Step 1 of 2: start».
// Текст — тот же, что пишет Windows-версия в пакетном режиме.
void ConsolePrint( const char *pszFormat, ... )
{
    char szBuf[4096];
    va_list args;
    va_start( args, pszFormat );
    std::vsnprintf( szBuf, sizeof( szBuf ), pszFormat, args );
    va_end( args );

    ClearStatus();
    std::fprintf( stdout, "[%s] %s\n", TMC_KERNEL_EXE, szBuf );
    std::fflush( stdout );
}

void ConsoleReportResult( void )
{
    switch( GetRunExitCode() )
    {
        case TMC_EXIT_OK:
            ConsolePrint( "RESULT %d: OK - all steps are calculated", TMC_EXIT_OK );
            break;
        case TMC_EXIT_TPL_ERROR:
            ConsolePrint( "RESULT %d: ERROR in .tpl file - calculation is not done", TMC_EXIT_TPL_ERROR );
            break;
        case TMC_EXIT_CALC_ERROR:
            ConsolePrint( "RESULT %d: ERROR during calculation", TMC_EXIT_CALC_ERROR );
            break;
        default:
            ConsolePrint( "RESULT %d: STOPPED - calculation is not finished", GetRunExitCode() );
            break;
    };
}

// --- Форматы вывода: дефолты из pl_iofor.h (FORMAT_OUT_FILE_*) ---------------
// Изменяемые статические буферы (сигнатуры геттеров — char*), заполненные
// значениями по умолчанию: формат выходных файлов совпадает с Windows.
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

// --- Вычислительный предикат: ПЕРЕНЕСЁН БАЙТ-В-БАЙТ из pl_glfun.cpp ----------
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
