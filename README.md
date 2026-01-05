# SysWatch - Linux System Monitor for Raspberry Pi | Real-Time Hardware Monitoring

[![GitHub Release](https://img.shields.io/github/v/release/Marauder699/SysWatch)](https://github.com/Marauder699/SysWatch/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-green.svg)](https://www.linux.org/)
[![GTK3](https://img.shields.io/badge/GTK-3-blue.svg)](https://www.gtk.org/)
[![Raspberry Pi](https://img.shields.io/badge/Raspberry%20Pi-Compatible-red.svg)](https://www.raspberrypi.org/)

🖥️ **Lightweight GTK system monitor for Linux/Raspberry Pi** - Real-time CPU/GPU/Memory/Disk monitoring with color-coded temperature alerts. Multi-architecture support (ARM, x86, x64).

## What is SysWatch?  

SysWatch is a **free, open-source system monitor** for **Linux** and **Raspberry Pi** built with **GTK3** and **C**. It provides real-time monitoring of CPU temperature, GPU usage, memory consumption, network traffic, and disk performance with a modern graphical interface.

Perfect for Raspberry Pi enthusiasts, Linux system administrators, and developers who need lightweight hardware monitoring with visual temperature alerts.

**Hybrid GUI application** in C combining:
- 📋 **System Information**: detailed hardware and OS specifications
- 🌡️ **Health monitoring**: real-time system health indicators (color-coded temperature)

Designed for Linux (Raspberry Pi, PC, servers) with a modern GTK interface. 

## 🔍 Keywords & Topics

**Search Keywords**:  
system monitor • linux monitoring • raspberry pi monitoring • gtk application • real-time monitoring • cpu temperature monitor • gpu monitoring • memory usage monitor • disk speed test • network monitoring • health monitoring • desktop application • c programming • system health • hardware monitoring • thermal monitoring • lightweight monitor

**Recommended GitHub Topics** (to add in repository settings):
`system-monitor` • `linux` • `raspberry-pi` • `gtk` • `c` • `health-monitoring` • `real-time-monitoring` • `desktop-application` • `hardware-monitoring` • `temperature-monitor` • `cpu-monitor` • `gpu-monitor` • `performance-monitoring` • `system-information`

*Note: Repository topics can be added by navigating to the repository Settings tab → scrolling to the "Topics" section*

## 📥 Quick Download

**[⬇️ Download Latest Release (ARM64 - Raspberry Pi)](https://github.com/Marauder699/SysWatch/releases/latest/download/syswatch-aarch64-linux)**

```bash
# Quick install (Raspberry Pi 64-bit)
wget https://github.com/Marauder699/SysWatch/releases/latest/download/syswatch-aarch64-linux
chmod +x syswatch-aarch64-linux
sudo apt-get install libgtk-3-0  # Install dependency if needed
./syswatch-aarch64-linux
```

**Or compile from source for other platforms (x86_64, ARMv7)** → See [Build](#-build) section below

## 🎯 Current Features (v1.0)

### 📋 System Information (Static)
- ✅ Displays hardware model (Raspberry Pi, PC, etc.)
- ✅ Processor detection with exact architecture
- ✅ GPU identification (NVIDIA, AMD, Intel, Broadcom)
- ✅ Linux kernel version
- ✅ Distribution and version
- ✅ Desktop environment + display server (X11/Wayland)
- ✅ CPU core count

### 🌡️ Health Monitoring (Real-Time - 1s)
- ✅ **CPU temperature** with color indicators 🟢🟡🔴
  - 🟢 Green: < 60°C (optimal)
  - 🟡 Yellow: 60–75°C (warning)
  - 🔴 Red: > 75°C (hot)
- ✅ **CPU usage** (overall %)
- ✅ **GPU usage** (%)
- ✅ **Memory used** (%) + available/total (GB)
- ✅ **Network throughput** (upload/download) per interface
- ✅ **System uptime**

### 🌐 Network
- ✅ Lists network interfaces (Ethernet, WiFi, Mobile)
- ✅ **IP address per interface** (dynamic, refreshed every 1s)
- ✅ Hostname
- ✅ Real-time upload/download per interface

### 💾 Storage (Disks)
- ✅ **Refresh button** — updates the disk list when new disks are connected (USB, SD card, external HDD)
- ✅ **Disk speed test** — measures read/write in MB/s per disk (on-demand)
- ✅ **Physical disk identification** — NVMe, USB SSD, HDD with capacity
- ✅ **Interface detection**: 
  - NVMe:  automatically detects PCIe Gen3/Gen4/Gen5
  - USB:  detects USB 1.x / 2.0 / 3.0 / 3.1+ by actual speed
  - SATA/IDE: traditional HDD
- ✅ **Used/available space** per disk (real-time)

### 🎨 Interface
- ✅ **Automatic refresh every 1 second** for health monitoring
- ✅ **Visual color indicators** (CPU temperature:  🟢🟡🔴)
- ✅ GTK3-based modern responsive GUI
- ✅ Modular architecture **(MVC)**:  separation of view/model/controller
- ✅ Adaptive layout that responds to window resizing

## 📸 Screenshots

![App Screenshot](https://github.com/Marauder699/SysWatch/releases/download/v1.0.0/SysWatch.png)

## 📋 Prerequisites

```bash
# Install GTK3 dependencies
sudo apt-get update
sudo apt-get install libgtk-3-dev pkg-config
```

Or use the Makefile:

```bash
make install-deps
```

## 🔨 Build

```bash
# Build
make

# Clean
make clean

# Build + run
make run
```

## 🚀 Run

```bash
# Launch the application
./syswatch

# Or via Makefile
make run
```

## 📁 Project Structure

```
SysWatch/
├── include/              # Headers (public APIs)
│   ├── system_info.h     # System info functions
│   └── gui.h             # GUI interface
├── src/                  # Implementations
│   ├── main.c            # Entry point
│   ├── system_info.c     # Model (system data ~1700 lines)
│   └── gui.c             # View (GTK UI ~960 lines)
├── obj/                  # Object files (. o)
├── Makefile              # Build system
├── README.md             # Documentation (this file)
├── ROADMAP.md            # Planned features
├── SPECIFICATIONS.md     # Technical details
└── syswatch              # Compiled executable
```

## 🗺️ Roadmap (see `ROADMAP.md`)

See `ROADMAP.md` for the detailed plan of upcoming features.

### 🌍 Next major feature:  Multilingual system

A complete translation system using JSON files per language is planned to make the app easily localizable.  See `ROADMAP.md` for details.

## 🔧 Technical architecture

- **Model**:  `system_info.c` — reads system data via `/sys`, `/proc`
- **View**: `gui.c` — GTK+ interface with tabs (System, CPU, Memory, Network, Disk)
- **Controller**: `main.c` — orchestration and main loop
- **Portable**: Works on Raspberry Pi, PC Linux, servers (x86, ARM, ARM64)

## 📊 App Tabs

### 1️⃣ System
- Hardware model
- Processor (with architecture)
- GPU (vendor and model)
- Kernel version
- Distribution
- Desktop Environment
- Locale
- Uptime

### 2️⃣ CPU
- Current temperature (format: 45.2°C (113.4°F))
- CPU usage (%)
- GPU usage (%)
- Core count

### 3️⃣ Memory
- Usage (%)
- Available (GB)
- Total (GB)

### 4️⃣ Network
- Hostname
- Active interfaces (detected type)
- **IP address per interface** (new!)
- Upload/download throughput per interface

### 5️⃣ Disk ✨ (New!)
- List of physical disks
- **Refresh button** — reload disk list when new devices are connected
- **Speed Test** — read/write speed per disk
- Total and used capacity
- Disk type identification
- **Detected interfaces**:  PCIe Gen3/4/5, USB 2.0/3.0/3.1+, SATA

## 📝 Technical notes

### GPU detection
- NVIDIA: via `nvidia-smi`
- AMD: via `/sys/class/drm/card*/device/gpu_busy_percent`
- Intel: via GPU frequency `/sys/class/drm/card*/gt/gt0/rps_*_freq_mhz`
- Raspberry Pi: automatically detects Broadcom VideoCore (IV/VI/VII depending on model)

### Disk detection
- **NVMe**: read PCIe current link speed via `/sys/block/nvme*/device/device/current_link_speed` (GT/s)
- **USB**: identify via `/sys/block/sd*/device/../speed` (real Mbps)
- **SATA**: detect via `/sys/block/`

### Disk speed test
- Uses `O_DIRECT` to bypass system cache
- 512-byte aligned buffers (required by O_DIRECT)
- Automatic fallback if O_DIRECT not supported
- Tests each disk at its mountpoint
- 100 MB per test (separate read and write)

## 💡 Project philosophy

**SysWatch** combines the best of two approaches: 

### Clear system information
- Clean presentation of hardware specs
- Comprehensive OS information
- Organized, readable interface

### Practical health monitoring
- Real-time health monitoring
- Visual temperature indicators
- CPU/GPU/Memory/Network monitoring
- Modern, responsive design

## 🎓 What started as a "vibe project"

✅ **Originally planned:** CPU temperature only  
✨ **Delivered:**
- Multi-tab system
- 15+ advanced detection functions
- Sophisticated disk speed test using O_DIRECT
- Accurate interface detection (USB Gen, PCIe Gen)
- Multi-architecture support (ARM, x86, x64)

- GPU handling for 4+ vendor types
- Dual-unit temperature display for global accessibility
- ~2700 lines of structured C code

## 🎯 Use Cases

SysWatch is perfect for:
- 🍓 **Raspberry Pi enthusiasts** monitoring their SBCs (Single Board Computers)
- 🖥️ **Linux desktop users** wanting lightweight system monitoring
- 🔧 **System administrators** needing real-time health indicators
- 👨‍💻 **Developers** testing applications under different system loads
- 🎮 **Overclockers** monitoring temperatures and performance
- 📊 **Hardware testers** benchmarking disk speeds and system capabilities
- 🌡️ **Temperature-sensitive applications** requiring thermal monitoring

## 📄 License

MIT License - Free to use (see `LICENSE` file).

## 💝 Support the Project

If SysWatch helps you or you'd like to support development, consider:

**Donation Platforms** (coming soon - to be decided):
- Ko-fi
- Buy Me a Coffee
- Patreon

*Note: You can use SysWatch completely free regardless - donations are optional and appreciated! *

## 👤 Author

**Stéphane Corriveau** — Full-Stack Developer

### Journey
- **🕹️ Early Years**: Vic-20, Commodore 64, Amiga 500 (the classics!)
- **🎓 University**: Pascal, C/C++
- **💼 Early Career**:  Delphi, C++ Builder
- **🏢 Mid Career**: 10+ years in enterprise systems (SAP)
- **🌐 Recent (10 years)**: Web development (TypeScript, Angular, C# . NET, Python)

### Current Focus
- 🐧 Linux enthusiast, junior exploring deeper
- 🔧 System tools and optimization
- 💻 Full-stack across web, desktop, and embedded systems

Developed December 2025 for Raspberry Pi and general Linux  
With passion and lots of fun ❤️