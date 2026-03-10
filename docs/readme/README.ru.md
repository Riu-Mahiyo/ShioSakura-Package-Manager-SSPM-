<div align="center">

# 🌸 SSPM — ShioSakura Менеджер Пакетов

**Универсальный оркестратор пакетов · Кросс-дистрибуционный оркестратор управления пакетами**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#supported-platforms)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** — это универсальный оркестратор пакетных менеджеров, который работает поверх собственных пакетных менеджеров вашей системы.  
> Одна команда. Любое дистрибутив. Любое бэкенд.

</div>

---

## 📖 Содержание

- [Что такое SSPM?](#what-is-sspm)
- [Архитектура](#architecture)
- [Поддерживаемые платформы](#supported-platforms)
- [Установка](#installation)
- [Быстрый старт](#quick-start)
- [Справочник CLI](#cli-reference)
- [Система бэкендов](#backend-system)
- [Формат пакетов SPK](#spk-package-format)
- [Система репозиториев](#repo-system)
- [Система профилей](#profile-system)
- [Система зеркал](#mirror-system)
- [Безопасность](#security)
- [Doctor & Диагностика](#doctor--diagnostics)
- [Режим API & GUI](#api-mode--gui)
- [Система плагинов](#plugin-system)
- [Конфигурация](#configuration)
- [Структура проекта](#project-structure)
- [Вклад в проект](#contributing)
- [Лицензия](#license)

---

## 🌸 Что такое SSPM?

SSPM (**ShioSakura Package Manager**) — это **универсальный оркестратор пакетов** — он не заменяет пакетный менеджер вашей системы, а *координирует* их.

```
Вы  →  sspm install nginx
              ↓
     Слой абстракции бэкенда
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM определяет вашу среду, выбирает лучший доступный бэкенд, разрешает зависимости, обрабатывает транзакции и предоставляет единый опыт на **Linux, macOS, BSD и Windows**.

---

## 🏗️ Архитектура

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  apt / pacman / brew стиль — на ваш выбор
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Решатель   │  Разрешение транзитивных зависимостей
 │  зависимостей│  Решение ограничений версий (>=, <=, !=, …)
 │             │  Обнаружение конфликтов / сбоев
 │             │  Топологическая сортировка → порядок установки
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Решатель   │  Выбор лучшего бэкенда для каждого пакета
 │  бэкендов   │  Стратегия: приоритет пользователя → ленивый зонд → доступность
 │  + Реестр   │  20 бэкендов автоматически обнаруживаются, никогда не загружаются, если не присутствуют
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ Транзакция  │  begin → verify → install → commit
 │             │  При неудаче: откат через журнал отмены
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   Слой абстракции бэкенда                    │
 │                                              │
 │  Linux:    apt · pacman · dnf · zypper       │
 │            portage · apk · xbps · nix · run  │
 │  BSD:      pkg · pkg_add · pkgin             │
 │  macOS:    brew · macports                   │
 │  Windows:  winget · scoop · chocolatey       │
 │  Universal: flatpak · snap · appimage · spk  │
 └──────┬───────────────────────────────────────┘
        │
        ▼
  Системный пакетный менеджер  (или SPK: обрабатывается нативно в SSPM)
```

**Поддерживаемые подсистемы:**

| Подсистема | Роль |
|-----------|------|
| Mirror | Геолокация по IP, ранжирование по задержке, VPN-ориентированный автoselect |
| Network | Параллельная загрузка libcurl, возобновление, запасное зеркало |
| Security | Подписи ed25519 + проверка sha256 для каждого файла |
| Cache | `~/.cache/sspm` — загрузки + метаданные + индексы репозиториев |
| SkyDB | SQLite: пакеты · история · транзакции · репозитории · профили |
| Repo | Синхронизация SPK-репозиториев, получение индексов, проверка подписей |
| Index | Нечеткий поиск, поиск по regex, граф зависимостей |
| Profile | Именованные группы пакетов: dev / desktop / server / gaming |
| Logger | `~/.local/state/sspm/log/` — уровневый + режим tail |
| Doctor | Бэкенд · разрешения · сеть · репозитории · кэш · здоровье БД |
| Plugin | Расширения бэкенда `dlopen()` из `~/.local/share/sspm/plugins/` |
| REST API | `sspm-daemon` на `:7373` — обеспечивает работу SSPM Center и GUI-интерфейсов |

| Компонент | Описание |
|-----------|-------------|
| `CLI` | Анализ аргументов, маршрутизация команд, форматирование вывода |
| `Package Resolver` | Выбирает лучший бэкенд для каждого пакета |
| `Backend Layer` | Адаптеры для каждого собственного пакетного менеджера |
| `Transaction System` | Атомная установка/удаление с поддержкой отката |
| `SkyDB` | Локальная база данных состояния на основе SQLite |
| `Repo System` | Официальные, сторонние и локальные репозитории `.spk` |
| `SPK Format` | Собственный переносимый формат пакетов SSPM |
| `Cache System` | Кэш загрузок, метаданные, индексы репозиториев |
| `Profile System` | Группы пакетов на основе окружения |
| `Mirror System` | Автоматическая геодетекция + зеркала с ранжированием по задержке |
| `Security` | Подписи ed25519 + проверка sha256 |
| `Doctor` | Системная диагностика и проверки здоровья |
| `REST API` | Режим демона для GUI-интерфейсов |
| `Plugin System` | Расширяемые бэкенды и хуки |

---

## 💻 Поддерживаемые платформы

### Linux

| Семейство | Пакетные менеджеры |
|--------|-----------------|
| Debian / Ubuntu | `apt`, `apt-get`, `dpkg` |
| Red Hat / Fedora | `dnf`, `yum`, `rpm` |
| SUSE | `zypper` |
| Arch | `pacman` |
| Gentoo | `portage` (emerge) |
| Alpine | `apk` |
| Void | `xbps` |
| NixOS | `nix-env` |

### BSD

| Система | Пакетный менеджер |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| Инструмент | Примечания |
|------|-------|
| `homebrew` | Также доступен как Linuxbrew на Linux |
| `macports` | Альтернативный бэкенд |

### Windows

| Инструмент | Примечания |
|------|-------|
| `winget` | Встроенный пакетный менеджер Windows |
| `scoop` | Установщик пользовательского пространства |
| `chocolatey` | Пакетный менеджер сообщества |

### Универсальные (Кросс-платформенные)

| Бэкенд | Примечания |
|---------|-------|
| `flatpak` | Песочнизированные приложения Linux |
| `snap` | Универсальный формат Canonical |
| `AppImage` | Переносимые двоичные файлы Linux |
| `nix profile` | Репродуцируемые кросс-платформенные пакеты |
| `spk` | Собственный формат пакетов SSPM |

---

## 📦 Установка

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

Или сборка из исходников:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# или
scoop install sspm
```

---

## 🚀 Быстрый старт

```bash
# Установка пакета
sspm install nginx

# Поиск пакетов
sspm search nodejs

# Обновление базы данных пакетов
sspm update

# Обновление всех пакетов
sspm upgrade

# Удаление пакета
sspm remove nginx

# Список установленных пакетов
sspm list

# Получение информации о пакете
sspm info nginx

# Выполнение системной диагностики
sspm doctor
```

---

## 📟 Справочник CLI

### Основные команды

| Команда | Описание |
|---------|-------------|
| `sspm install <pkg>` | Установка пакета |
| `sspm remove <pkg>` | Удаление пакета |
| `sspm search <query>` | Поиск пакетов |
| `sspm update` | Синхронизация базы данных пакетов |
| `sspm upgrade` | Обновление всех установленных пакетов |
| `sspm list` | Список установленных пакетов |
| `sspm info <pkg>` | Отображение деталей пакета |
| `sspm doctor` | Выполнение системной диагностики |

### Расширенные команды

| Команда | Описание |
|---------|-------------|
| `sspm repo <sub>` | Управление репозиториями |
| `sspm cache <sub>` | Управление кэшем загрузок |
| `sspm config <sub>` | Редактирование конфигурации |
| `sspm profile <sub>` | Управление профилями окружения |
| `sspm history` | Просмотр истории установки/удаления |
| `sspm rollback` | Откат последней транзакции |
| `sspm verify <pkg>` | Проверка целостности пакета |
| `sspm mirror <sub>` | Управление источниками зеркал |
| `sspm log` | Просмотр логов SSPM |
| `sspm log tail` | Отслеживание живого вывода логов |

### Флаги вывода

```bash
sspm search nginx --json          # JSON-вывод
sspm list --format table          # Табличный вывод
sspm install nginx --verbose      # Подробный режим
sspm install nginx --dry-run      # Предпросмотр без выполнения
sspm install nginx --backend apt  # Принудительное использование определенного бэкенда
```

### Пользовательский стиль команд

SSPM поддерживает несколько конвенций команд. Переключитесь на стиль pacman, если предпочитаете:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → установка
# sspm -Rs nginx     → удаление
# sspm -Ss nginx     → поиск
# sspm -Syu          → обновление всех

sspm config set cli.style apt     # по умолчанию
```

---

## 🔧 Система бэкендов

### Ленивое автоматическое обнаружение

SSPM никогда не загружает бэкенд, который не установлен. Backend Registry проверяет каждый из 20 бэкендов, проверяя наличие их двоичного файла — и **кеширует результат**. Нет двоичного файла = не загружается, нет накладных расходов.

```
На Arch Linux:
  /usr/bin/pacman   ✅  загружен   (приоритет 10)
  /usr/bin/apt-get  ⬜  пропущен
  /usr/bin/dnf      ⬜  пропущен
  /usr/bin/flatpak  ✅  загружен   (приоритет 30)
  spk (встроенный)   ✅  загружен   (приоритет 50)
```

После установки нового инструмента (например, `flatpak`) выполните `sspm doctor` для повторной проверки.

### Приоритет бэкендов

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

Принудительное использование определенного бэкенда для одной команды:

```bash
sspm install firefox --backend flatpak
```

### Метки бэкендов (Интеграция с SSPM Center / Store)

| Метка | Бэкенд |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | Нативный SPK |

---

## 🔍 Решатель зависимостей

Решатель SSPM обрабатывает три проблемы перед установкой чего-либо:

**1. Решение зависимостей** — все транзитивные зависимости расширяются рекурсивно  
**2. Решение версий** — несколько ограничений для одного и того же пакета объединяются с помощью AND  
**3. Решение конфликтов** — проверка `conflicts`, `breaks` и обратных зависимостей

```bash
sspm install certbot --dry-run

Разрешение зависимостей...
План установки (6 пакетов):
  установка  libc      2.38    зависимость python3
  установка  libffi    3.4.4   зависимость python3
  установка  openssl   3.1.4   зависимость certbot (>= 3.0)
  установка  python3   3.12.0  зависимость certbot (>= 3.9 после добавления ограничения acme)
  установка  acme      2.7.0   зависимость certbot
  установка  certbot   2.7.4   запрошенный
```

См. [docs/RESOLVER.md](docs/RESOLVER.md) для полного алгоритма.

---




```
sspm install nginx
      ↓
Проверка пакетного резолвера:
  1. Конфигурированный пользователем backend_priority
  2. Какие бэкенды имеют этот пакет доступным
  3. Выбор доступного бэкенда с наивысшим приоритетом
      ↓
Пример результата: pacman (Arch) → выполнение: pacman -S nginx
```

### Конфигурация приоритета бэкендов

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### Принудительное использование бэкенда

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### Интерфейс бэкенда

Каждый адаптер бэкенда реализует один и тот же абстрактный интерфейс:

```cpp
class Backend {
public:
    virtual Result install(const Package& pkg) = 0;
    virtual Result remove(const Package& pkg) = 0;
    virtual SearchResult search(const std::string& query) = 0;
    virtual Result update() = 0;
    virtual Result upgrade() = 0;
    virtual PackageList list_installed() = 0;
};
```

---

## 📦 Формат пакетов SPK

`.spk` — это собственный переносимый формат пакетов SSPM.

### Структура пакета

```
package.spk
├── metadata.toml       # Название пакета, версия, зависимости, скрипты
├── payload.tar.zst     # Сжатый файловый payload (zstd)
├── install.sh          # Хуки pre/post установки
├── remove.sh           # Хуки pre/post удаления
└── signature           # Подпись ed25519
```

### Пример metadata.toml

```toml
[package]
name = "example"
version = "1.0.0"
description = "An example SPK package"
author = "Riu-Mahiyo"
license = "MIT"
arch = ["x86_64", "aarch64"]

[dependencies]
required = ["libc >= 2.17", "zlib"]
optional = ["libssl"]

[scripts]
pre_install  = "install.sh pre"
post_install = "install.sh post"
pre_remove   = "remove.sh pre"
post_remove  = "remove.sh post"
```

### Сборка пакета SPK

```bash
sspm build ./mypackage/
# Вывод: mypackage-1.0.0.spk
```

---

## 🗄️ Система репозиториев

### Команды репозиториев

```bash
sspm repo add https://repo.example.com/sspm     # Добавление репозитория
sspm repo remove example                        # Удаление репозитория
sspm repo sync                                  # Синхронизация всех репозиториев
sspm repo list                                  # Список конфигурарованных репозиториев
```

### Формат репозитория

```
repo/
├── repo.json       # Метаданные репозитория & индекс пакетов
├── packages/       # Файлы пакетов .spk
└── signature       # Ключ подписи репозитория (ed25519)
```

Репозитории поддерживают: **официальные**, **сторонние** и **локальные** источники.

---

## 🧑‍💼 Система профилей

Профили группируют пакеты по окружению, что облегчает воспроизведение полной настройки.

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### Встроенные шаблоны профилей

| Профиль | Типичные пакеты |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | браузер, медиаплеер, офисный набор |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 Система зеркал

### Автоматическая геодетекция

SSPM определяет вашу страну через геолокацию IP и автоматически переключает **все бэкенды** на самые быстрые региональные зеркала — даже через VPN.

Для пользователей с **правилами прокси** (не полным туннелем VPN) SSPM определяет фактический исходящий IP и корректно обрабатывает выбор зеркала.

```bash
sspm mirror list              # Список доступных зеркал
sspm mirror test              # Бенчмарк зеркал по задержке
sspm mirror select            # Ручной выбор зеркала
```

### Конфигурация зеркал

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 Безопасность

| Функция | Реализация |
|---------|---------------|
| Подписи пакетов | `ed25519` |
| Проверка хэша | `sha256` |
| Якоря доверия репозиториев | Пиннинг открытого ключа per-repo |
| Обязательно для `.spk` | Проверка подписи перед установкой |

```bash
sspm verify nginx              # Проверка установленного пакета
sspm verify ./package.spk      # Проверка локального файла SPK
```

---

## 🏥 Doctor & Диагностика

```bash
sspm doctor
```

Выполняемые проверки:

- ✅ Доступность и версия бэкенда
- ✅ Разрешения файлов
- ✅ Соединение с сетью  
- ✅ Доступность репозиториев
- ✅ Целостность кэша
- ✅ Целостность базы данных SkyDB
- ✅ Задержка зеркала

---

## 🔌 Режим API & GUI

### Режим демона

```bash
sspm daemon start      # Запуск REST API демона
sspm daemon stop
sspm daemon status
```

### REST API эндпоинты

| Метод | Эндпоинт | Описание |
|--------|----------|-------------|
| `GET` | `/packages` | Список установленных пакетов |
| `GET` | `/packages/search?q=` | Поиск пакетов |
| `POST` | `/packages/install` | Установка пакета |
| `DELETE` | `/packages/:name` | Удаление пакета |
| `GET` | `/repos` | Список репозиториев |
| `POST` | `/repos` | Добавление репозитория |
| `GET` | `/health` | Проверка здоровья демона |

### SSPM Center (GUI-интерфейс)

SSPM Center — это официальный графический интерфейс, интегрированный с:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

Пакеты помечены по источнику бэкенда:

| Метка | Значение |
|-------|---------|
| `SSPM-APT` | Установлено через apt |
| `SSPM-PACMAN` | Установлено через pacman |
| `SSPM-FLATPAK` | Установлено через Flatpak |
| `SSPM-SNAP` | Установлено через Snap |
| `SSPM-NIX` | Установлено через Nix |
| `SSPM-SPK` | Нативный пакет SSPM |

Теги категорий: `🛠 Tools` · `🎮 Games` · `🎵 Media` · `⚙️ System` · `📦 Development` · `🌐 Network`

---

## 🧩 Система плагинов

Расширьте SSPM с помощью пользовательских бэкендов и хуков:

```
~/.local/share/sspm/plugins/
├── aur/            # Бэкенд AUR (Arch User Repository)
├── brew-tap/       # Поддержка Homebrew tap
└── docker/         # Бэкенд Docker-образов
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ Конфигурация

**Файл конфигурации:** `~/.config/sspm/config.yaml`

```yaml
# Порядок приоритета бэкендов
backend_priority:
  - pacman
  - flatpak
  - nix

# Стиль CLI: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# Настройки зеркал и геолокации
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# Настройки кэша
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# Логирование
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 Структура проекта

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # Парсинг аргументов CLI и маршрутизация команд
│   ├── resolver/         # Решатель пакетов и селектор бэкендов
│   ├── backends/         # Адаптеры бэкендов
│   │   ├── apt/
│   │   ├── pacman/
│   │   ├── dnf/
│   │   ├── zypper/
│   │   ├── portage/
│   │   ├── apk/
│   │   ├── xbps/
│   │   ├── nix/
│   │   ├── brew/
│   │   ├── macports/
│   │   ├── winget/
│   │   ├── scoop/
│   │   ├── chocolatey/
│   │   ├── flatpak/
│   │   ├── snap/
│   │   ├── appimage/
│   │   ├── pkg/          # FreeBSD
│   │   ├── pkg_add/      # OpenBSD
│   │   ├── pkgin/        # NetBSD
│   │   └── spk/          # Нативный SSPM
│   ├── transaction/      # Атомная система транзакций + откат
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # Управление репозиториями
│   ├── spk/              # Сборщик и парсер пакетов SPK
│   ├── cache/            # Система кэша
│   ├── profile/          # Система профилей
│   ├── mirror/           # Ранжирование зеркал и геодетекция
│   ├── security/         # Проверка ed25519 + sha256
│   ├── doctor/           # Системная диагностика
│   ├── api/              # REST API демон
│   ├── log/              # Система логирования
│   ├── network/          # HTTP-клиент (libcurl)
│   └── plugin/           # Загрузчик плагинов
│
├── include/              # Публичные заголовки
├── tests/                # Unit- и интеграционные тесты
├── docs/                 # Полная документация
├── packages/             # Официальные рецепты пакетов SPK
└── assets/               # Логотипы, иконки
```

---

## 🤝 Вклад в проект

Вклады очень приветствуются! Пожалуйста, прочтите [CONTRIBUTING.md](CONTRIBUTING.md) перед отправкой PR.

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 Лицензия

GPLv2 License — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Made with 🌸 by [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>