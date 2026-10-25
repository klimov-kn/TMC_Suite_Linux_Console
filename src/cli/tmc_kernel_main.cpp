// tmc_kernel_main.cpp — консольная точка входа счётных ядер (tmc_rth, tmc_rtx).
//
// Один файл на оба ядра: заголовок <tmcrthrectnode.h> находится в каталоге
// своего ядра (путь включения цели), а различия — макросы TMC_KERNEL_MODE
// ("H"/"X") и TMC_KERNEL_EXE из CMake.
//
// Режим работы: файл -> расчёт -> файл. Имя ВХОДНОГО файла (.tpl) — аргумент.
// Имена ВЫХОДНЫХ файлов берутся из самого задания (раздел #OUTPUT), файлы
// пишутся рядом с заданием — как в Windows-версии.
//
// Цикл повторяет пакетный расчёт Windows-версии 5.0 — CPlanRT_HDoc::RunAll() и
// RunAll1() (planrt_hdoc.cpp) — без окон: замер времени (ключ -Time), те же
// сообщения в консоль и те же коды завершения
//     0 — все шаги посчитаны, 1 — ошибка задания, 2 — ошибка расчёта,
//     3 — расчёт остановлен (Ctrl+C, сигнал TERM).
// Расчёт на видеокарте (-GPU, -GPU:n, -CPU, переменная TMC_GPU) ядро выбирает
// само по командной строке процесса (tmcgpuh.h / tmcgpux.h).
// Математика не затронута.

#include <afxwin.h>            // слой MFC (CString и пр.) — только Linux
#include <tmcrthrectnode.h>    // CTmcRTHRectNode — вычислительный класс ядра
#include <planrt_h.h>          // -> resource.h (IDD_*): его ждут заголовки pl_iofor.h
#include <pl_iofor.h>          // TMC_EXIT_*, ConsolePrint

#include "tmc_about.h"

#include <cerrno>
#include <climits>
#include <clocale>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

#include <unistd.h>

// Режим вывода хода расчёта (строка состояния Windows-версии) — для
// tmc_kernel_console.cpp: 0 — тихо, 1 — обычный, 2 — подробный.
int g_nTmcTraceLevel = 1;

namespace {

volatile std::sig_atomic_t g_bStop = 0;
CTmcRTHRectNode *g_pNode = nullptr;

// Ctrl+C / kill: расчёт останавливается так же, как кнопкой «Стоп» в окне
// (CTmcRTHRectNode::Stop — снять признак счёта); итог — код 3.
extern "C" void OnStopSignal( int )
{
    g_bStop = 1;
    if( g_pNode ) g_pNode->Stop();
}

// Ключ: «-имя» или «--имя» без учёта регистра (как /Time, /GPU в Windows).
bool IsKey( const char *arg, const char *name )
{
    if( arg[0] != '-' ) return false;
    const char *k = ( arg[1] == '-' ) ? arg + 2 : arg + 1;
    return strcasecmp( k, name ) == 0;
}

bool IsGpuKey( const char *arg )
{
    if( arg[0] != '-' ) return false;
    const char *k = ( arg[1] == '-' ) ? arg + 2 : arg + 1;
    if( strncasecmp( k, "GPU", 3 ) != 0 ) return false;
    return k[3] == '\0' || k[3] == ':' || k[3] == '=';
}

void PrintUsage( const char *exe )
{
    const bool ru = TmcRussian();
    if( ru )
        std::printf(
            "Запуск:  %s [ключи] <задание.tpl>\n"
            "\n"
            "  <задание.tpl>  файл задания TMC. Выходные файлы (имена — в разделе\n"
            "                 #OUTPUT задания) пишутся рядом с заданием.\n"
            "\n"
            "Ключи (регистр не важен):\n"
            "  -Time         отчёт о времени расчёта <задание>.timing_%s_linux64.txt\n"
            "  -GPU, -GPU:n  считать на видеокарте n (OpenCL, двойная точность; n с 0);\n"
            "                то же — переменная TMC_GPU=n+1\n"
            "  -CPU          считать на процессоре (сильнее -GPU и TMC_GPU)\n"
            "  -DN           после расчёта записать диаграммы направленности (если они\n"
            "                заданы в задании); то же — переменная TMC_AUTO_DN=1\n"
            "  -q, --quiet   не выводить ход расчёта (строку состояния)\n"
            "  --verbose     выводить ход расчёта целиком, каждую строку\n"
            "  --about, -v, --version   о программе\n"
            "  --manual      открыть руководство пользователя\n"
            "  --lang=ru|en  язык сообщений (запоминается; общий с версией с окнами)\n"
            "  -h, --help    эта справка\n"
            "\n"
            "Код завершения: 0 — все шаги посчитаны, 1 — ошибка задания,\n"
            "                2 — ошибка расчёта, 3 — расчёт остановлен (Ctrl+C).\n",
            exe, TMC_KERNEL_MODE );
    else
        std::printf(
            "Usage:  %s [keys] <task.tpl>\n"
            "\n"
            "  <task.tpl>     TMC task file. Output files (named in the #OUTPUT section\n"
            "                 of the task) are written next to the task.\n"
            "\n"
            "Keys (case does not matter):\n"
            "  -Time         calculation time report <task>.timing_%s_linux64.txt\n"
            "  -GPU, -GPU:n  calculate on video card n (OpenCL, double precision; n from 0);\n"
            "                the same: environment variable TMC_GPU=n+1\n"
            "  -CPU          calculate on the CPU (stronger than -GPU and TMC_GPU)\n"
            "  -DN           write the directional pattern files after the calculation\n"
            "                (if the task defines them); the same: TMC_AUTO_DN=1\n"
            "  -q, --quiet   do not print the calculation progress (status line)\n"
            "  --verbose     print the whole progress, every line\n"
            "  --about, -v, --version   about the program\n"
            "  --manual      open the user manual\n"
            "  --lang=ru|en  message language (remembered; shared with the windowed version)\n"
            "  -h, --help    this help\n"
            "\n"
            "Exit code: 0 - all steps are calculated, 1 - error in the task,\n"
            "           2 - error during the calculation, 3 - stopped (Ctrl+C).\n",
            exe, TMC_KERNEL_MODE );
}

// Полный путь к заданию: каталог задания нужен ядру для вложенных файлов
// (#include, распределения .eps) и выходных файлов — как GetPathName() в Windows.
std::string FullPath( const char *path )
{
    char buf[PATH_MAX];
    if( ::realpath( path, buf ) ) return std::string( buf );
    return std::string( path );
}

// Тело CPlanRT_HDoc::RunAll1(): все шаги задания по порядку.
void RunAll1( CTmcRTHRectNode &cRectNode, const CString &csPath )
{
    int i, n, nStep = 1;

    __int64 qTmPr = TmcTimingNow();                                 // TIMING
    n = cRectNode.GetNStepMax( csPath ) + 1;
    cRectNode.GetTiming().qRunPrepr += TmcTimingNow() - qTmPr;      // TIMING

    // ошибка препроцессора или нет #STEP в .tpl — считать нечего
    if( cRectNode.IsError() )
    {
        ConsolePrint( "ERROR in .tpl: %s", (LPCTSTR)cRectNode.GetErrorMessage() );
        SetRunExitCode( TMC_EXIT_TPL_ERROR );
        return;
    };
    if( n - 1 < nStep )
    {
        ConsolePrint( "ERROR in .tpl: no #STEP to calculate (steps found: %d)", n - 1 );
        SetRunExitCode( TMC_EXIT_TPL_ERROR );
        return;
    };
    ConsolePrint( "Steps: %d", n - 1 );

    for( i = nStep; i < n; i++ )
    {
        nStep = i;

        if( g_bStop )
        {
            ConsolePrint( "STOPPED before step %d of %d", nStep, n - 1 );
            SetRunExitCode( TMC_EXIT_ABORTED );
            return;
        };
        cRectNode.ReadData( csPath, nStep );
        if( cRectNode.IsError() )
        {
            ConsolePrint( "ERROR in .tpl (step %d of %d): %s", nStep, n - 1,
                (LPCTSTR)cRectNode.GetErrorMessage() );
            SetRunExitCode( TMC_EXIT_TPL_ERROR );
            return;
        };

        ConsolePrint( "Step %d of %d: start", nStep, n - 1 );

        if( g_bStop )
        {
            ConsolePrint( "STOPPED at step %d of %d", nStep, n - 1 );
            SetRunExitCode( TMC_EXIT_ABORTED );
            return;
        };
        cRectNode.RunStep();
        if( cRectNode.IsError() )
        {
            ConsolePrint( "ERROR in calculation (step %d of %d): %s", nStep, n - 1,
                (LPCTSTR)cRectNode.GetErrorMessage() );
            SetRunExitCode( TMC_EXIT_CALC_ERROR );
            return;
        };
        if( g_bStop )
        {
            ConsolePrint( "STOPPED at step %d of %d", nStep, n - 1 );
            SetRunExitCode( TMC_EXIT_ABORTED );
            return;
        };

        ConsolePrint( "Step %d of %d: done", nStep, n - 1 );
    };

    SetRunExitCode( TMC_EXIT_OK );
}

} // namespace

int main( int argc, char **argv )
{
    // Числа в файлах TMC пишутся и читаются только с ТОЧКОЙ — так их создаёт
    // Windows-версия, и так их ждут все sscanf("%lg") в перенесённом коде.
    // Закрепляем числовую локаль явно, чтобы окружение (русская локаль —
    // запятая) не повлияло на выходные файлы.
    std::setlocale( LC_NUMERIC, "C" );

    const char *exe = TMC_KERNEL_EXE;
    const char *pszTask = nullptr;
    bool bTime = false;

    // Язык сообщений (общий выбор с версией с интерфейсом). Спросить можно только
    // в справочных командах, запущенных из терминала, — расчёт и скрипты ничего
    // не спрашивают.
    const char *pszLang = nullptr;
    bool bInfo = ( argc < 2 );
    for( int i = 1; i < argc; i++ )
    {
        const char *a = argv[i];
        if( std::strncmp( a, "--lang=", 7 ) == 0 ) pszLang = a + 7;
        else if( std::strncmp( a, "-lang=", 6 ) == 0 ) pszLang = a + 6;
        if( IsKey( a, "h" ) || IsKey( a, "help" ) || std::strcmp( a, "-?" ) == 0 ||
            IsKey( a, "about" ) || IsKey( a, "version" ) || std::strcmp( a, "-v" ) == 0 ||
            IsKey( a, "manual" ) )
            bInfo = true;
    }
    if( !TmcInitLanguage( pszLang, bInfo ) )
    {
        std::fprintf( stderr, "%s: --lang=ru | en | system\n", exe );
        return TMC_EXIT_TPL_ERROR;
    }

    for( int i = 1; i < argc; i++ )
    {
        const char *a = argv[i];
        if( std::strncmp( a, "--lang=", 7 ) == 0 || std::strncmp( a, "-lang=", 6 ) == 0 )
        {
            if( argc == 2 )                       // только смена языка
            {
                std::printf( TmcRussian() ? "Язык сообщений: русский\n" : "Message language: English\n" );
                return 0;
            }
            continue;
        }
        if( IsKey( a, "h" ) || IsKey( a, "help" ) || std::strcmp( a, "-?" ) == 0 )
        {
            PrintUsage( exe );
            return 0;
        }
        if( IsKey( a, "about" ) || IsKey( a, "version" ) || std::strcmp( a, "-v" ) == 0 )
        {
            TmcPrintAbout();
            return 0;
        }
        if( IsKey( a, "manual" ) )
            return TmcOpenManual();
        if( IsKey( a, "Time" ) )                  { bTime = true; continue; }
        if( IsKey( a, "DN" ) )                    { ::setenv( "TMC_AUTO_DN", "1", 1 ); continue; }
        if( IsKey( a, "q" ) || IsKey( a, "quiet" ) ) { g_nTmcTraceLevel = 0; continue; }
        if( IsKey( a, "verbose" ) )               { g_nTmcTraceLevel = 2; continue; }
        // Видеокарту выбирает само ядро по командной строке процесса.
        if( IsGpuKey( a ) || IsKey( a, "CPU" ) )  continue;
        // Ключи пакетного режима Windows-версии: консольная версия всегда так работает.
        if( IsKey( a, "Ar" ) || IsKey( a, "Arb" ) ) continue;
        if( a[0] == '-' && a[1] != '\0' )
        {
            std::fprintf( stderr, TmcRussian() ? "%s: неизвестный ключ %s (справка: %s --help)\n"
                                               : "%s: unknown key %s (help: %s --help)\n",
                          exe, a, exe );
            return TMC_EXIT_TPL_ERROR;
        }
        if( pszTask )
        {
            std::fprintf( stderr, TmcRussian() ? "%s: задание можно указать только одно\n"
                                               : "%s: only one task file can be given\n", exe );
            return TMC_EXIT_TPL_ERROR;
        }
        pszTask = a;
    }

    // Как пакетный запуск /Arb Windows-версии: задание обязательно и должно быть.
    if( !pszTask )
    {
        PrintUsage( exe );
        ConsolePrint( "ERROR: no .tpl file in the command line" );
        SetRunExitCode( TMC_EXIT_TPL_ERROR );
        ConsoleReportResult();
        return GetRunExitCode();
    }
    if( FILE *fin = std::fopen( pszTask, "rb" ) )
    {
        std::fclose( fin );
    }
    else
    {
        ConsolePrint( "ERROR: can't open file %s", pszTask );
        SetRunExitCode( TMC_EXIT_TPL_ERROR );
        ConsoleReportResult();
        return GetRunExitCode();
    }

    const std::string full = FullPath( pszTask );
    const CString csPath( full.c_str() );
    ConsolePrint( "File: %s", full.c_str() );

    struct sigaction sa;
    std::memset( &sa, 0, sizeof( sa ) );
    sa.sa_handler = OnStopSignal;
    sigemptyset( &sa.sa_mask );
    ::sigaction( SIGINT, &sa, nullptr );
    ::sigaction( SIGTERM, &sa, nullptr );

    // Тело CPlanRT_HDoc::RunAll(): замер всего расчёта, исключения — код 2.
    static CTmcRTHRectNode cRectNode;   // большой объект — не на стеке
    g_pNode = &cRectNode;
    cRectNode.GetTiming().BeginRunAll();
    try
    {
        RunAll1( cRectNode, csPath );
    }
    catch( std::bad_alloc & )
    {
        ConsolePrint( "ERROR in calculation: not enough memory" );
        PutTrace( "Error: not enough memory" );
        SetRunExitCode( TMC_EXIT_CALC_ERROR );
    }
    catch( ... )
    {
        ConsolePrint( "ERROR in calculation: unknown exception" );
        PutTrace( "Error: exception in calculation" );
        SetRunExitCode( TMC_EXIT_CALC_ERROR );
    };
    cRectNode.GetTiming().EndRunAll();
    if( bTime )
    {
        const CString csReport = cRectNode.GetTiming().DefaultReportName();
        if( cRectNode.GetTiming().SaveReport( csReport ) )
            ConsolePrint( "Time report: %s", (LPCTSTR)csReport );
        else
            ConsolePrint( "WARNING: can't write the time report %s", (LPCTSTR)csReport );
    }
    g_pNode = nullptr;

    ConsoleReportResult();
    return GetRunExitCode();
}
