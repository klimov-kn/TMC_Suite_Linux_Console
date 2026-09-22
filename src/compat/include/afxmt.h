// Заглушка MFC <afxmt.h> (примитивы синхронизации MFC) для сборки под Linux. ISSUES.md R3.
// Счётное ядро в CLI однопоточное (потоки запускались GUI-обвязкой, которая
// в Linux-сборку не входит); нужен лишь скелет MFC из tmc_mfc_gui.h.
#pragma once
#include "tmc_mfc_gui.h"
