/// @file tmc_about.h
/// @brief «О программе», язык сообщений и руководство пользователя консольных ядер.
///
/// Те же данные, что в окне «О программе» версий с интерфейсом, но в терминале (рамка, цвет,
/// ссылки); руководство (PDF рядом с ядрами) открывается программой просмотра системы или
/// выводится текстом. Язык — тот же выбор, что в версии с интерфейсом:
/// `~/.config/TMC_Suite_Qt/Common.ini`, ключ `UI/Language`. К расчёту отношения не имеет.
/// @see https://www.tamic.ru/docs/platforms/linux-cli.html

#pragma once

#include <string>

/// @brief Выбирает язык сообщений.
/// @param[in] pszForced значение ключа `--lang` (ru, en, system) или `nullptr`
/// @param[in] bMayAsk можно спросить пользователя (справочная команда из терминала): спрашивается
///                    один раз, ответ запоминается
/// @return false — значение `--lang` не распознано.
bool TmcInitLanguage( const char *pszForced, bool bMayAsk );

/// @brief Сообщает язык сообщений.
/// @return true — русский.
bool TmcRussian( void );

/// @brief Каталог, где лежит сама программа (по `/proc/self/exe`).
/// @return Путь без завершающего «/».
std::string TmcExeDir( void );

/// @brief Ищет руководство на языке сообщений (`TMC_Suite_User_Manual.pdf`, `TMC_Suite_User_Manual_EN.pdf`) рядом с программой и в
///        `../share/tmc-suite`, `../share/doc/tmc-suite`, `../doc`, `..`.
/// @return Полный путь или пустая строка.
std::string TmcFindManual( void );

/// @brief Печатает «О программе» в stdout.
void TmcPrintAbout( void );

/// @brief Открывает руководство: в графическом сеансе — программой просмотра PDF (`xdg-open`),
///        в терминале — текстом (`pdftotext`), иначе называет файл и адрес на сайте.
/// @return Код завершения процесса.
int TmcOpenManual( void );
