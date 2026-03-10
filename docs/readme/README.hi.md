<div align="center">

# 🌸 SSPM — ShioSakura पैकेज मैनेजर

**यूनिवर्सल पैकेज ऑर्केस्ट्रेटर · क्रॉस-डिस्ट्रिब्यूशन पैकेज प्रबंधन ऑर्केस्ट्रेटर**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](../../LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#समर्थित-प्लेटफार्म)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** एक यूनिवर्सल पैकेज मैनेजर ऑर्केस्ट्रेटर है जो आपके सिस्टम के मूल पैकेज मैनेजरों के ऊपर स्थित है।
> एक कमांड। कोई भी डिस्ट्रो। कोई भी बैकएंड।

</div>

---

## 📖 सामग्री की सूची

- [SSPM क्या है?](#sspm-क्या-है)
- [वास्तुकला](#वास्तुकला)
- [समर्थित प्लेटफार्म](#समर्थित-प्लेटफार्म)
- [स्थापना](#स्थापना)
- [त्वरित प्रारंभ](#त्वरित-प्रारंभ)
- [CLI संदर्भ](#cli-संदर्भ)
- [बैकएंड सिस्टम](#बैकएंड-सिस्टम)
- [SPK पैकेज प्रारूप](#spk-पैकेज-प्रारूप)
- [रिपो सिस्टम](#रिपो-सिस्टम)
- [प्रोफ़ाइल सिस्टम](#प्रोफ़ाइल-सिस्टम)
- [मिरर सिस्टम](#मिरर-सिस्टम)
- [सुरक्षा](#सुरक्षा)
- [डॉक्टर और निदान](#डॉक्टर-और-निदान)
- [API मोड और GUI](#api-मोड-और-gui)
- [प्लगइन सिस्टम](#प्लगइन-सिस्टम)
- [विन्यास](#विन्यास)
- [परियोजना संरचना](#परियोजना-संरचना)
- [योगदान](#योगदान)
- [लाइसेंस](#लाइसेंस)

---

## 🌸 SSPM क्या है?

SSPM (**ShioSakura पैकेज मैनेजर**) एक **यूनिवर्सल पैकेज ऑर्केस्ट्रेटर** है — यह आपके सिस्टम के पैकेज मैनेजर को बदलता नहीं है, यह उन्हें *समन्वयित* करता है।

```
आप  →  sspm install nginx
              ↓
     बैकएंड अमूर्तता परत
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM आपके पर्यावरण का पता लगाता है, सबसे अच्छा उपलब्ध बैकएंड चुनता है, निर्भरताओं को हल करता है, लेन-देन को संभालता है, और **लिनक्स, macOS, BSD और विंडोज** पर आपको एक एकीकृत अनुभव प्रदान करता है।

---

## 🏗️ वास्तुकला

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  apt / pacman / brew शैली — आपकी पसंद
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  निर्भरता   │  संक्रमणात्मक निर्भरताओं को हल करें
 │  रिज़ॉल्वर   │  संस्करण सीमा हल करना (>=, <=, !=, …)
 │             │  संघर्ष / ब्रेक डिटेक्शन
 │             │  टोपोलॉजिकल सॉर्ट → इंस्टॉल क्रम
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  बैकएंड     │  प्रत्येक पैकेज के लिए सबसे अच्छा बैकएंड चुनें
 │  रिज़ॉल्वर   │  रणनीति: उपयोगकर्ता प्राथमिकता → आलसी जांच → उपलब्धता
 │  + रजिस्ट्री │  20 बैकएंड स्वचालित रूप से पहचाने जाते हैं, उपस्थित नहीं होने पर कभी भी लोड नहीं होते
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ लेन-देन    │  शुरू → सत्यापन → स्थापना → कमिट
 │             │  विफलता पर: पूर्ववत लाने के लिए पूर्ववत लाने के लोग से रोलबैक
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   बैकएंड अमूर्तता परत                        │
 │                                              │
 │  लिनक्स:    apt · pacman · dnf · zypper       │
 │            portage · apk · xbps · nix · run  │
 │  BSD:      pkg · pkg_add · pkgin             │
 │  macOS:    brew · macports                   │
 │  विंडोज:  winget · scoop · chocolatey       │
 │  यूनिवर्सल: flatpak · snap · appimage · spk  │
 └──────┬───────────────────────────────────────┘
        │
        ▼
  सिस्टम पैकेज मैनेजर  (या SPK: SSPM में मूल रूप से संभाला जाता है)
```

**समर्थित सबसिस्टम:**

| सबसिस्टम | भूमिका |
|-----------|------|
| मिरर | Geo-IP देश का पता लगाना, लेटेंसी रैंकिंग, VPN-जागरूक ऑटो-चयन |
| नेटवर्क | libcurl समानांतर डाउनलोड, रिज्यूम, मिरर फॉलबैक |
| सुरक्षा | ed25519 हस्ताक्षर + प्रति-फ़ाइल sha256 सत्यापन |
| कैश | `~/.cache/sspm` — डाउनलोड + मेटाडेटा + रिपो इंडेक्स |
| SkyDB | SQLite: पैकेज · इतिहास · लेन-देन · रिपो · प्रोफ़ाइल |
| रिपो | SPK रिपो सिंक, इंडेक्स प्राप्त करना, हस्ताक्षर जांच |
| इंडेक्स | फज़ी खोज, रेगेक्स खोज, निर्भरता ग्राफ |
| प्रोफ़ाइल | नामित पैकेज समूह: डेव / डेस्कटॉप / सर्वर / गेमिंग |
| लॉगर | `~/.local/state/sspm/log/` — स्तरित + टेल मोड |
| डॉक्टर | बैकएंड · अनुमतियाँ · नेटवर्क · रिपो · कैश · डीबी स्वास्थ्य |
| प्लगइन | `~/.local/share/sspm/plugins/` से बैकएंड एक्सटेंशन डायनामिक रूप से लोड करें |
| REST API | `:7373` पर `sspm-daemon` — SSPM सेंटर और GUI फ्रंटएंड को शक्ति प्रदान करता है |

| घटक | विवरण |
|-----------|-------------|
| `CLI` | तर्क विश्लेषण, कमांड रूटिंग, आउटपुट स्वरूपण |
| `पैकेज रिज़ॉल्वर` | प्रत्येक पैकेज के लिए सबसे अच्छा बैकएंड चुनता है |
| `बैकएंड परत` | प्रत्येक मूल पैकेज मैनेजर के लिए एडैप्टर |
| `लेन-देन सिस्टम` | रोलबैक समर्थन के साथ परमाणु इंस्टॉल/रिमूव |
| `SkyDB` | SQLite-आधारित स्थानीय स्टेट डेटाबेस |
| `रिपो सिस्टम` | आधिकारिक, तृतीय-पक्ष, और स्थानीय `.spk` रिपो |
| `SPK प्रारूप` | SSPM का अपना पोर्टेबल पैकेज प्रारूप |
| `कैश सिस्टम` | डाउनलोड कैश, मेटाडेटा, रिपो इंडेक्स |
| `प्रोफ़ाइल सिस्टम` | पर्यावरण-आधारित पैकेज समूह |
| `मिरर सिस्टम` | ऑटो भू-डिटेक्शन + लेटेंसी-रैंक किए गए मिरर |
| `सुरक्षा` | ed25519 हस्ताक्षर + sha256 सत्यापन |
| `डॉक्टर` | सिस्टम निदान और स्वास्थ्य जांच |
| `REST API` | GUI फ्रंटएंड के लिए डेमन मोड |
| `प्लगइन सिस्टम` | विस्तार योग्य बैकएंड और हुक |

---

## 💻 समर्थित प्लेटफार्म

### लिनक्स

| परिवार | पैकेज मैनेजर |
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

| सिस्टम | पैकेज मैनेजर |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| उपकरण | नोट्स |
|------|-------|
| `homebrew` | लिनक्स पर Linuxbrew के रूप में भी उपलब्ध है |
| `macports` | वैकल्पिक बैकएंड |

### विंडोज

| उपकरण | नोट्स |
|------|-------|
| `winget` | अंतर्निहित विंडोज पैकेज मैनेजर |
| `scoop` | उपयोगकर्ता-स्पेस इंस्टॉलर |
| `chocolatey` | सामुदायिक पैकेज मैनेजर |

### यूनिवर्सल (क्रॉस-प्लेटफार्म)

| बैकएंड | नोट्स |
|---------|-------|
| `flatpak` | सैंडबॉक्स्ड लिनक्स ऐप्स |
| `snap` | Canonical का यूनिवर्सल प्रारूप |
| `AppImage` | पोर्टेबल लिनक्स बाइनरीज |
| `nix profile` | पुनरुत्पादन योग्य क्रॉस-प्लेटफार्म पैकेज |
| `spk` | SSPM का मूल पैकेज प्रारूप |

---

## 📦 स्थापना

### लिनक्स / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

या स्रोत से बनाएं:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### विंडोज

```powershell
winget install SSPM
# या
scoop install sspm
```

---

## 🚀 त्वरित प्रारंभ

```bash
# पैकेज स्थापित करें
sspm install nginx

# पैकेज खोजें
sspm search nodejs

# पैकेज डेटाबेस अपडेट करें
sspm update

# सभी पैकेज अपग्रेड करें
sspm upgrade

# पैकेज हटाएं
sspm remove nginx

# स्थापित पैकेज सूची बनाएं
sspm list

# पैकेज जानकारी प्राप्त करें
sspm info nginx

# सिस्टम निदान चलाएं
sspm doctor
```

---

## 📟 CLI संदर्भ

### बुनियादी कमांड

| कमांड | विवरण |
|---------|-------------|
| `sspm install <pkg>` | पैकेज स्थापित करें |
| `sspm remove <pkg>` | पैकेज हटाएं |
| `sspm search <query>` | पैकेज खोजें |
| `sspm update` | पैकेज डेटाबेस सिंक करें |
| `sspm upgrade` | सभी स्थापित पैकेज अपग्रेड करें |
| `sspm list` | स्थापित पैकेज सूची बनाएं |
| `sspm info <pkg>` | पैकेज विवरण दिखाएं |
| `sspm doctor` | सिस्टम निदान चलाएं |

### उन्नत कमांड

| कमांड | विवरण |
|---------|-------------|
| `sspm repo <sub>` | रिपो प्रबंधित करें |
| `sspm cache <sub>` | डाउनलोड कैश प्रबंधित करें |
| `sspm config <sub>` | विन्यास संपादित करें |
| `sspm profile <sub>` | पर्यावरण प्रोफ़ाइल प्रबंधित करें |
| `sspm history` | स्थापना/हटाने का इतिहास देखें |
| `sspm rollback` | अंतिम लेन-देन को रोलबैक करें |
| `sspm verify <pkg>` | पैकेज अखंडता सत्यापित करें |
| `sspm mirror <sub>` | मिरर स्रोत प्रबंधित करें |
| `sspm log` | SSPM लॉग देखें |
| `sspm log tail` | लाइव लॉग आउटपुट का अनुसरण करें |

### आउटपुट फ्लैग

```bash
sspm search nginx --json          # JSON आउटपुट
sspm list --format table          # टेबल आउटपुट
sspm install nginx --verbose      # विस्तृत मोड
sspm install nginx --dry-run      # निष्पादन किए बिना पूर्वावलोकन करें
sspm install nginx --backend apt  # एक विशिष्ट बैकएंड बल दें
```

### कस्टम कमांड शैली

SSPM कई कमांड संविधानों का समर्थन करता है। यदि आप पसंद करते हैं, तो pacman शैली में स्विच करें:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → स्थापित करें
# sspm -Rs nginx     → हटाएं
# sspm -Ss nginx     → खोजें
# sspm -Syu          → सभी अपग्रेड करें

sspm config set cli.style apt     # डिफ़ॉल्ट
```

---

## 🔧 बैकएंड सिस्टम

### आलसी ऑटो-डिटेक्शन

SSPM कभी भी ऐसा बैकएंड लोड नहीं करता है जो स्थापित नहीं है। बैकएंड रजिस्ट्री 20 बैकएंडों में से प्रत्येक की जांच करती है उनके बाइनरी की उपस्थिति की जांच करके — और **परिणाम कैश करती है**। कोई बाइनरी = कोई लोड नहीं, कोई ओवरहेड नहीं।

```
Arch Linux पर:
  /usr/bin/pacman   ✅  लोड हुआ   (प्राथमिकता 10)
  /usr/bin/apt-get  ⬜  छोड़ दिया
  /usr/bin/dnf      ⬜  छोड़ दिया
  /usr/bin/flatpak  ✅  लोड हुआ   (प्राथमिकता 30)
  spk (बिल्ट-इन)    ✅  लोड हुआ   (प्राथमिकता 50)
```

एक नया टूल (जैसे `flatpak`) स्थापित करने के बाद, `sspm doctor` चलाकर पुन: जांच करें।

### बैकएंड प्राथमिकता

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

एक कमांड के लिए एक विशिष्ट बैकएंड बल दें:

```bash
sspm install firefox --backend flatpak
```

### बैकएंड लेबल (SSPM सेंटर / स्टोर एकीकरण)

| लेबल | बैकएंड |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | मूल SPK |

---

## 🔍 निर्भरता रिज़ॉल्वर

SSPM का रिज़ॉल्वर कुछ भी स्थापित करने से पहले तीन समस्याओं को संभालता है:

**1. निर्भरता हल** — सभी संक्रमणात्मक निर्भरताएं पुनरावर्ती रूप से विस्तारित होती हैं  
**2. संस्करण हल** — एक ही पैकेज पर कई बाधाएं AND की जाती हैं  
**3. संघर्ष हल** — `conflicts`, `breaks`, और रिवर्स-डेप जांचें

```bash
sspm install certbot --dry-run

निर्भरताएं हल करना...
इंस्टॉल योजना (6 पैकेज):
  इंस्टॉल  libc      2.38    python3 की निर्भरता
  इंस्टॉल  libffi    3.4.4   python3 की निर्भरता
  इंस्टॉल  openssl   3.1.4   certbot की निर्भरता (>= 3.0)
  इंस्टॉल  python3   3.12.0  certbot की निर्भरता (acme से बाधा जोड़ने के बाद >= 3.9)
  इंस्टॉल  acme      2.7.0   certbot की निर्भरता
  इंस्टॉल  certbot   2.7.4   अनुरोधित
```

पूरे एल्गोरिथ्म के लिए, [docs/RESOLVER.md](../../docs/RESOLVER.md) देखें।

---





```
sspm install nginx
      ↓
पैकेज रिज़ॉल्वर जांच करता है:
  1. उपयोगकर्ता-कॉन्फ़िगर्ड backend_priority
  2. कौन से बैकएंडों में यह पैकेज उपलब्ध है
  3. उच्चतम-प्राथमिकता वाला उपलब्ध बैकएंड चुनें
      ↓
उदाहरण परिणाम: pacman (Arch) → निष्पादित करता है: pacman -S nginx
```

### बैकएंड प्राथमिकता कॉन्फ़िगर करना

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### बैकएंड बल देना

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### बैकएंड इंटरफ़ेस

प्रत्येक बैकएंड एडैप्टर एक ही अमूर्त इंटरफ़ेस लागू करता है:

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

## 📦 SPK पैकेज प्रारूप

`.spk` SSPM का मूल पोर्टेबल पैकेज प्रारूप है।

### पैकेज संरचना

```
package.spk
├── metadata.toml       # पैकेज नाम, संस्करण, निर्भरताएं, स्क्रिप्ट
├── payload.tar.zst     # संपीड़ित फ़ाइल पेलोड (zstd)
├── install.sh          # पूर्व/पोस्ट इंस्टॉल हुक
├── remove.sh           # पूर्व/पोस्ट रिमूव हुक
└── signature           # ed25519 हस्ताक्षर
```

### metadata.toml उदाहरण

```toml
[package]
name = "example"
version = "1.0.0"
description = "एक उदाहरण SPK पैकेज"
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

### SPK पैकेज बनाना

```bash
sspm build ./mypackage/
# आउटपुट: mypackage-1.0.0.spk
```

---

## 🗄️ रिपो सिस्टम

### रिपो कमांड

```bash
sspm repo add https://repo.example.com/sspm     # रिपो जोड़ें
sspm repo remove example                        # रिपो हटाएं
sspm repo sync                                  # सभी रिपो सिंक करें
sspm repo list                                  # कॉन्फ़िगर किए गए रिपो सूची बनाएं
```

### रिपो प्रारूप

```
repo/
├── repo.json       # रिपो मेटाडेटा और पैकेज इंडेक्स
├── packages/       # .spk पैकेज फ़ाइलें
└── signature       # रिपो साइनिंग की (ed25519)
```

रिपो समर्थन करते हैं: **आधिकारिक**, **तृतीय-पक्ष**, और **स्थानीय** स्रोत।

---

## 🧑‍💼 प्रोफ़ाइल सिस्टम

प्रोफ़ाइल पर्यावरण के अनुसार पैकेजों को समूहीकृत करते हैं, जिससे पूरी सेटअप को पुन: प्राप्त करना आसान हो जाता है।

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### बिल्ट-इन प्रोफ़ाइल टेम्पलेट

| प्रोफ़ाइल | विशिष्ट पैकेज |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | ब्राउज़र, मीडिया प्लेयर, ऑफिस सूट |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 मिरर सिस्टम

### ऑटो भू-डिटेक्शन

SSPM IP भू-स्थान定位 के माध्यम से आपका देश पता लगाता है और स्वचालित रूप से **सभी बैकएंड** को सबसे तेज क्षेत्रीय मिरर में स्विच करता है — यहां तक कि VPN के माध्यम से भी।

**नियम-आधारित प्रॉक्सी** (पूर्ण-टनल VPN नहीं) के साथ उपयोगकर्ताओं के लिए, SSPM वास्तविक आउटबाउंड IP का पता लगाता है और मिरर चयन को सही ढंग से संभालता है।

```bash
sspm mirror list              # उपलब्ध मिरर सूची बनाएं
sspm mirror test              # लेटेंसी द्वारा मिरर का बेंचमार्क करें
sspm mirror select            # मैन्युअल रूप से मिरर चुनें
```

### मिरर कॉन्फ़िग

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 सुरक्षा

| सुविधा | कार्यान्वयन |
|---------|---------------|
| पैकेज हस्ताक्षर | `ed25519` |
| हैश सत्यापन | `sha256` |
| रिपो ट्रस्ट एंकर | प्रति-रिपो पब्लिक की पिनning |
| `.spk` के लिए अनिवार्य | इंस्टॉल से पहले हस्ताक्षर जांच |

```bash
sspm verify nginx              # स्थापित पैकेज की सत्यापन करें
sspm verify ./package.spk      # स्थानीय SPK फ़ाइल की सत्यापन करें
```

---

## 🏥 डॉक्टर और निदान

```bash
sspm doctor
```

किए गए जांच:

- ✅ बैकएंड उपलब्धता और संस्करण
- ✅ फ़ाइल अनुमतियां
- ✅ नेटवर्क कनेक्टिविटी  
- ✅ रिपो पहुंच
- ✅ कैश अखंडता
- ✅ SkyDB डेटाबेस अखंडता
- ✅ मिरर लेटेंसी

---

## 🔌 API मोड और GUI

### डेमन मोड

```bash
sspm daemon start      # REST API डेमन शुरू करें
sspm daemon stop
sspm daemon status
```

### REST API एंडपॉइंट

| विधि | एंडपॉइंट | विवरण |
|--------|----------|-------------|
| `GET` | `/packages` | स्थापित पैकेज सूची बनाएं |
| `GET` | `/packages/search?q=` | पैकेज खोजें |
| `POST` | `/packages/install` | पैकेज स्थापित करें |
| `DELETE` | `/packages/:name` | पैकेज हटाएं |
| `GET` | `/repos` | रिपो सूची बनाएं |
| `POST` | `/repos` | रिपो जोड़ें |
| `GET` | `/health` | डेमन स्वास्थ्य जांच |

### SSPM सेंटर (GUI फ्रंटएंड)

SSPM सेंटर आधिकारिक ग्राफिकल फ्रंटएंड है, जो निम्नलिखित के साथ एकीकृत है:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

पैकेज बैकएंड स्रोत द्वारा लेबल किए जाते हैं:

| लेबल | अर्थ |
|-------|---------|
| `SSPM-APT` | apt के माध्यम से स्थापित |
| `SSPM-PACMAN` | pacman के माध्यम से स्थापित |
| `SSPM-FLATPAK` | Flatpak के माध्यम से स्थापित |
| `SSPM-SNAP` | Snap के माध्यम से स्थापित |
| `SSPM-NIX` | Nix के माध्यम से स्थापित |
| `SSPM-SPK` | SSPM मूल पैकेज |

श्रेणी टैग: `🛠 उपकरण` · `🎮 गेम` · `🎵 मीडिया` · `⚙️ सिस्टम` · `📦 विकास` · `🌐 नेटवर्क`

---

## 🧩 प्लगइन सिस्टम

SSPM को कस्टम बैकएंड और हुक के साथ विस्तारित करें:

```
~/.local/share/sspm/plugins/
├── aur/            # AUR (Arch यूजर रिपो) बैकएंड
├── brew-tap/       # Homebrew टैप समर्थन
└── docker/         # Docker छवि बैकएंड
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ विन्यास

**कॉन्फ़िग फ़ाइल:** `~/.config/sspm/config.yaml`

```yaml
# बैकएंड प्राथमिकता क्रम
backend_priority:
  - pacman
  - flatpak
  - nix

# CLI शैली: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# मिरर और भू-सेटिंग्स
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# कैश सेटिंग्स
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# लॉगिंग
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 परियोजना संरचना

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # CLI तर्क विश्लेषण और कमांड रूटिंग
│   ├── resolver/         # पैकेज रिज़ॉल्वर और बैकएंड चयनकर्ता
│   ├── backends/         # बैकएंड एडैप्टर
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
│   │   └── spk/          # SSPM मूल
│   ├── transaction/      # परमाणु लेन-देन सिस्टम + रोलबैक
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # रिपो प्रबंधन
│   ├── spk/              # SPK पैकेज बिल्डर और पार्सर
│   ├── cache/            # कैश सिस्टम
│   ├── profile/          # प्रोफ़ाइल सिस्टम
│   ├── mirror/           # मिरर रैंकिंग और भू-डिटेक्शन
│   ├── security/         # ed25519 + sha256 सत्यापन
│   ├── doctor/           # सिस्टम निदान
│   ├── api/              # REST API डेमन
│   ├── log/              # लॉग सिस्टम
│   ├── network/          # HTTP क्लाइंट (libcurl)
│   └── plugin/           # प्लगइन लोडर
│
├── include/              # सार्वजनिक हेडर
├── tests/                # यूनिट और एकीकरण परीक्षण
├── docs/                 # पूर्ण दस्तावेज़ीकरण
├── packages/             # आधिकारिक SPK पैकेज रेसिपी
└── assets/               # लोगो, आइकन
```

---

## 🤝 योगदान

योगदान बहुत स्वागत्य है! PR सबमिट करने से पहले [CONTRIBUTING.md](../../CONTRIBUTING.md) पढ़ें।

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 लाइसेंस

GPLv2 लाइसेंस — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

[Riu-Mahiyo](https://github.com/Riu-Mahiyo) द्वारा 🌸 के साथ बनाया गया

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>