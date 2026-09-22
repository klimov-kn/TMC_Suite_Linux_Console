// tmc_textmode.cpp — репликация текстового режима CRT Windows для Linux.
//
// ЗАЧЕМ. Исходные файлы TMC (.TPL и текстовые результаты) имеют CRLF-окончания
// (Windows). На Windows fopen(...,"r"/"w") открывает поток в ТЕКСТОВОМ режиме:
// при чтении CRLF -> LF (символ '\r' исчезает), при записи LF -> CRLF. На Linux
// текстовый режим == бинарный: '\r' НЕ срезается и НЕ добавляется. Из-за этого
// '\r' от CRLF доживал до парсера выражений (например, "(2.00)\r"), ломая разбор
// ("abnormal function"), а вывод отличался от Windows на байты окончаний строк.
//
// РЕШЕНИЕ. Перехватываем fopen компоновочным флагом -Wl,--wrap=fopen и для
// ТЕКСТОВЫХ режимов (без 'b') возвращаем поток fopencookie, который:
//   * при чтении выбрасывает все '\r'  (CRLF/CR -> LF);
//   * при записи перед каждым '\n' вставляет '\r'  (LF -> CRLF).
// БИНАРНЫЕ режимы ('b': "rb"/"wb"/"r+b"/"w+b") идут в настоящий fopen без
// изменений — числовые форматы остаются байт-в-байт совместимы с Windows.
//
// Это слой совместимости (НЕ математика): меняется только трактовка окончаний
// строк на границе ввода-вывода, ровно как это делает CRT Windows.
//
// Активно только вне Windows. На Windows используется родной текстовый режим CRT.

#ifndef _WIN32

#define _GNU_SOURCE
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" FILE* __real_fopen(const char* path, const char* mode);

namespace {

struct TextCookie {
    FILE* raw;   // подлежащий поток, открытый в бинарном режиме
};

ssize_t tm_read(void* c, char* buf, size_t size)
{
    TextCookie* tc = static_cast<TextCookie*>(c);
    size_t out = 0;
    while (out < size) {
        int ch = std::fgetc(tc->raw);
        if (ch == EOF) break;
        if (ch == '\r') continue;        // текстовое чтение: срезаем CR
        buf[out++] = static_cast<char>(ch);
    }
    if (out == 0 && std::ferror(tc->raw)) return -1;
    return static_cast<ssize_t>(out);
}

ssize_t tm_write(void* c, const char* buf, size_t size)
{
    TextCookie* tc = static_cast<TextCookie*>(c);
    for (size_t i = 0; i < size; ++i) {
        if (buf[i] == '\n') {            // текстовая запись: LF -> CRLF
            if (std::fputc('\r', tc->raw) == EOF) return -1;
        }
        if (std::fputc(static_cast<unsigned char>(buf[i]), tc->raw) == EOF) return -1;
    }
    return static_cast<ssize_t>(size);
}

// Делегируем позиционирование подлежащему (бинарному) потоку. Для файлов без CR
// смещения совпадают с логическими; CRLF-файлы парсер читает строго
// последовательно (PREPR не использует fseek/ftell), поэтому несоответствие
// смещений на число '\r' значения не имеет.
int tm_seek(void* c, off64_t* offset, int whence)
{
    TextCookie* tc = static_cast<TextCookie*>(c);
    if (fseeko(tc->raw, static_cast<off_t>(*offset), whence) != 0) return -1;
    *offset = static_cast<off64_t>(ftello(tc->raw));
    return 0;
}

int tm_close(void* c)
{
    TextCookie* tc = static_cast<TextCookie*>(c);
    int r = std::fclose(tc->raw);
    std::free(tc);
    return r;
}

} // namespace

extern "C" FILE* __wrap_fopen(const char* path, const char* mode)
{
    if (mode == nullptr) return __real_fopen(path, mode);

    // Бинарный режим — без трансляции (числовые форматы байт-в-байт).
    if (std::strchr(mode, 'b')) return __real_fopen(path, mode);

    // Текстовый режим: открываем подлежащий поток в бинарном эквиваленте
    // (тот же режим + 'b'), трансляцию делает cookie.
    char rawmode[8];
    std::snprintf(rawmode, sizeof rawmode, "%sb", mode);

    FILE* raw = __real_fopen(path, rawmode);
    if (raw == nullptr) return nullptr;

    TextCookie* tc = static_cast<TextCookie*>(std::malloc(sizeof(TextCookie)));
    if (tc == nullptr) { std::fclose(raw); return nullptr; }
    tc->raw = raw;

    cookie_io_functions_t io = { tm_read, tm_write, tm_seek, tm_close };
    FILE* f = fopencookie(tc, mode, io);
    if (f == nullptr) { std::fclose(raw); std::free(tc); return nullptr; }
    return f;
}

#endif // !_WIN32
