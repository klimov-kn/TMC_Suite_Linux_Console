/// @file tmc_kernel_console.cpp
/// @brief Консольные реализации функций, объявленных в `typerth.h` и `pl_iofor.h`: ход расчёта,
///        код завершения, сообщения пакетного режима, форматы вывода, `IsPointInTr`.
///
/// Общий для обоих ядер: нужные заголовки находятся в каталоге ядра. Консольная версия всегда
/// работает как пакетный запуск `/Arb` Windows-версии: нет ожидания FieldView, звука и окон.
/// Строка состояния окна становится выводом в поток ошибок (ключи `-q` и `--verbose` меняют
/// подробность), сообщения пакетного режима идут в stdout. Форматы вывода — значения по
/// умолчанию из `pl_iofor.h`: в окне их менял диалог, которого здесь нет.
/// Единственная вычислительная функция — `IsPointInTr`, перенесена байт в байт.
/// @see https://www.tamic.ru/docs/platforms/linux-cli.html

#include <pl_iofor.h>   // объявления формат-геттеров, Is*Run, ConsolePrint, TMC_EXIT_*
#include <typerth.h>    // объявления PutTrace/PutStatistics/PutModel/Beep*/IsPointInTr

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include <unistd.h>

/// Подробность хода расчёта; определена в `tmc_kernel_main.cpp`.
extern int g_nTmcTraceLevel;

// --- Строка состояния -> терминал ------------------------------------------------
namespace {

bool   g_bStatusShown = false;   ///< В терминале стоит обновляемая строка.
double g_dStatusLast  = -1.0;    ///< Когда её обновляли, с.
double g_dLogLast     = -1.0;    ///< Когда ход расчёта в последний раз писали в журнал, с.

/// @brief Монотонные часы.
/// @return Время, с.
double NowSec()
{
    struct timespec t;
    clock_gettime( CLOCK_MONOTONIC, &t );
    return t.tv_sec + t.tv_nsec * 1e-9;
}

/// @brief Сообщает, идёт ли поток ошибок в терминал.
/// @return true — терминал; иначе файл или канал.
bool StderrIsTerminal()
{
    static const int tty = ::isatty( STDERR_FILENO );
    return tty != 0;
}

/// @brief Отделяет ход расчёта («Load prepr.», «Set block …») от важных сообщений:
///        ошибок, предупреждений и того, на чём идёт счёт.
/// @param[in] s строка состояния
/// @return true — ход расчёта.
bool IsProgress( const char *s )
{
    if( std::strstr( s, "rror" ) || std::strstr( s, "WARN" ) || std::strstr( s, "arning" ) ||
        std::strstr( s, "cancel" ) || std::strncmp( s, "GPU", 3 ) == 0 ||
        std::strncmp( s, "CPU", 3 ) == 0 )
        return false;
    return true;
}

/// @brief Убирает обновляемую строку перед обычным выводом.
void ClearStatus()
{
    if( g_bStatusShown )
    {
        std::fputs( "\r\033[K", stderr );
        std::fflush( stderr );
        g_bStatusShown = false;
    }
}

/// @brief Выводит строку состояния: в терминал — одной обновляемой строкой, в журнал — сообщения
///        и раз в 10 с ход расчёта.
/// @param[in] s текст
/// @param[in] bStatistics true — вторая панель строки состояния (такты)
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

void PutTrace( CString csTrace )          { ShowStatus( (const char*)csTrace, false ); }
void PutTrace( const char *szTrace )      { ShowStatus( szTrace ? szTrace : "", false ); }
void PutStatistics( CString csTrace )     { ShowStatus( (const char*)csTrace, true ); }
void PutStatistics( const char *szTrace ) { ShowStatus( szTrace ? szTrace : "", true ); }

// --- Код завершения и сообщения пакетного режима (как pl_iofor.cpp 5.0) -----
/// Код завершения процесса (`TMC_EXIT_*`).
static volatile int nRunExitCode = TMC_EXIT_OK;

void SetRunExitCode( int nCode ) { nRunExitCode = nCode; }
int  GetRunExitCode( void )      { return nRunExitCode; }

/// Последнее предупреждение расчёта: повторяется в конце, как в строке состояния окна.
static char szRunWarning[1024] = "";

void SetRunWarning( const char *pszText )
{
    std::snprintf( szRunWarning, sizeof( szRunWarning ), "%s", pszText ? pszText : "" );
}

const char *GetRunWarning( void ) { return szRunWarning; }

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
// (математика не изменена; см. описание файла).
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
