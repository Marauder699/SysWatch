# SysWatch - System Information & Health Monitor

🖥️ **Hybrid GUI application** in C combining:
- 📋 **System Information**: detailed hardware and OS specifications
- 🌡️ **Health monitoring**: real-time system health indicators (color-coded temperature)

Designed for Linux (Raspberry Pi, PC, servers) with a modern GTK interface.

---

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
  - NVMe: automatically detects PCIe Gen3/Gen4/Gen5
  - USB: detects USB 1.x / 2.0 / 3.0 / 3.1+ by actual speed
  - SATA/IDE: traditional HDD
- ✅ **Used/available space** per disk (real-time)

### 🎨 Interface
- ✅ **Automatic refresh every 1 second** for health monitoring
- ✅ **Visual color indicators** (CPU temperature: 🟢🟡🔴)
- ✅ GTK3-based modern responsive GUI
- ✅ Modular architecture **(MVC)**: separation of view/model/controller
- ✅ Adaptive layout that responds to window resizing

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
├── obj/                  # Object files (.o)
├── Makefile              # Build system
├── README.md             # Documentation (this file)
├── ROADMAP.md            # Planned features
├── SPECIFICATIONS.md     # Technical details
└── syswatch              # Compiled executable
```

## 🗺️ Roadmap (see `ROADMAP.md`)

See `ROADMAP.md` for the detailed plan of upcoming features.

### 🌍 Next major feature: Multilingual system

A complete translation system using JSON files per language is planned to make the app easily localizable. See `ROADMAP.md` for details.

## 🔧 Technical architecture

- **Model**: `system_info.c` — reads system data via `/sys`, `/proc`
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
- **Detected interfaces**: PCIe Gen3/4/5, USB 2.0/3.0/3.1+, SATA

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

---

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

## 📄 License

MIT License - Free to use (see `LICENSE` file).

## 💝 Support the Project

If SysWatch helps you or you'd like to support development, consider:

**Donation Platforms** (coming soon - to be decided):
- Ko-fi
- Buy Me a Coffee
- Patreon

*Note: You can use SysWatch completely free regardless - donations are optional and appreciated!*

## 👤 Author

Developed December 2025 for Raspberry Pi and general Linux  
With lots of love and "Vibe Programming" ❤️
