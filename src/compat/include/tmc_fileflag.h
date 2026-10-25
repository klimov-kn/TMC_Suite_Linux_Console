// Признак «архивный» файла (бит ARCHIVE Windows) на Linux и macOS.
//
// Признак в пакете играет одну роль: FieldView помечает им кадр поля, который
// уже прочитал (CFldViewDoc::SetSynchronizationFlag), а счётное ядро по нему
// понимает, можно ли писать следующий (CTmcRTHRectNode::IsFieldFileRead).
//
// В Windows этот бит ставит САМА система при любой записи в файл, а снять его
// может только программа. Отсюда и всё поведение пакета: ядро записало кадр —
// бит снова взведён, и ядро не ждёт; FieldView прочитал кадр и снял бит — до
// следующей записи файл считается прочитанным.
//
// На Linux и macOS такого бита нет. Повторяем его смысл точно: когда программа
// снимает признак, запоминаем в расширенном атрибуте файла время его последнего
// изменения. Признак считается снятым, только пока файл с тех пор не менялся;
// любая новая запись — и он снова взведён, как в Windows.
//
// Если файловая система расширенных атрибутов не поддерживает (например, раздел
// Windows, подключённый в /mnt), признак хранить негде — считаем его взведённым.
// Тогда ядро не ждёт просмотрщика: иначе при включённой синхронизации оно
// простаивало бы по 25 секунд на каждом кадре.
//
// Заголовок не зависит от слоя MFC: его берут и функции Set/GetFileAttributes
// слоя (tmc_mfc.h), и FieldView 5.0, который со слоем не собирается.
#pragma once
#ifndef _WIN32

#include <cerrno>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>

#if defined(__APPLE__)
  #define TMC_FILEFLAG_ENOATTR ENOATTR
#else
  #define TMC_FILEFLAG_ENOATTR ENODATA
#endif

namespace tmc_fileflag {

enum State { Missing, Archive, Normal };

// Есть ли у файловой системы расширенные атрибуты (выясняется при первом отказе).
inline bool& xattrWorks()
{
    static bool works = true;
    return works;
}

// Время последнего изменения файла в виде строки (секунды и наносекунды).
inline std::string stamp(const char* fileName)
{
    struct stat st;
    if (::stat(fileName, &st) != 0)
        return std::string();
    char buf[64];
#if defined(__APPLE__)
    std::snprintf(buf, sizeof(buf), "%lld.%09ld",
                  (long long)st.st_mtimespec.tv_sec, (long)st.st_mtimespec.tv_nsec);
#else
    std::snprintf(buf, sizeof(buf), "%lld.%09ld",
                  (long long)st.st_mtime, (long)st.st_mtim.tv_nsec);
#endif
    return std::string(buf);
}

// archive = true — взвести признак (забыть отметку о прочтении);
// false — снять (запомнить, каким файл был в этот момент).
inline bool set(const char* fileName, bool archive)
{
    if (!fileName || !fileName[0])
        return false;

    int rc = 0;
    if (archive) {
#if defined(__APPLE__)
        rc = ::removexattr(fileName, "user.tmc_read_stamp", 0);
#else
        rc = ::removexattr(fileName, "user.tmc_read_stamp");
#endif
        if (rc != 0 && errno == TMC_FILEFLAG_ENOATTR)
            rc = 0;                      // и так не стоял
    } else {
        const std::string s = stamp(fileName);
        if (s.empty())
            return false;
#if defined(__APPLE__)
        rc = ::setxattr(fileName, "user.tmc_read_stamp", s.c_str(), s.size(), 0, 0);
#else
        rc = ::setxattr(fileName, "user.tmc_read_stamp", s.c_str(), s.size(), 0);
#endif
    }

    if (rc != 0 && errno == ENOTSUP)
        xattrWorks() = false;
    return true;
}

inline State get(const char* fileName)
{
    if (!fileName || !fileName[0])
        return Missing;

    const std::string s = stamp(fileName);
    if (s.empty())
        return Missing;                  // файла нет

    if (!xattrWorks())
        return Archive;

    char saved[64];
#if defined(__APPLE__)
    const ssize_t n = ::getxattr(fileName, "user.tmc_read_stamp", saved, sizeof(saved), 0, 0);
#else
    const ssize_t n = ::getxattr(fileName, "user.tmc_read_stamp", saved, sizeof(saved));
#endif
    if (n < 0) {
        if (errno == ENOTSUP)
            xattrWorks() = false;
        return Archive;                  // отметки нет — признак взведён
    }

    // Отметка есть: признак снят, только если файл с тех пор не менялся.
    return (std::string(saved, size_t(n)) == s) ? Normal : Archive;
}

} // namespace tmc_fileflag

#endif // !_WIN32
