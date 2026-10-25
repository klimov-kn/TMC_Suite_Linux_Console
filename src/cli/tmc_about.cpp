// tmc_about.cpp — «О программе», язык сообщений и руководство пользователя
// консольных ядер.
//
// Данные — те же, что в окне «О программе» Linux-версии 5.0 с интерфейсом
// (TMC_Suite_Qt/src/gui_common/aboutbox.cpp): версия пакета, год, автор, его
// страницы, сайт, руководство на сайте и имя файла руководства в поставке.
// Оформление в терминале повторяет окно: рамка, акцентный цвет проекта
// (#34e3c4), «плашки» версии и системы, ссылки, по которым можно щёлкнуть
// (в терминалах, которые это умеют). Если вывод не в терминал или задана
// переменная NO_COLOR — без цвета.
//
// К расчёту отношения не имеет.

#include "tmc_about.h"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <limits.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef TMC_KERNEL_MODE
#define TMC_KERNEL_MODE "H"
#endif
#ifndef TMC_KERNEL_EXE
#define TMC_KERNEL_EXE "tmc_rth"
#endif
#ifndef TMC_VERSION_SHORT
#define TMC_VERSION_SHORT "5.0"
#endif

namespace {

const char kYear[]       = "2026";
const char kSiteUrl[]    = "https://www.tamic.ru";
const char kSiteText[]   = "www.tamic.ru";
const char kOnlineDoc[]  = "https://www.tamic.ru/docs/user-manual/index.html";
const char kManualFile[] = "TMC_Suite_User_Manual.pdf";

struct ProfileLink
{
    const char *name;
    const char *url;
};

const ProfileLink kProfiles[] = {
    { "GitHub",       "https://github.com/klimov-kn" },
    { "LinkedIn",     "https://www.linkedin.com/in/konstantinklimov" },
    { "ResearchGate", "https://www.researchgate.net/profile/Konstantin-Klimov" },
    { "eLibrary",     "https://www.elibrary.ru/author_items.asp?authorid=121878" },
};

// Название и назначение — из окна «О программе» (по авторской документации).
struct KernelInfo
{
    const char *name;
    const char *descriptionRu;
    const char *descriptionEn;
};

const KernelInfo kKernelH = {
    "PlanarRT_H",
    "Счётное ядро для электродинамического анализа H-плоскостных планарных "
    "волноводных СВЧ устройств на основе метода Rt сеток.",
    "Computational kernel for electrodynamic analysis of H-plane planar "
    "waveguide microwave devices by the Rt-grid method."
};

const KernelInfo kKernelX = {
    "PlanarRT_X",
    "Счётное ядро, реализующее 4-параметрический алгоритм анализа рассеяния "
    "электромагнитных волн Е поляризации от неоднородной подмагниченной плазмы.",
    "Computational kernel implementing a four-parameter algorithm for scattering "
    "analysis of E-polarized electromagnetic waves by inhomogeneous magnetized plasma."
};

const KernelInfo &Kernel()
{
    return ( std::strcmp( TMC_KERNEL_MODE, "X" ) == 0 ) ? kKernelX : kKernelH;
}

const char *Arch()
{
#if defined(__aarch64__)
    return "ARM64";
#elif defined(__x86_64__)
    return "x64";
#else
    return "x86";
#endif
}

bool g_bRussian = false;

const char *T( const char *ru, const char *en ) { return g_bRussian ? ru : en; }

bool IsFile( const std::string &path )
{
    struct stat st;
    return ::stat( path.c_str(), &st ) == 0 && S_ISREG( st.st_mode );
}

// Программа есть в PATH и её можно запустить.
bool HaveProgram( const char *name )
{
    const char *path = std::getenv( "PATH" );
    if( !path ) return false;
    const std::string all( path );
    size_t i = 0;
    while( i <= all.size() )
    {
        size_t e = all.find( ':', i );
        if( e == std::string::npos ) e = all.size();
        std::string dir = all.substr( i, e - i );
        if( dir.empty() ) dir = ".";
        const std::string full = dir + "/" + name;
        if( ::access( full.c_str(), X_OK ) == 0 && IsFile( full ) ) return true;
        i = e + 1;
    }
    return false;
}

// Аргумент для /bin/sh в одинарных кавычках.
std::string ShellQuote( const std::string &s )
{
    std::string r = "'";
    for( char c : s )
    {
        if( c == '\'' ) r += "'\\''";
        else r += c;
    }
    r += "'";
    return r;
}

// =============================================================================
// Язык: общий выбор с версией с интерфейсом
// =============================================================================

// ~/.config/TMC_Suite_Qt/Common.ini — тот же файл, что у Linux-версии с
// интерфейсом (QSettings, раздел [UI], ключ Language = ru | en | system).
std::string SettingsDir()
{
    const char *xdg = std::getenv( "XDG_CONFIG_HOME" );
    std::string base;
    if( xdg && *xdg ) base = xdg;
    else
    {
        const char *home = std::getenv( "HOME" );
        if( !home || !*home ) return std::string();
        base = std::string( home ) + "/.config";
    }
    return base + "/TMC_Suite_Qt";
}

std::string SettingsFile()
{
    const std::string d = SettingsDir();
    return d.empty() ? d : d + "/Common.ini";
}

std::vector<std::string> ReadLines( const std::string &file )
{
    std::vector<std::string> v;
    FILE *f = std::fopen( file.c_str(), "rb" );
    if( !f ) return v;
    std::string cur;
    int c;
    while( ( c = std::fgetc( f ) ) != EOF )
    {
        if( c == '\n' ) { v.push_back( cur ); cur.clear(); }
        else if( c != '\r' ) cur += char( c );
    }
    if( !cur.empty() ) v.push_back( cur );
    std::fclose( f );
    return v;
}

std::string Trim( const std::string &s )
{
    size_t a = s.find_first_not_of( " \t" ), b = s.find_last_not_of( " \t" );
    return ( a == std::string::npos ) ? std::string() : s.substr( a, b - a + 1 );
}

// Значение UI/Language или пустая строка, если выбора ещё не было.
std::string ReadLanguageSetting()
{
    bool inUi = false;
    for( const std::string &raw : ReadLines( SettingsFile() ) )
    {
        const std::string l = Trim( raw );
        if( !l.empty() && l[0] == '[' ) { inUi = ( l == "[UI]" ); continue; }
        if( !inUi ) continue;
        const size_t eq = l.find( '=' );
        if( eq != std::string::npos && Trim( l.substr( 0, eq ) ) == "Language" )
            return Trim( l.substr( eq + 1 ) );
    }
    return std::string();
}

// Записать UI/Language, сохранив остальные настройки файла.
void WriteLanguageSetting( const std::string &value )
{
    const std::string dir = SettingsDir();
    if( dir.empty() ) return;
    {   // mkdir -p для ~/.config/TMC_Suite_Qt
        std::string p;
        for( size_t i = 0; i <= dir.size(); i++ )
        {
            if( i == dir.size() || ( dir[i] == '/' && i > 0 ) ) ::mkdir( p.c_str(), 0755 );
            if( i < dir.size() ) p += dir[i];
        }
    }
    std::vector<std::string> v = ReadLines( SettingsFile() );
    const std::string kv = "Language=" + value;
    int ui = -1;
    bool done = false;
    for( size_t i = 0; i < v.size() && !done; i++ )
    {
        const std::string l = Trim( v[i] );
        if( !l.empty() && l[0] == '[' )
        {
            if( ui >= 0 ) { v.insert( v.begin() + i, kv ); done = true; break; }
            if( l == "[UI]" ) ui = int( i );
            continue;
        }
        const size_t eq = l.find( '=' );
        if( ui >= 0 && eq != std::string::npos && Trim( l.substr( 0, eq ) ) == "Language" )
        {
            v[i] = kv;
            done = true;
        }
    }
    if( !done )
    {
        if( ui < 0 ) { if( !v.empty() && !v.back().empty() ) v.push_back( "" ); v.push_back( "[UI]" ); }
        v.push_back( kv );
    }
    FILE *f = std::fopen( SettingsFile().c_str(), "wb" );
    if( !f ) return;
    for( const std::string &l : v ) std::fprintf( f, "%s\n", l.c_str() );
    std::fclose( f );
}

bool SystemIsRussian()
{
    const char *vars[] = { "LC_ALL", "LC_MESSAGES", "LANG" };
    for( const char *v : vars )
    {
        const char *s = std::getenv( v );
        if( s && *s ) return std::strncmp( s, "ru", 2 ) == 0;
    }
    return false;
}

// Один раз спросить язык (по-русски и по-английски сразу).
// Enter без ответа — язык системы. Возвращает "ru" или "en".
std::string AskLanguage()
{
    const bool sysRu = SystemIsRussian();
    std::printf( "\n  Язык / Language\n"
                 "    1  Русский\n"
                 "    2  English\n"
                 "  Выберите / Choose [%c]: ", sysRu ? '1' : '2' );
    std::fflush( stdout );
    char buf[64];
    std::string ans;
    if( std::fgets( buf, sizeof( buf ), stdin ) ) ans = Trim( std::string( buf, std::strcspn( buf, "\r\n" ) ) );
    std::string lang;
    if( ans == "1" || ans == "ru" || ans == "RU" || ans == "р" ) lang = "ru";
    else if( ans == "2" || ans == "en" || ans == "EN" || ans == "e" ) lang = "en";
    else lang = sysRu ? "ru" : "en";
    std::printf( "%s\n\n", lang == "ru"
        ? "  Язык: русский. Сменить: " TMC_KERNEL_EXE " --lang=en"
        : "  Language: English. To change: " TMC_KERNEL_EXE " --lang=ru" );
    return lang;
}

// =============================================================================
// Вывод в терминал: рамка, цвет, ссылки
// =============================================================================

enum Style { S_TEXT, S_TITLE, S_ACCENT, S_DIM, S_MUTE, S_CHIP_MAIN, S_CHIP, S_LINK, S_CAPTION };

struct Seg
{
    std::string text;
    Style       style;
    std::string url;     // ссылка (OSC 8), если терминал её поддерживает
};
typedef std::vector<Seg> Line;

bool g_bColor = false;
bool g_bTrue  = false;   // 24-битный цвет

const char *Ansi( Style s )
{
    if( g_bTrue )
        switch( s )
        {
            case S_TITLE:     return "\033[1;38;2;231;238;247m";
            case S_ACCENT:    return "\033[1;38;2;52;227;196m";
            case S_DIM:       return "\033[38;2;159;176;200m";
            case S_MUTE:      return "\033[38;2;108;125;151m";
            case S_CHIP_MAIN: return "\033[1;38;2;10;14;22;48;2;52;227;196m";
            case S_CHIP:      return "\033[38;2;231;238;247;48;2;36;48;73m";
            case S_LINK:      return "\033[4;38;2;52;227;196m";
            case S_CAPTION:   return "\033[1;38;2;176;124;255m";
            default:          return "\033[38;2;231;238;247m";
        }
    switch( s )
    {
        case S_TITLE:     return "\033[1m";
        case S_ACCENT:    return "\033[1;36m";
        case S_DIM:       return "\033[37m";
        case S_MUTE:      return "\033[90m";
        case S_CHIP_MAIN: return "\033[1;30;46m";
        case S_CHIP:      return "\033[37;100m";
        case S_LINK:      return "\033[4;36m";
        case S_CAPTION:   return "\033[1;35m";
        default:          return "\033[0m";
    }
}

// Ширина в колонках терминала: число символов UTF-8 (широких знаков в тексте нет).
size_t Width( const std::string &s )
{
    size_t n = 0;
    for( unsigned char c : s ) if( ( c & 0xC0 ) != 0x80 ) n++;
    return n;
}

size_t Width( const Line &l )
{
    size_t n = 0;
    for( const Seg &s : l ) n += Width( s.text );
    return n;
}

// Первые n символов UTF-8 строки (по границе символа).
size_t BytesFor( const std::string &s, size_t chars )
{
    size_t i = 0, n = 0;
    while( i < s.size() )
    {
        if( ( (unsigned char)s[i] & 0xC0 ) != 0x80 )
        {
            if( n == chars ) break;
            n++;
        }
        i++;
    }
    return i;
}

class Box
{
public:
    explicit Box( size_t inner ) : W( inner ) {}

    void Add( const Line &l ) { lines.push_back( l ); }
    void Sep() { lines.push_back( Line( 1, Seg{ "\x01", S_TEXT, "" } ) ); }
    void Blank() { lines.push_back( Line() ); }

    // Текст с переносом по словам, отступ indent.
    void Text( const std::string &s, Style st, size_t indent )
    {
        const size_t room = W - indent;
        std::string cur;
        size_t pos = 0;
        while( pos < s.size() )
        {
            size_t e = s.find( ' ', pos );
            if( e == std::string::npos ) e = s.size();
            const std::string word = s.substr( pos, e - pos );
            const size_t need = Width( cur ) + ( cur.empty() ? 0 : 1 ) + Width( word );
            if( !cur.empty() && need > room )
            {
                Add( Line{ Seg{ std::string( indent, ' ' ), S_TEXT, "" }, Seg{ cur, st, "" } } );
                cur.clear();
            }
            if( !cur.empty() ) cur += ' ';
            cur += word;
            pos = e + 1;
        }
        if( !cur.empty() )
            Add( Line{ Seg{ std::string( indent, ' ' ), S_TEXT, "" }, Seg{ cur, st, "" } } );
    }

    // «Подпись   значение»; длинное значение (путь, адрес) переносится без
    // разрывов слов — по символам, с тем же отступом.
    void KV( const std::string &label, Style lst, const std::string &value, Style vst,
             const std::string &url = std::string(), size_t col = 15 )
    {
        std::string head = label;
        if( Width( head ) < col ) head += std::string( col - Width( head ), ' ' );
        const size_t room = W - col;
        std::string rest = value;
        bool first = true;
        do
        {
            size_t b = BytesFor( rest, room );
            if( b < rest.size() )
            {
                // Перенос — после «/» (каталог пути, часть адреса), если он не
                // слишком близко к началу строки; иначе — по символам.
                const size_t k = rest.rfind( '/', b - 1 );
                if( k != std::string::npos && Width( rest.substr( 0, k + 1 ) ) > room / 3 )
                    b = k + 1;
            }
            const std::string part = rest.substr( 0, b );
            rest = rest.substr( b );
            Add( Line{ Seg{ first ? head : std::string( col, ' ' ), first ? lst : S_TEXT, "" },
                       Seg{ part, vst, url } } );
            first = false;
        } while( !rest.empty() );
    }

    void Print( const char *title ) const
    {
        const Style fr = S_MUTE;
        // Верх: ╭─ PlanarRT_H ───╮
        std::string top = "╭─";
        Out( top, fr );
        Out( " ", S_TEXT );
        Out( title, S_ACCENT );
        Out( " ", S_TEXT );
        std::string rule;
        for( size_t i = Width( title ) + 4; i < W + 2; i++ ) rule += "─";
        Out( rule + "─╮", fr );
        std::fputc( '\n', stdout );

        for( const Line &l : lines )
        {
            if( l.size() == 1 && l[0].text == "\x01" )
            {
                std::string r = "├";
                for( size_t i = 0; i < W + 2; i++ ) r += "─";
                Out( r + "┤", fr );
                std::fputc( '\n', stdout );
                continue;
            }
            Out( "│ ", fr );
            for( const Seg &s : l ) Out( s.text, s.style, s.url );
            const size_t w = Width( l );
            if( w < W ) std::fputs( std::string( W - w, ' ' ).c_str(), stdout );
            Out( " │", fr );
            std::fputc( '\n', stdout );
        }

        std::string bot = "╰";
        for( size_t i = 0; i < W + 2; i++ ) bot += "─";
        Out( bot + "╯", fr );
        std::fputc( '\n', stdout );
    }

private:
    static void Out( const std::string &text, Style st, const std::string &url = std::string() )
    {
        if( !g_bColor || text.find_first_not_of( ' ' ) == std::string::npos )
        {
            std::fputs( text.c_str(), stdout );
            return;
        }
        if( !url.empty() ) std::printf( "\033]8;;%s\033\\", url.c_str() );
        std::printf( "%s%s\033[0m", Ansi( st ), text.c_str() );
        if( !url.empty() ) std::printf( "\033]8;;\033\\" );
    }

    size_t W;
    std::vector<Line> lines;
};

size_t TerminalColumns()
{
    struct winsize ws;
    if( ::ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws ) == 0 && ws.ws_col > 0 ) return ws.ws_col;
    const char *c = std::getenv( "COLUMNS" );
    if( c && std::atoi( c ) > 0 ) return size_t( std::atoi( c ) );
    return 80;
}

std::string Chip( const std::string &s ) { return g_bColor ? " " + s + " " : "[" + s + "]"; }

} // namespace

// =============================================================================

bool TmcInitLanguage( const char *pszForced, bool bMayAsk )
{
    std::string lang;
    if( pszForced )
    {
        lang = pszForced;
        for( char &c : lang ) c = char( std::tolower( (unsigned char)c ) );
        if( lang == "ру" ) lang = "ru";
        if( lang != "ru" && lang != "en" && lang != "system" ) return false;
        WriteLanguageSetting( lang );           // как «Настройки → Language / Язык»
    }
    else
    {
        lang = ReadLanguageSetting();
        if( lang.empty() && bMayAsk && ::isatty( STDIN_FILENO ) && ::isatty( STDOUT_FILENO ) )
        {
            lang = AskLanguage();
            WriteLanguageSetting( lang );
        }
    }
    if( lang == "ru" )      g_bRussian = true;
    else if( lang == "en" ) g_bRussian = false;
    else                    g_bRussian = SystemIsRussian();
    return true;
}

bool TmcRussian( void ) { return g_bRussian; }

std::string TmcExeDir( void )
{
    char buf[PATH_MAX];
    const ssize_t n = ::readlink( "/proc/self/exe", buf, sizeof( buf ) - 1 );
    if( n <= 0 ) return std::string( "." );
    buf[n] = '\0';
    const std::string dir( buf );
    const size_t k = dir.rfind( '/' );
    return ( k == std::string::npos || k == 0 ) ? std::string( "/" ) : dir.substr( 0, k );
}

std::string TmcFindManual( void )
{
    const std::string exe = TmcExeDir();
    const std::string places[] = {
        exe,
        exe + "/../share/tmc-suite",
        exe + "/../share/doc/tmc-suite",
        exe + "/../doc",
        exe + "/..",
    };
    for( const std::string &dir : places )
    {
        const std::string f = dir + "/" + kManualFile;
        if( IsFile( f ) )
        {
            char real[PATH_MAX];
            return ::realpath( f.c_str(), real ) ? std::string( real ) : f;
        }
    }
    return std::string();
}

void TmcPrintAbout( void )
{
    const char *noColor = std::getenv( "NO_COLOR" );
    const char *term = std::getenv( "TERM" );
    g_bColor = ::isatty( STDOUT_FILENO ) && !( noColor && *noColor ) &&
               !( term && std::strcmp( term, "dumb" ) == 0 );
    const char *ct = std::getenv( "COLORTERM" );
    g_bTrue = g_bColor && ct && ( std::strstr( ct, "truecolor" ) || std::strstr( ct, "24bit" ) );

    size_t cols = TerminalColumns();
    if( cols > 80 ) cols = 80;
    if( cols < 56 ) cols = 56;
    Box box( cols - 4 );

    const KernelInfo &k = Kernel();
    const std::string manual = TmcFindManual();

    // Название и назначение.
    box.Blank();
    box.Add( Line{ Seg{ "  ", S_TEXT, "" }, Seg{ k.name, S_TITLE, "" },
                   Seg{ "  ·  ", S_MUTE, "" }, Seg{ TMC_KERNEL_EXE, S_ACCENT, "" },
                   Seg{ T( "  ·  консольная версия", "  ·  console version" ), S_DIM, "" } } );
    box.Text( T( k.descriptionRu, k.descriptionEn ), S_DIM, 2 );
    box.Blank();
    box.Add( Line{ Seg{ "  ", S_TEXT, "" },
                   Seg{ Chip( std::string( T( "Версия ", "Version " ) ) + TMC_VERSION_SHORT ), S_CHIP_MAIN, "" },
                   Seg{ "  ", S_TEXT, "" }, Seg{ Chip( kYear ), S_CHIP, "" },
                   Seg{ "  ", S_TEXT, "" }, Seg{ Chip( std::string( "Linux · " ) + Arch() ), S_CHIP, "" } } );
    box.Blank();
    box.Sep();

    // Автор и его страницы.
    box.Blank();
    box.Add( Line{ Seg{ "  ", S_TEXT, "" }, Seg{ T( "АВТОР", "AUTHOR" ), S_CAPTION, "" } } );
    box.Add( Line{ Seg{ "  ", S_TEXT, "" }, Seg{ T( "К. Н. Климов", "K. N. Klimov" ), S_TITLE, "" },
                   Seg{ std::string( "  © " ) + kYear, S_DIM, "" } } );
    box.Blank();
    for( const ProfileLink &l : kProfiles )
        box.KV( std::string( "  " ) + l.name, S_DIM, l.url, S_LINK, l.url );
    box.Blank();
    box.Sep();

    // Сайт и руководство пользователя.
    box.Blank();
    box.KV( T( "  Сайт", "  Website" ), S_DIM, kSiteText, S_LINK, kSiteUrl );
    if( !manual.empty() )
    {
        box.KV( T( "  Руководство", "  Manual" ), S_DIM, manual, S_LINK, "file://" + manual );
        box.KV( "", S_DIM, std::string( T( "открыть: ", "open: " ) ) + TMC_KERNEL_EXE + " --manual", S_ACCENT );
        box.KV( "", S_DIM, std::string( T( "на сайте: ", "online: " ) ) + kOnlineDoc, S_MUTE, kOnlineDoc );
    }
    else
    {
        box.KV( T( "  Руководство", "  Manual" ), S_DIM, kOnlineDoc, S_LINK, kOnlineDoc );
        box.KV( "", S_DIM, std::string( T( "файл ", "file " ) ) + kManualFile +
                           T( " рядом с программой не найден", " was not found next to the program" ), S_MUTE );
    }
    box.Blank();
    box.Sep();

    // Низ: пакет.
    box.Add( Line{ Seg{ std::string( "TMC Suite " ) + TMC_VERSION_SHORT, S_TEXT, "" },
                   Seg{ T( " · консольная версия, окна не нужны · g++ ",
                           " · console version, no windows needed · g++ " ), S_MUTE, "" },
                   Seg{ __VERSION__, S_MUTE, "" } } );

    box.Print( T( "О программе", "About" ) );
    std::fflush( stdout );
}

int TmcOpenManual( void )
{
    const std::string manual = TmcFindManual();

    if( manual.empty() )
    {
        std::fprintf( stderr, T( "Файл %s рядом с программой не найден.\n"
                                 "Руководство на сайте: %s\n",
                                 "%s was not found next to the program.\n"
                                 "Online manual: %s\n" ),
                      kManualFile, kOnlineDoc );
        return 1;
    }

    // Графический сеанс: PDF открывает программа просмотра, назначенная в системе.
    const char *disp = std::getenv( "DISPLAY" );
    const char *wl = std::getenv( "WAYLAND_DISPLAY" );
    const bool gui = ( disp && *disp ) || ( wl && *wl );
    if( gui && HaveProgram( "xdg-open" ) )
    {
        std::printf( T( "Открываю руководство: %s\n", "Opening the manual: %s\n" ), manual.c_str() );
        std::fflush( stdout );
        const pid_t pid = ::fork();
        if( pid == 0 )
        {
            ::setsid();                       // программа просмотра живёт сама по себе
            std::freopen( "/dev/null", "r", stdin );
            std::freopen( "/dev/null", "w", stdout );
            std::freopen( "/dev/null", "w", stderr );
            ::execlp( "xdg-open", "xdg-open", manual.c_str(), (char *)nullptr );
            ::_exit( 127 );
        }
        return ( pid > 0 ) ? 0 : 1;
    }

    // Терминал без графики: руководство текстом (пакет poppler-utils).
    if( HaveProgram( "pdftotext" ) )
    {
        std::string cmd = "pdftotext -layout -enc UTF-8 " + ShellQuote( manual ) + " -";
        if( ::isatty( STDOUT_FILENO ) )
        {
            const char *pager = std::getenv( "PAGER" );
            if( pager && *pager ) cmd += std::string( " | " ) + pager;
            else if( HaveProgram( "less" ) ) cmd += " | less";
            else if( HaveProgram( "more" ) ) cmd += " | more";
        }
        std::fflush( stdout );
        const int rc = std::system( cmd.c_str() );
        return ( rc == 0 ) ? 0 : 1;
    }

    std::printf( T( "Руководство пользователя: %s\n"
                    "В этом сеансе нет графического окружения, поэтому PDF открыть нечем.\n"
                    "Прочитать руководство прямо в терминале можно текстом — установите\n"
                    "пакет poppler-utils (sudo apt install poppler-utils) и повторите:\n"
                    "  %s --manual\n"
                    "Руководство на сайте: %s\n",
                    "User manual: %s\n"
                    "This session has no graphical desktop, so there is nothing to open a PDF with.\n"
                    "To read the manual in the terminal as text, install poppler-utils\n"
                    "(sudo apt install poppler-utils) and run again:\n"
                    "  %s --manual\n"
                    "Online manual: %s\n" ),
                 manual.c_str(), TMC_KERNEL_EXE, kOnlineDoc );
    return 0;
}
