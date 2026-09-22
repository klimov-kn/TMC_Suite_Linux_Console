# Сборка под Linux — навигация

Основные документы по сборке вынесены в **.docx** (по требованию заказчика):

| Документ | Что внутри |
|---|---|
| **`Инструкция_сборки_Linux_WSL2.docx`** | Пошаговая инструкция: окружение (WSL2 + Ubuntu 24.04), подготовка, сборка библиотек и ядер, запуск, зависимости. |
| **`Изменения_исходников_Windows-Linux.docx`** | Что и в какой строке менять в исходниках для сборки каждой библиотеки и ядра (таблицы: строка · было · стало · почему). |
| [port-changes.md](port-changes.md) | Рабочая запись сверки Windows↔Linux (источник данных для отчёта `.docx`). |

> ⚠️ **Где собирали:** не в Visual Studio и не на отдельном Linux-сервере, а в
> **WSL2 (Ubuntu-24.04) на хосте Windows 11**, через CMake/ninja/g++ внутри WSL.
> Тулчейн совпадает с целевым сервером заказчика (Ubuntu 24.04.4, g++ 13.3, glibc 2.39).
> Подробности окружения — [target-environment.md](target-environment.md).

## Кратко (для тех, кто уже в WSL/Ubuntu)

```bash
# зависимости (один раз)
sudo apt update && sudo apt install -y build-essential g++ cmake ninja-build

# конфигурация + сборка библиотек и обоих ядер
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DTMC_BUILD_KERNELS=ON
cmake --build build -j

# артефакты: dist/linux-x86-64/{lib,bin}
```

Полные шаги, нюансы WSL (гипервизор, сеть `mirrored`, CRLF) и первичные правки
исходников — в `Инструкция_сборки_Linux_WSL2.docx`.
