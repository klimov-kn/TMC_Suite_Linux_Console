/*
** Compat stub: <conio.h> (DOS/Windows console I/O header).
**
** На Linux/glibc такого заголовка нет. В портируемых файлах
** (Inter/INTE_IN1.CPP, Inter/INTE_OPE.CPP, Inter/INTE_VAL.CPP) он
** подключался, но ни одна функция из conio.h фактически не вызывается -
** единственные упоминания (getch()) закомментированы в исходниках.
** Поэтому пустая заглушка достаточна для совместимости сборки под g++;
** на математику и поведение программы это не влияет.
*/
#ifndef TMC_COMPAT_CONIO_H
#define TMC_COMPAT_CONIO_H
#endif /* TMC_COMPAT_CONIO_H */
