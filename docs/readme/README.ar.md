<div align="center">

# 🌸 SSPM — ShioSakura Package Manager

**محلل حزم عالمي · محلل إدارة حزم متعدد التوزيعات**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#supported-platforms)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** هو محلل عامل إدارة الحزم الذي يقع فوق مديري الحزم الأصليين للنظام.  
> أمر واحد. أي توزيع. أي خلفية.

</div>

---

## 📖 جدول المحتويات

- [ما هو SSPM؟](#what-is-sspm)
- [الархитكتура](#architecture)
- [المنصات المدعومة](#supported-platforms)
- [التثبيت](#installation)
- [بدء التشغيل السريع](#quick-start)
- [مرجع CLI](#cli-reference)
- [نظام الخلفية](#backend-system)
- [تنسيق حزمة SPK](#spk-package-format)
- [نظام المستودعات](#repo-system)
- [نظام الملفات الشخصية](#profile-system)
- [نظام المرايا](#mirror-system)
- [الأمان](#security)
- [Doctor & التشخيص](#doctor--diagnostics)
- [وضع API & GUI](#api-mode--gui)
- [نظام الإضافات](#plugin-system)
- [التكوين](#configuration)
- [بنية المشروع](#project-structure)
- [المساهمة](#contributing)
- [الترخيص](#license)

---

## 🌸 ما هو SSPM؟

SSPM (**ShioSakura Package Manager**) هو **محلل حزم عالمي** — إنه لا يحل محل مدير الحزم الخاص بنظامك، بل *يوchestrate* لهم.

```
أنت  →  sspm install nginx
              ↓
     طبقة التجريد الخلفية
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM يكتشف بيئتك، يختار أفضل خلفية متاحة، يحل التبعيات، يتعامل معاليات، ويعطيك تجربة موحدة على **Linux، macOS، BSD، وWindows**.

---

## 🏗️ Архитектурa

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  أسلوب apt / pacman / brew — حسب تفضيلك
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  المحلل     │  حل التبعيات الانتقالية
 │  للتبعيات  │  حل قيود الإصدار (>=, <=, !=, …)
 │             │  الكشف عن النزاعات / الانقطاعات
 │             │  الترتيب الطوبولوجي → ترتيب التثبيت
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  المحلل     │  اختيار أفضل خلفية لكل حزمة
 │  للخلفية   │  الإستراتيجية: أولوية المستخدم → اختبار كسول → التوفر
 │  + السجل   │  20 خلفية تم اكتشافها تلقائياً، لا يتم تحميلها أبداً إذا لم تكن موجودة
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ المعاملة   │  begin → verify → install → commit
 │             │  في حالة الفشل: التراجع عبر سجل التراجع
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   طبقة التجريد الخلفية                      │
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
  مدير الحزم النظامي  (أو SPK: يتم معالجته بشكل أصلي في SSPM)
```

**الأنظمة الفرعية المدعومة:**

| النظام الفرعي | الدور |
|-----------|------|
| Mirror | الكشف عن البلد عبر Geo-IP، الترتيب حسب التأخير، التحديد التلقائي مع الوعي بالVPN |
| Network | تحميل متوازي libcurl، استئناف، بديل للمرايا |
| Security | التوقيعات ed25519 + التحقق sha256 لكل ملف |
| Cache | `~/.cache/sspm` — التنزيلات + البيانات الوصفية + فهارس المستودعات |
| SkyDB | SQLite: الحزم · التاريخ · المعاملات · المستودعات · الملفات الشخصية |
| Repo | مزامنة مستودع SPK، جلب الفهارس، التحقق من التوقيع |
| Index | البحث الغامض، البحثregex، رسم تخطيط التبعيات |
| Profile | مجموعات الحزم المسماة: dev / desktop / server / gaming |
| Logger | `~/.local/state/sspm/log/` — حسب المستوى + وضع الذيل |
| Doctor | الخلفية · الأذونات · الشبكة · المستودعات · الكاش · صحة قاعدة البيانات |
| Plugin | امتدادات الخلفية `dlopen()` من `~/.local/share/sspm/plugins/` |
| REST API | `sspm-daemon` على `:7373` — يقوم بتشغيل SSPM Center وواجهات GUI |

| المكون | الوصف |
|-----------|-------------|
| `CLI` | تحليل الوسائط، توجيه الأوامر، تنسيق الإخراج |
| `Package Resolver` | اختيار أفضل خلفية لكل حزمة |
| `Backend Layer` | محولات لكل مدير حزم أصلي |
| `Transaction System` | التثبيت/الإزالة الذريع مع دعم التراجع |
| `SkyDB` | قاعدة بيانات حالة محلية تعتمد على SQLite |
| `Repo System` | المستودعات الرسمية، والثالثة الطرف، والمحلية `.spk` |
| `SPK Format` | تنسيق حزمة منفصل أصلي لـ SSPM |
| `Cache System` | ذاكرة التخزين المؤقت للتحميلات، والبيانات الوصفية، وفهارس المستودعات |
| `Profile System` | مجموعات الحزم بناءً على البيئة |
| `Mirror System` | الكشف الجغرافي التلقائي + المرايا المُرتبة حسب التأخير |
| `Security` | التوقيعات ed25519 + التحقق sha256 |
| `Doctor` | التشخيص النظامي والتحققات الصحية |
| `REST API` | وضع守护程序 لمواجهات GUI |
| `Plugin System` | الخلفيات والحاجزات القابلة للتوسيع |

---

## 💻 المنصات المدعومة

### Linux

| العائلة | مديري الحزم |
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

| النظام | مدير الحزم |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| الأداة | الملاحظات |
|------|-------|
| `homebrew` | متاح أيضًا على Linux كـ Linuxbrew |
| `macports` | خلفية بديلة |

### Windows

| الأداة | الملاحظات |
|------|-------|
| `winget` | مدير الحزم المدمج في Windows |
| `scoop` | مثبت مساحة المستخدم |
| `chocolatey` | مدير الحزم المجتمعي |

### عالمي (عابر المنصات)

| الخلفية | الملاحظات |
|---------|-------|
| `flatpak` | تطبيقات Linux مع حماية بيئية |
| `snap` | تنسيق عالمي من Canonical |
| `AppImage` | ملفات ثنائية Linux المحمولة |
| `nix profile` | حزم عبر المنصات قابلة للتكرار |
| `spk` | تنسيق حزمة أصلي لـ SSPM |

---

## 📦 التثبيت

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

أو بناء من المصدر:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# أو
scoop install sspm
```

---

## 🚀 بدء التشغيل السريع

```bash
# تثبيت حزمة
sspm install nginx

# البحث عن حزم
sspm search nodejs

# تحديث قاعدة بيانات الحزم
sspm update

# تحديث جميع الحزم
sspm upgrade

# إزالة حزمة
sspm remove nginx

# قائمة الحزم المثبتة
sspm list

# الحصول على معلومات حول الحزمة
sspm info nginx

# تشغيل التشخيص النظامي
sspm doctor
```

---

## 📟 مرجع CLI

### الأوامر الأساسية

| الأمر | الوصف |
|---------|-------------|
| `sspm install <pkg>` | تثبيت حزمة |
| `sspm remove <pkg>` | إزالة حزمة |
| `sspm search <query>` | البحث عن حزم |
| `sspm update` | مزامنة قاعدة بيانات الحزم |
| `sspm upgrade` | تحديث جميع الحزم المثبتة |
| `sspm list` | قائمة الحزم المثبتة |
| `sspm info <pkg>` | عرض تفاصيل الحزمة |
| `sspm doctor` | تشغيل التشخيص النظامي |

### الأوامر المتقدمة

| الأمر | الوصف |
|---------|-------------|
| `sspm repo <sub>` | إدارة المستودعات |
| `sspm cache <sub>` | إدارة ذاكرة التخزين المؤقت للتحميلات |
| `sspm config <sub>` | تعديل التكوين |
| `sspm profile <sub>` | إدارة الملفات الشخصية البيئية |
| `sspm history` | عرض تاريخ التثبيت/الإزالة |
| `sspm rollback` | التراجع عن آخر معاملة |
| `sspm verify <pkg>` | التحقق من سلامة الحزمة |
| `sspm mirror <sub>` | إدارة مصادر المرايا |
| `sspm log` | عرض سجلات SSPM |
| `sspm log tail` | متابعة خرج السجلات في الوقت الفعلي |

### أعلام الإخراج

```bash
sspm search nginx --json          # خرج JSON
sspm list --format table          # خرج جدول

sspm install nginx --verbose      # وضع مفصل

sspm install nginx --dry-run      # معاينة بدون تنفيذ

sspm install nginx --backend apt  # إجبار على خلفية معينة
```

### أسلوب أمر مخصص

SSPM يدعم العديد من توصيفات الأوامر. قم بالتبديل إلى أسلوب pacman إذا كنت تفضل ذلك:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → تثبيت
# sspm -Rs nginx     → إزالة
# sspm -Ss nginx     → البحث
# sspm -Syu          → تحديث الكل

sspm config set cli.style apt     # الافتراضي
```

---

## 🔧 نظام الخلفية

### الكشف التلقائي الكسول

SSPM لا يحمل أي خلفية غير مثبتة. سجل الخلفية يفحص كل من 20 خلفية من خلال التحقق من وجود ملفاتها الثنائية — و **يخزن النتيجة في ذاكرة التخزين المؤقت**. لا ملف ثنائي = لا تحميل، لا عبء زائد.

```
على Arch Linux:
  /usr/bin/pacman   ✅  تم تحميله   (أولوية 10)
  /usr/bin/apt-get  ⬜  تم تخطيه
  /usr/bin/dnf      ⬜  تم تخطيه
  /usr/bin/flatpak  ✅  تم تحميله   (أولوية 30)
  spk (مدمج)        ✅  تم تحميله   (أولوية 50)
```

بعد تثبيت أداة جديدة (مثل `flatpak`)، قم بتشغيل `sspm doctor` لإعادة الفحص.

### أولوية الخلفية

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

إجبار على خلفية معينة لأمر واحد:

```bash
sspm install firefox --backend flatpak
```

### علامات الخلفية (تكامل SSPM Center / Store)

| العلامة | الخلفية |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | SPK الأصلي |

---

## 🔍 محلل التبعيات

يتعامل محلل SSPM مع ثلاثة مشكلات قبل تثبيت أي شيء:

**1. حل التبعيات** — يتم تمديد جميع التبعيات الانتقالية بشكل متكرر  
**2. حل الإصدارات** — يتم AND العديد من القيود على نفس الحزمة  
**3. حل النزاعات** — تحقق `conflicts`، `breaks`، والتبعيات العكسية

```bash
sspm install certbot --dry-run

حل التبعيات...
خطة التثبيت (6 حزم):
  تثبيت  libc      2.38    تبعية python3
  تثبيت  libffi    3.4.4   تبعية python3
  تثبيت  openssl   3.1.4   تبعية certbot (>= 3.0)
  تثبيت  python3   3.12.0  تبعية certbot (>= 3.9 بعد إضافة قيد acme)
  تثبيت  acme      2.7.0   تبعية certbot
  تثبيت  certbot   2.7.4   مطلوب
```

راجع [docs/RESOLVER.md](docs/RESOLVER.md) للحصول على الخوارزمية الكاملة.

---




```
sspm install nginx
      ↓
تحقق محلل الحزم:
  1. backend_priority المضبوط من قبل المستخدم
  2. أي خلفية لديها هذا الحزمة متاحة
  3. اختيار الخلفية المتاحة بأعلى أولوية
      ↓
مثال النتيجة: pacman (Arch) → يقوم بتنفيذ: pacman -S nginx
```

### تكوين أولوية الخلفية

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### إجبار على خلفية

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### واجهة الخلفية

تقوم كل محول خلفية بتنفيذ نفس الواجهة المجردة:

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

## 📦 تنسيق حزمة SPK

`.spk` هو تنسيق حزمة منفصل أصلي لـ SSPM.

### بنية الحزمة

```
package.spk
├── metadata.toml       # اسم الحزمة، الإصدار، التبعيات، البرامج النصية
├── payload.tar.zst     # حمولة الملفات المضغوطة (zstd)
├── install.sh          # وصلات pre/post التثبيت
├── remove.sh           # وصلات pre/post الإزالة
└── signature           # توقيع ed25519
```

### مثال metadata.toml

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

### بناء حزمة SPK

```bash
sspm build ./mypackage/
# الإخراج: mypackage-1.0.0.spk
```

---

## 🗄️ نظام المستودعات

### أمرات المستودع

```bash
sspm repo add https://repo.example.com/sspm     # إضافة مستودع
sspm repo remove example                        # إزالة مستودع
sspm repo sync                                  # مزامنة جميع المستودعات
sspm repo list                                  # قائمة المستودعات المكوّنة
```

### تنسيق المستودع

```
repo/
├── repo.json       # بيانات الوصف للمستودع & فهرس الحزم
├── packages/       # ملفات الحزم .spk
└── signature       # مفتاح التوقيع للمستودع (ed25519)
```

يدعم المستودعات: مصادر **رسمية**، **ثالثة الطرف**، و**محلية**.

---

## 🧑‍💼 نظام الملفات الشخصية

تقوم الملفات الشخصية بتجميع الحزم حسب البيئة، مما يجعل من السهل إعادة إنشاء إعداد كامل.

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### نماذج الملفات الشخصية المضمنة

| الملف الشخصي | الحزم النموذجية |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | المتصفح، مشغل الوسائط، مجموعة المكتب |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 نظام المرايا

### الكشف الجغرافي التلقائي

يتكشف SSPM بلدك عبر تحديد المواقع الجغرافية IP وينتقل تلقائياً **جميع الخلفيات** إلى المرايا الإقليمية الأسرع — حتى عبر VPN.

للمستخدمين الذين لديهم **وكلاء يعتمدون على القواعد** (ليس VPN أنفاق كامل)، يتكشف SSPM عنوان IP الخروج الفعلي ويتعامل بشكل صحيح مع اختيار المرآة.

```bash
sspm mirror list              # قائمة المرايا المتاحة
sspm mirror test              # تحليل أداء المرايا حسب التأخير
sspm mirror select            # اختيار مرآة يدويًا
```

### تكوين المرايا

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 الأمان

| الميزة | التنفيذ |
|---------|---------------|
| توقيعات الحزم | `ed25519` |
| التحقق من التجزئة | `sha256` |
| أسطوانات الثقة للمستودع | تثبيت المفتاح العام per-repo |
| إلزامي لـ `.spk` | التحقق من التوقيع قبل التثبيت |

```bash
sspm verify nginx              # التحقق من حزمة مثبتة
sspm verify ./package.spk      # التحقق من ملف SPK محلي
```

---

## 🏥 Doctor & التشخيص

```bash
sspm doctor
```

التحققات المنفذة:

- ✅ توفر الخلفية وإصدارها
- ✅ أذونات الملفات
- ✅ الاتصال بالشبكة  
- ✅ توفر المستودعات
- ✅ سلامة الكاش
- ✅ سلامة قاعدة بيانات SkyDB
- ✅ تأخير المرآة

---

## 🔌 وضع API & GUI

### وضع守护程序

```bash
sspm daemon start      # بدء守护程序 REST API
sspm daemon stop
sspm daemon status
```

### نقاط نهاية REST API

| المنهج | نقطة النهاية | الوصف |
|--------|----------|-------------|
| `GET` | `/packages` | قائمة الحزم المثبتة |
| `GET` | `/packages/search?q=` | البحث عن حزم |
| `POST` | `/packages/install` | تثبيت حزمة |
| `DELETE` | `/packages/:name` | إزالة حزمة |
| `GET` | `/repos` | قائمة المستودعات |
| `POST` | `/repos` | إضافة مستودع |
| `GET` | `/health` | تحقق من صحة守护程序 |

### SSPM Center (واجهة GUI)

SSPM Center هو الواجهة الرسومية الرسمية، المتكاملة مع:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

تُسمى الحزم حسب مصدر الخلفية:

| العلامة | المعنى |
|-------|---------|
| `SSPM-APT` | مثبت عبر apt |
| `SSPM-PACMAN` | مثبت عبر pacman |
| `SSPM-FLATPAK` | مثبت عبر Flatpak |
| `SSPM-SNAP` | مثبت عبر Snap |
| `SSPM-NIX` | مثبت عبر Nix |
| `SSPM-SPK` | حزمة SSPM الأصلية |

علامات الفئة: `🛠 Tools` · `🎮 Games` · `🎵 Media` · `⚙️ System` · `📦 Development` · `🌐 Network`

---

## 🧩 نظام الإضافات

قم بتوسيع SSPM مع خلفيات وحاجزات مخصصة:

```
~/.local/share/sspm/plugins/
├── aur/            # خلفية AUR (مستودع المستخدمين Arch)
├── brew-tap/       # دعم Homebrew tap
└── docker/         # خلفية صور Docker
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ التكوين

**ملف التكوين:** `~/.config/sspm/config.yaml`

```yaml
# ترتيب أولوية الخلفية
backend_priority:
  - pacman
  - flatpak
  - nix

# أسلوب CLI: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# إعدادات المرآة والجغرافيا
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# إعدادات الكاش
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# التسجيل
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 بنية المشروع

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # تحليل وسيطات CLI وتوجيه الأوامر
│   ├── resolver/         # محلل الحزم ومحدد الخلفية
│   ├── backends/         # محولات الخلفية
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
│   │   └── spk/          # SSPM الأصلي
│   ├── transaction/      # نظام المعاملة الذريع + التراجع
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # إدارة المستودعات
│   ├── spk/              # مُبني ومُحلل حزم SPK
│   ├── cache/            # نظام الكاش
│   ├── profile/          # نظام الملفات الشخصية
│   ├── mirror/           # تصنيف المرايا والكشف الجغرافي
│   ├── security/         # التحقق ed25519 + sha256
│   ├── doctor/           # التشخيص النظامي
│   ├── api/              #守护程序 REST API
│   ├── log/              # نظام التسجيل
│   ├── network/          # عميل HTTP (libcurl)
│   └── plugin/           # محمل الإضافات
│
├── include/              # الرؤوس العامة
├── tests/                # الاختبارات الوحدوية وال интегра�יה
├── docs/                 # التوثيق الكامل
├── packages/             # أدواق حزم SPK الرسمية
└── assets/               # الشعارات، الرموز
```

---

## 🤝 المساهمة

مرحبًا بجميع المساهمات! يرجى قراءة [CONTRIBUTING.md](CONTRIBUTING.md) قبل إرسال PR.

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 الترخيص

GPLv2 License — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Made with 🌸 by [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>