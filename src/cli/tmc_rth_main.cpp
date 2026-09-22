// tmc_rth_main.cpp — консольная точка входа ядра PlanarRT_H (tmc_rth).
//
// Режим работы: файл -> расчёт -> файл (см. правила проекта). Имя ВХОДНОГО файла
// (.001/TPL) передаётся аргументом argv[1]. Имя ВЫХОДНОГО файла НЕ передаётся:
// оно берётся из самого входного файла (секция OUTPUT, cIndan.GetOutput()->
// csFileName), как и в Windows-версии.
//
// Логика повторяет тело CPlanRT_HDoc::RunAll() (PlanRT_HDoc.cpp:303-363) БЕЗ
// MFC-обёртки Doc/View: создаём CTmcRTHRectNode напрямую и прогоняем все шаги
// 1..GetNStepMax(). Цикл (GetNStepMax -> ReadData/RunStep с проверкой IsError)
// идентичен оригиналу; убраны лишь GUI-флаги отмены пользователем (bIsReadData),
// которые без окон не нужны. Математика не затронута.

#include <afxwin.h>            // MFC-шим (CString и пр.) — только Linux
#include <TmcRTHRectNode.h>    // CTmcRTHRectNode — вычислительный класс ядра

#include <cstdio>
#include <cstring>
#include <clocale>

// Название, версия, система и где искать документацию. Версия — та же, что
// показывает окно «О программе» в версиях с интерфейсом (ресурс IDD_ABOUTBOX).
static void PrintAbout( void )
{
    std::fprintf( stderr,
        "tmc_rth - counting kernel PlanarRT_H (H-mode), TMC Suite 4.1\n"
        "built for Linux (x64), console version - no windows required\n"
        "\n"
        "  site:   https://www.tamic.ru\n"
        "  manual: TMC_Suite_User_Manual.pdf (next to this program)\n"
        "          https://www.tamic.ru/docs/user-manual/index.html\n" );
}


int main( int argc, char **argv )
{
    // Числа в файлах TMC пишутся и читаются только с ТОЧКОЙ — так их создаёт
    // Windows-версия, и так их ждут все sscanf("%lg") в перенесённом коде.
    // Сейчас программа локаль не переключает и остаётся в «C», но стоит кому-то
    // добавить setlocale(LC_ALL, "") — числовой разделитель пришёл бы из
    // окружения (в русской локали это запятая), и выходные файлы перестали бы
    // совпадать с Windows байт в байт. Закрепляем числовую локаль явно.
    std::setlocale( LC_NUMERIC, "C" );

    // Ключ справки: то же, что показывает окно «О программе» в версиях с
    // интерфейсом — что это за программа, какая версия, для какой системы
    // собрана, где документация.
    if( argc >= 2 && ( strcmp( argv[1], "--version" ) == 0 ||
                       strcmp( argv[1], "-v" ) == 0 ) )
    {
        PrintAbout();
        return 0;
    }

    if( argc < 2 )
    {
        PrintAbout();
        std::fprintf( stderr,
            "\nusage: %s <input-file>\n"
            "  <input-file>  path to TMC input task (.001 / TPL).\n"
            "                give the FULL path: the task directory is taken\n"
            "                from it for #include files and .eps distributions.\n"
            "  Output file name is read from the input task (OUTPUT section).\n"
            "\nexit codes: 0 - done, 1 - error, 2 - no input file given\n",
            argv[0] ? argv[0] : "tmc_rth" );
        return 2;
    }

    const char *inputPath = argv[1];

    // CLI-гард (не математика): на сервере отсутствующий вход должен падать явно.
    // В оригинале RunAll() при GetNStepMax()==0 просто молча ничего не считал;
    // для консольного запуска это плохой контракт, поэтому проверяем доступность
    // входного файла заранее. На сам алгоритм/формат это не влияет.
    if( FILE *fin = std::fopen( inputPath, "rb" ) )
    {
        std::fclose( fin );
    }
    else
    {
        std::fprintf( stderr, "ERROR: cannot open input file: %s\n", inputPath );
        return 1;
    }

    CTmcRTHRectNode cRectNode;

    // Полный прогон всех шагов задачи (как RunAll(): i = 1 .. GetNStepMax()).
    int nMax = cRectNode.GetNStepMax( inputPath );

    for( int nStep = 1; nStep <= nMax; nStep++ )
    {
        cRectNode.ReadData( inputPath, nStep );
        if( cRectNode.IsError() )
        {
            std::fprintf( stderr, "ERROR (read, step %d): %s\n",
                nStep, (const char*)cRectNode.GetErrorMessage() );
            return 1;
        }

        cRectNode.RunStep();
        if( cRectNode.IsError() )
        {
            std::fprintf( stderr, "ERROR (run, step %d): %s\n",
                nStep, (const char*)cRectNode.GetErrorMessage() );
            return 1;
        }
    }

    return 0;
}
