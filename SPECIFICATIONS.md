# SysWatch — Technical Specifications
**Version:** 1.0  
**Date:** 2 December 2025  
**Author:** SysWatch Project  
**Status:** Active development

---

## 1. Overview

### 1.1 Purpose
SysWatch is a lightweight real-time system monitor designed for Raspberry Pi and portable to modern Linux distributions. The application displays hardware, system and performance information in a simple responsive GTK GUI.

### 1.2 Design philosophy
- **Lightweight**: minimal memory footprint, no heavy dependencies
- **Portable**: runs on x86, x64, ARM, ARM64, RISC-V
- **Clear**: simple interface with essential information at a glance
- **Performant**: real-time updates with minimal resource impact

### 1.3 Target audience
- Raspberry Pi users who want quick system status
- New orcasual linux users
- Linux system administrators looking for a lightweight companion tool
- Developers testing applications on multiple architectures

---

## 2. System architecture

### 2.1 Modular structure (MVC-like)

```
SysWatch/
├── src/
│   ├── main.c          # Entry point, GTK init
│   ├── gui.c           # View: GTK UI
│   └── system_info.c   # Model: system data collection
├── include/
│   ├── gui.h           # GUI API
│   └── system_info.h   # System info API
├── obj/                # Compiled object files (.o)
├── Makefile            # Build system
├── README.md           # User documentation
├── ROADMAP.md          # Planned evolution
└── SPECIFICATIONS.md   # This file
```

### 2.2 Tech stack

| Component | Technology | Version | Rationale |
|-----------|------------|---------|-----------|
| Language | C | C11 | Performance, portability, low-level access |
| GUI Framework | GTK+ | 3.0+ | Mature, lightweight, widely available |
| Build system | GNU Make | 4.0+ | Simple and standard |
| Compiler | GCC | 8.0+ | C11 support, ARM optimizations |
| Debugger | GDB | 8.0+ | VS Code integration |

### 2.3 Dependencies

**Required:**
- `libgtk-3-dev` (GTK+ 3)
- `pkg-config`

**Optional (advanced features):**
- `nvidia-smi` (NVIDIA GPU info)
- `vcgencmd` (Raspberry Pi GPU and clocks)
- `radeontop` (AMD monitoring alternative)

---

## 3. Functional specifications

### 3.1 User interface

#### 3.1.1 Main window
- **Initial size**: 700×600
- **Centered** on screen
- **Resizable**
- **Layout**: several sections + button bar
- **Refresh**: automatic (dynamic sections) and manual

#### 3.1.2 Section: System (50% width left)

| Field | Description | Data source | Refresh |
|-------|-------------|-------------|---------|
| Kernel | Linux kernel version | `/proc/version` | Manual only |
| Distribution | Name and version | `/etc/os-release` (PRETTY_NAME) | Manual only |
| Desktop environment | Desktop env + display server | `$XDG_CURRENT_DESKTOP`, `$DESKTOP_SESSION`, process detection, `$XDG_SESSION_TYPE`/`$WAYLAND_DISPLAY` | Manual only |

Layout uses a 2-column grid (label left | value right), column spacing 60px, row spacing 5px.

#### 3.1.3 Hardware information (50% width right)

| Field | Desc. | Source | Refresh | Status |
|-------|-------|--------|---------|--------|
| System Model | Device model / motherboard | `/sys/firmware/devicetree/base/model`, `/proc/cpuinfo`, `/sys/class/dmi/id/product_name` | Manual | ✅ Implemented |
| Processor Type | CPU model + architecture | `lscpu`, `uname -m`, `/proc/cpuinfo` | Manual | ✅ Implemented |
| GPU | Vendor and model | `lspci`, `/sys/class/drm/card*/device/`, Raspberry Pi detection | Manual | ✅ Implemented |

Cached at startup and on manual refresh to avoid repeated reads.

#### 3.1.4 CPU section (50% width, left)

| Field | Unit | Source | Refresh |
|-------|------|--------|---------|
| Temperature | °C / °F | `/sys/class/thermal/thermal_zone0/temp`, fallback `vcgencmd` | Auto 1s |
| CPU usage | % | `/proc/stat` (delta) | Auto 1s |
| GPU usage | % | see GPU detection methods | Auto 1s |

#### 3.1.5 Memory (50% width, right)

| Field | Unit | Source | Refresh |
|-------|------|--------|---------|
| Used | % | `/proc/meminfo` | Auto 1s |
| Available | GB | `/proc/meminfo (MemAvailable)` | Auto 1s |
| Total | GB | `/proc/meminfo (MemTotal)` | Auto 1s |

Calculations use `MemAvailable` for a realistic available memory figure.

#### 3.1.6 Network section (full width)

| Field | Description | Data source | Refresh |
|-------|-------------|-------------|---------|
| Hostname | System hostname | `/etc/hostname` | Manual only |
| Interface table | Per-interface data | `/sys/class/net/` | Auto 1s |
| → Interface name (type) | Network card name and type | `/sys/class/net/` detection | Auto 1s |
| → IP address | IPv4 address assigned to interface | `ip addr show <iface>` | **Auto 1s (new!)** |
| → Upload | Current upload speed | `/proc/net/dev` (delta) | Auto 1s |
| → Download | Current download speed | `/proc/net/dev` (delta) | Auto 1s |

#### 3.1.7 Disk section (full width)

Features include listing physical disks, types, capacities, used/free, detected interface (PCIe Gen, USB speed) and an on-demand speed test invoked by the ⚡ button.

**Buttons:**
- **Refresh 🔄** — reload the disk list (useful when connecting USB devices, SD cards, or external drives)
- **Speed Test ⚡** — on-demand disk speed test

#### 3.1.8 Button bar (global)

| Button | Location | Action |
|--------|----------|--------|
| Refresh 🔄 | Storage section (left) | Reload disk list from `/sys/block/` |
| Speed Test ⚡ | Storage section (right) | Start disk speed tests in background |
| About ℹ️ | Bottom left | Show version and credits |
| Quit ❌ | Bottom right | Exit the application |

---

## 3.2 Detailed technical specs

### 3.2.1 Hardware detection algorithm

Steps:
1. Try `/sys/firmware/devicetree/base/model` (ARM/RPi)
2. If failing, parse `/proc/cpuinfo` for Model
3. If failing, read `/sys/class/dmi/id/product_name` (x86)
4. Fallback: "Unknown Hardware"

Implemented as `const char* get_hardware_model(void)` in `system_info.c`.

### 3.2.2 Processor detection

Return formats such as "Cortex-A76 (aarch64)" or "Intel Core i7-8550U (x86_64)".
Algorithm:
1. Get raw architecture with `uname -m` (aarch64, armv7l, x86_64, etc.)
2. Try `lscpu` for model name
3. Fallback to `/proc/cpuinfo` for x86 or map ARM CPU part codes
4. Combine model + architecture

Implemented as `const char* get_processor_type(void)`.

### 3.2.3 GPU detection

Cascading approach:
1. `lspci` on x86 to parse VGA/3D controller lines
2. `/sys/class/drm/card*/device/vendor` to infer vendor if `lspci` missing
3. Raspberry Pi detection by hardware model for Broadcom VideoCore
4. Fallback to "Unknown GPU"

Implemented as `const char* get_gpu_info(void)`.

### 3.2.4 Kernel

Read `/proc/version`, extract the version token after "version".

### 3.2.5 GPU usage

Cascading methods implemented:
- NVIDIA: `nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits`
- AMD: read `/sys/class/drm/card*/device/gpu_busy_percent`
- Intel: read current and max GPU frequency via sysfs and compute %
- Raspberry Pi: use `vcgencmd` frequencies
- Fallback: return 0.0f if unavailable

Function: `float get_gpu_usage_percent(void)`.

### 3.2.6 Network interface IP address (NEW)

For each network interface, fetch its IPv4 address dynamically using:
```bash
ip addr show <interface_name> | grep 'inet ' | awk '{print $2}' | cut -d/ -f1
```

Implemented as `const char* get_interface_ip_address(const char *interface_name)` with caching to avoid excessive command calls.

**Refresh rate:** Every 1 second (automatic)
**Usefulness:** Detects DHCP renewals, VPN connections, network changes

### 3.2.7 Physical disk detection and refresh (UPDATED)

Read `/sys/block/`, filter partitions and non-disk devices, then for each disk read size, model, mount points, and compute used space using `/proc/mounts` and `statvfs()`.

**Manual refresh:** User clicks the **Refresh 🔄** button in the Storage section to reload the disk list. This is useful for:
- USB keys freshly inserted
- SD cards connected
- External drives attached
- Network mounts added

Data structure `PhysicalStorage` holds name, type, interface, model, capacity_gb, used_gb, available_gb.

Disk interface detection:
- NVMe: parse `/sys/block/nvme*/device/device/current_link_speed` (GT/s)
- USB: read `/sys/block/sdX/device/../speed` (Mbps)

### 3.2.8 Disk speed test (O_DIRECT)

Objectives: accurate read/write measurements per disk avoiding page cache effects.

Approach:
- Open test file on disk mount point with `O_DIRECT | O_SYNC` if possible
- Use `posix_memalign` to get 512-byte aligned buffer
- Write 100MB synchronously with `fsync()`
- Flush and drop caches (if running as root) then read 100MB
- Fallback if `O_DIRECT` not supported

Function: `void get_disk_speed_test(const char *disk_name, float *read_mbps, float *write_mbps)`

### 3.2.9 CPU usage

Read first fields of `/proc/stat` for cumulative counters. Keep previous total and idle to compute delta on next call. First call returns 0%.

Function: `float get_cpu_usage_percent(void)`.

### 3.2.10 Memory reading

Read `MemTotal` and `MemAvailable` from `/proc/meminfo`. Compute used % and convert KB to GB for human display.

Functions: `get_memory_*()` family.

### 3.2.11 Celsius/Fahrenheit conversion

Display both units for wider accessibility. Formula: `°F = (°C × 9/5) + 32`.


---

## 3.3 GTK interface specifications

### 3.3.1 Widgets used

| Widget | Usage |
|--------|-------|
| GtkWindow | Main window |
| GtkNotebook | Tabs |
| GtkBox (VBox/HBox) | Layout containers |
| GtkFrame | Titled frames |
| GtkGrid | Two-column layout |
| GtkLabel | Text display |
| GtkButton | Buttons (Refresh, Quit, Speed Test) |

### 3.3.2 Responsive layout

Use `gtk_widget_set_hexpand(TRUE)` on value widgets to keep right alignment while resizing.

### 3.3.3 Events

- Refresh button: calls `update_system_info_display()` + `update_all_displays()`
- Quit button: `gtk_main_quit()`
- Window destroy: `gtk_main_quit()`
- Timer (1s): calls `update_all_displays()`

---

## 4. Portability

### 4.1 Architectures

| Architecture | Status |
|--------------|--------|
| ARM (32-bit) | ✅ Tested |
| ARM64 | ✅ Tested |
| x86 (32-bit) | ⚠️ Untested but expected to work |
| x64 | ✅ Supported |
| RISC-V | ⚠️ Untested |

### 4.2 Tested distributions

Raspberry Pi OS (Bookworm, Trixie), Debian, Ubuntu. Fedora and Arch not extensively tested but GTK available.

---

## 5. Build & install

### 5.1 Prerequisites

Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential libgtk-3-dev pkg-config
```

Fedora:
```bash
sudo dnf install gcc make gtk3-devel pkg-config
```

Arch:
```bash
sudo pacman -S base-devel gtk3 pkg-config
```

### 5.2 Build

```bash
cd SysWatch
make clean
make
```

### 5.3 Run

```bash
./syswatch
# or
make run
```

---

## 6. Tests and validation

Unit and integration checks were performed for core functions: temperature reading, CPU usage delta, GPU usage multi-method, memory readings, physical disk detection and disk speed test. Compatibility tests were run on Raspberry Pi OS and Debian-based systems.

---

## 7. Limitations

- `O_DIRECT` may fail on some file systems (NFS, CIFS); the code falls back gracefully
- Dropping caches requires root privileges to fully clear page cache
- GPU metrics on some platforms are approximate (e.g. RPi uses frequency)
- First CPU reading returns 0% by design and corrects on subsequent reads

---

## 8. Summary of implemented features

Complete system info, CPU/GPU monitoring, memory, network basics, and robust disk detection and speed testing. Multilingual support is planned as the next major feature.

---

**Last update:** 3 December 2025
