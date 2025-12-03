# 🗺️ SysWatch - Roadmap & Development Plan

## ✅ Version 1.0 - COMPLETED (December 2025)

### Implemented features
- [x] Display CPU temperature
- [x] GTK+ interface with tabs (System, CPU, Memory, Network, Disk)
- [x] Automatic refresh (2 seconds)
- [x] Refresh and Quit buttons
- [x] Modular architecture (MVC)
- [x] Portable `/sys/class/thermal` reading
- [x] `vcgencmd` fallback for Raspberry Pi
- [x] Makefile with incremental build
- [x] README documentation

### System Information (System tab) ✨
- [x] Display hardware model (ARM Device Tree, x86 DMI)
- [x] Processor detection with exact architecture
- [x] GPU identification (NVIDIA, AMD, Intel, Broadcom VideoCore)
- [x] Kernel version
- [x] Distribution and version (`/etc/os-release`)
- [x] Desktop environment detection (GNOME, KDE, XFCE, labwc, etc.)
- [x] Display server (X11 vs Wayland)
- [x] CPU core count
- [x] Locale/Timezone

### CPU Monitoring (CPU tab) ✨
- [x] Real-time temperature (2s) — **Displays both Celsius & Fahrenheit (45.2°C (113.4°F))**
- [x] CPU usage % (delta calculation from `/proc/stat`)
- [x] GPU usage % (multi-method)
- [x] Uptime display

### Memory Monitoring (Memory tab) ✨
- [x] Memory usage percentage
- [x] Available memory (GB)
- [x] Total memory (GB)

### Network (Network tab) ✨
- [x] Hostname
- [x] **Network interfaces with IP address per interface** (new!)
- [x] Interface type detection (Ethernet, WiFi, Mobile)
- [x] Interface filtering (exclude lo, docker, veth, virbr, etc.)
- [x] **Dynamic IP refresh** (every 1 second with DHCP support)
- [x] Upload/download throughput infrastructure

### Disk Monitoring (Disk tab) ✨ **NEW!**
- [x] **Refresh button** — manually reload disk list when new USB/SD card inserted
- [x] **Physical disk identification**
  - [x] Read `/sys/block/` to list disks
  - [x] Filter partitions (ignore sda1, nvme0n1p1)
  - [x] Total capacity in GB
  - [x] Disk model
  - [x] Used/available space per partition

- [x] **Disk type**
  - [x] NVMe detected automatically
  - [x] USB detected via `/sys/block/sd*/device/`
  - [x] SATA/IDE for other disks

- [x] **Interface detection** (new approach!)
  - [x] **NVMe**: read PCIe link via `/sys/block/nvme*/device/device/current_link_speed`
    - PCIe Gen3 (8.0 GT/s)
    - PCIe Gen4 (16.0 GT/s)
    - PCIe Gen5 (32.0 GT/s)
  - [x] **USB**: read actual port speed via `/sys/block/sd*/device/../speed`
    - USB 1.x (1.5 or 12 Mbps)
    - USB 2.0 (480 Mbps)
    - USB 3.0 (5000 Mbps)
    - USB 3.1+ (10000 Mbps+)
  - [x] Display the interface for each disk

- [x] **Speed Test** ✨ **MAJOR FEATURE**
  - [x] "⚡ Speed Test" button to start tests
  - [x] Tests executed in a separate thread (non-blocking)
  - [x] Each disk tested individually
  - [x] **Uses O_DIRECT** to bypass the system cache
  - [x] 512-byte aligned buffer (required by O_DIRECT)
  - [x] Write test: 100 MB with fsync()
  - [x] Read test: 100 MB after clearing cache
  - [x] Automatic fallback if O_DIRECT not supported
  - [x] Results in MB/s per disk
  - [x] Clear distinction NVMe vs USB vs SATA

---

## 🎯 Version 1.1 - Minor polish and missing basics

**Goal**: polish existing features and add small enhancements

### Planned features
- [ ] **Data export**
  - [ ] CSV export button
  - [ ] Format: Timestamp, Temp, CPU%, GPU%, Memory%, Disk usage

- [ ] **Improved GPU detection**
  - [ ] Multi-GPU NVIDIA support
  - [ ] GPU temperature if available
  - [ ] GPU memory usage

- [ ] **Advanced network**
  - [ ] Accurate per-interface upload/download (implement using `/proc/net/dev`)
  - [ ] Live throughput graphs

- [ ] **Advanced disk features**
  - [ ] Disk temperature (S.M.A.R.T. if available)
  - [ ] Read/write IOPS if available
  - [ ] Detect which partitions belong to each disk

**Files to modify**
- `src/system_info.c` — export, advanced network
- `src/gui.c` — new widgets

**Estimate**: 2–3 hours development

---

## 🎯 Version 1.5 - Real-time graphs

**Goal**: add visual graphs to track trends

### Planned features
- [ ] **Cairo graphs**
  - [ ] CPU temperature history (60s)
  - [ ] CPU usage graph (60s)
  - [ ] Memory usage (60s)
  - [ ] GPU usage (60s)

- [ ] **Progress bars**
  - [ ] CPU %
  - [ ] Memory %
  - [ ] GPU %
  - [ ] Disk usage (per partition)

- [ ] **History**
  - [ ] Circular buffer (ring buffer)
  - [ ] Keep last 60 points
  - [ ] Timestamp each point

**Technologies**: Cairo for drawing, ring buffer for history

**Estimate**: 4–6 hours development

---

## 🎯 Version 2.0 - Theming and improved interface

**Goal**: modernize the UI with themes and persistent settings

### Planned features
- [ ] **Themes**
  - [ ] Light mode (default)
  - [ ] Dark mode
  - [ ] Custom GTK CSS

- [ ] **Persistent configuration**
  - [ ] `~/.config/syswatch/config.ini`
  - [ ] Theme choice (light/dark)
  - [ ] Refresh interval (1s, 2s, 5s, 10s)
  - [ ] Units (Celsius/Fahrenheit)
  - [ ] Enable/disable graphs

- [ ] **Preferences menu**
  - [ ] GTK dialog for configuration
  - [ ] Auto-save

**Technologies**
- GKeyFile for INI config
- GTK3 CSS for themes

**Estimate**: 3–4 hours development

---

## 🎯 Version 2.5 - Alerts and notifications

**Goal**: notify users of significant conditions

### Planned features
- [ ] **Configurable thresholds**
  - [ ] Alert if temperature > 80°C
  - [ ] Alert if RAM > 90%
  - [ ] Alert if disk > 90%

- [ ] **Notifications**
  - [ ] Desktop notifications using libnotify
  - [ ] Optional sounds
  - [ ] Alert history

**Estimate**: 2–3 hours development

---

## 🎯 Version 3.0 - Packaging and distribution

**Goal**: make the app easy to install

### Tasks
- [ ] **Debian package (.deb)**
  - [ ] Create `debian/` layout
  - [ ] Build scripts
  - [ ] Install via `apt`

- [ ] **System integration**
  - [ ] Add desktop menu entry (GNOME/KDE)
  - [ ] Add app icon
  - [ ] Provide man page

- [ ] **Complete documentation**
  - [ ] User guide
  - [ ] FAQ
  - [ ] Troubleshooting

**Estimate**: 3–4 hours development

---

## 📊 Priorities

### Short-term (Done ✅)
- ✅ Version 1.0 — Core system
- ✅ Disk detection and speed test

### Mid-term (1–2 months)
1. Version 1.1 — polishing and small features
2. Version 1.5 — real-time graphs
3. Version 2.0 — themes and config

### Long-term (2–3 months)
4. Version 2.5 — alerts
5. Version 3.0 — packaging and distribution

---

## 🌍 Version 3.0+ — MULTILINGUAL SYSTEM (Major future feature)

**Goal**: fully localize the application using a flexible JSON-based system

### Concept: JSON locale files + automatic mapping

**Folder structure:**
```
SysWatch/
├── src/
├── locales/
│   ├── en_US.json
│   ├── fr_FR.json
│   ├── es_ES.json
│   ├── de_DE.json
│   ├── ja_JP.json
│   ├── zh_CN.json
│   └── ru_RU.json
```

A full plan and implementation phases are included here (in `SPECIFICATIONS.md`).

---

**Last update:** 2 December 2025
