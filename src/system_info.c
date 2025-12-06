/*
 * system_info.c
 * System information reading functions implementation
 */

#define _GNU_SOURCE  // Pour strcasestr
#include "system_info.h"
#include "storage_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // Pour gethostname
#include <sys/stat.h>  // Pour lstat et S_ISLNK
#include <time.h>  // Pour clock_gettime
#include <ctype.h>  // Pour isdigit
#include <stdbool.h>  // Pour bool
#include <fcntl.h>  // Pour open
#include <errno.h>  // Pour errno

float get_cpu_temperature_celsius(void) {
    FILE *fp;
    char buffer[128];
    
    // Method 1: Direct read from /sys/class/thermal (universal Linux)
    fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // Temperature is in millidegrees Celsius
            int temp_millidegrees = atoi(buffer);
            float temp_celsius = temp_millidegrees / 1000.0f;
            fclose(fp);
            return temp_celsius;
        }
        fclose(fp);
    }
    
    // Method 2: Fallback to vcgencmd (Raspberry Pi specific)
    fp = popen("vcgencmd measure_temp", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // Format is: temp=XX.X'C
            char *temp_start = strchr(buffer, '=');
            if (temp_start != NULL) {
                temp_start++; // Skip the '='
                float temp = atof(temp_start);
                pclose(fp);
                return temp;
            }
        }
        pclose(fp);
    }
    
    // Error: no method worked
    return -1.0f;
}

bool get_cpu_temperature_string(char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < 64) {
        return false;
    }
    
    float temp_celsius = get_cpu_temperature_celsius();
    
    if (temp_celsius < 0) {
        snprintf(buffer, buffer_size, "Erreur: Impossible de lire la température");
        return false;
    }
    
    // Conversion Celsius vers Fahrenheit: F = (C × 9/5) + 32
    float temp_fahrenheit = (temp_celsius * 9.0f / 5.0f) + 32.0f;
    
    snprintf(buffer, buffer_size, "Température CPU: %.1f°C (%.1f°F)", temp_celsius, temp_fahrenheit);
    return true;
}

// ============================================================================
// FONCTIONS SYSTEM INFO - Lecture informations système
// ============================================================================

const char* get_hardware_model(void) {
    static char hardware_buffer[256] = {0};
    
    // Si déjà lu, retourner le cache
    if (hardware_buffer[0] != '\0') {
        return hardware_buffer;
    }
    
    // Méthode 1: Device Tree (Raspberry Pi, ARM)
    FILE *fp = fopen("/sys/firmware/devicetree/base/model", "r");
    if (fp != NULL) {
        if (fgets(hardware_buffer, sizeof(hardware_buffer), fp) != NULL) {
            // Supprimer le \0 final du device tree
            size_t len = strlen(hardware_buffer);
            if (len > 0 && hardware_buffer[len-1] == '\0') {
                hardware_buffer[len-1] = '\0';
            }
            fclose(fp);
            return hardware_buffer;
        }
        fclose(fp);
    }
    
    // Méthode 2: /proc/cpuinfo (ligne Model)
    fp = fopen("/proc/cpuinfo", "r");
    if (fp != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strncmp(line, "Model", 5) == 0) {
                char *colon = strchr(line, ':');
                if (colon != NULL) {
                    colon++;
                    // Sauter les espaces
                    while (*colon == ' ' || *colon == '\t') colon++;
                    // Copier et supprimer \n
                    strncpy(hardware_buffer, colon, sizeof(hardware_buffer) - 1);
                    char *newline = strchr(hardware_buffer, '\n');
                    if (newline) *newline = '\0';
                    fclose(fp);
                    return hardware_buffer;
                }
            }
        }
        fclose(fp);
    }
    
    // Méthode 3: DMI pour PC x86 (manufacturer + product)
    fp = fopen("/sys/class/dmi/id/product_name", "r");
    if (fp != NULL) {
        if (fgets(hardware_buffer, sizeof(hardware_buffer), fp) != NULL) {
            char *newline = strchr(hardware_buffer, '\n');
            if (newline) *newline = '\0';
            fclose(fp);
            return hardware_buffer;
        }
        fclose(fp);
    }
    
    // Fallback
    strncpy(hardware_buffer, "Unknown Hardware", sizeof(hardware_buffer) - 1);
    return hardware_buffer;
}

const char* get_processor_type(void) {
    static char processor_buffer[256] = {0};
    static char arch_buffer[32] = {0};
    
    // Si déjà lu, retourner le cache
    if (processor_buffer[0] != '\0') {
        return processor_buffer;
    }
    
    // Détecter l'architecture exacte (garder le nom brut pour précision)
    FILE *fp = popen("uname -m 2>/dev/null", "r");
    if (fp != NULL) {
        if (fgets(arch_buffer, sizeof(arch_buffer), fp) != NULL) {
            // Supprimer \n
            char *newline = strchr(arch_buffer, '\n');
            if (newline) *newline = '\0';
            // On garde l'architecture exacte : aarch64, armv7l, x86_64, i686, etc.
        }
        pclose(fp);
    }
    
    // Si architecture non détectée, mettre valeur par défaut
    if (arch_buffer[0] == '\0') {
        strncpy(arch_buffer, "Unknown", sizeof(arch_buffer) - 1);
    }
    
    char cpu_model[256] = {0};
    
    // Méthode 1: Utiliser lscpu (universel, lisible)
    fp = popen("lscpu 2>/dev/null | grep 'Model name:'", "r");
    if (fp != NULL) {
        char line[256];
        if (fgets(line, sizeof(line), fp) != NULL) {
            // Format: "Model name:          Cortex-A76"
            char *colon = strchr(line, ':');
            if (colon != NULL) {
                colon++;
                // Sauter les espaces
                while (*colon == ' ' || *colon == '\t') colon++;
                // Copier et supprimer \n
                strncpy(cpu_model, colon, sizeof(cpu_model) - 1);
                char *newline = strchr(cpu_model, '\n');
                if (newline) *newline = '\0';
                
                // Combiner modèle + architecture
                snprintf(processor_buffer, sizeof(processor_buffer), "%s (%s)", cpu_model, arch_buffer);
                pclose(fp);
                return processor_buffer;
            }
        }
        pclose(fp);
    }
    
    // Méthode 2: /proc/cpuinfo pour x86 (ligne "model name")
    fp = fopen("/proc/cpuinfo", "r");
    if (fp != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon != NULL) {
                    colon++;
                    while (*colon == ' ' || *colon == '\t') colon++;
                    strncpy(cpu_model, colon, sizeof(cpu_model) - 1);
                    char *newline = strchr(cpu_model, '\n');
                    if (newline) *newline = '\0';
                    
                    // Combiner modèle + architecture
                    snprintf(processor_buffer, sizeof(processor_buffer), "%s (%s)", cpu_model, arch_buffer);
                    fclose(fp);
                    return processor_buffer;
                }
            }
        }
        // Méthode 3: Pour ARM, mapper CPU part vers nom lisible
        rewind(fp);
        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strncmp(line, "CPU part", 8) == 0) {
                char *colon = strchr(line, ':');
                if (colon != NULL) {
                    colon++;
                    while (*colon == ' ' || *colon == '\t') colon++;
                    unsigned int cpu_part = 0;
                    if (sscanf(colon, "0x%x", &cpu_part) == 1) {
                        // Mapping ARM CPU parts
                        const char *cpu_name = "Unknown ARM";
                        switch (cpu_part) {
                            case 0xd03: cpu_name = "Cortex-A53"; break;
                            case 0xd04: cpu_name = "Cortex-A35"; break;
                            case 0xd05: cpu_name = "Cortex-A55"; break;
                            case 0xd07: cpu_name = "Cortex-A57"; break;
                            case 0xd08: cpu_name = "Cortex-A72"; break;
                            case 0xd09: cpu_name = "Cortex-A73"; break;
                            case 0xd0a: cpu_name = "Cortex-A75"; break;
                            case 0xd0b: cpu_name = "Cortex-A76"; break;
                            case 0xd0d: cpu_name = "Cortex-A77"; break;
                            case 0xd0e: cpu_name = "Cortex-A76AE"; break;
                            case 0xd40: cpu_name = "Neoverse-V1"; break;
                            case 0xd41: cpu_name = "Cortex-A78"; break;
                            case 0xd44: cpu_name = "Cortex-X1"; break;
                            case 0xd46: cpu_name = "Cortex-A510"; break;
                            case 0xd47: cpu_name = "Cortex-A710"; break;
                            case 0xd48: cpu_name = "Cortex-X2"; break;
                            case 0xd49: cpu_name = "Neoverse-N2"; break;
                            case 0xd4a: cpu_name = "Neoverse-E1"; break;
                            case 0xd4b: cpu_name = "Cortex-A78AE"; break;
                            case 0xd4c: cpu_name = "Cortex-X1C"; break;
                            case 0xd4d: cpu_name = "Cortex-A715"; break;
                            case 0xd4e: cpu_name = "Cortex-X3"; break;
                        }
                        // Combiner modèle + architecture
                        snprintf(processor_buffer, sizeof(processor_buffer), "%s (%s)", cpu_name, arch_buffer);
                        fclose(fp);
                        return processor_buffer;
                    }
                }
            }
        }
        fclose(fp);
    }
    
    // Fallback avec architecture
    snprintf(processor_buffer, sizeof(processor_buffer), "Unknown Processor (%s)", arch_buffer);
    return processor_buffer;
}

const char* get_gpu_info(void) {
    static char gpu_buffer[256] = {0};
    
    // Si déjà lu, retourner le cache
    if (gpu_buffer[0] != '\0') {
        return gpu_buffer;
    }
    
    // Méthode 1: Utiliser lspci pour détecter GPU dédié/intégré (PC)
    FILE *fp = popen("lspci 2>/dev/null | grep -iE 'vga|3d|display'", "r");
    if (fp != NULL) {
        char line[512];
        if (fgets(line, sizeof(line), fp) != NULL) {
            // Format typique: "01:00.0 VGA compatible controller: NVIDIA Corporation GP104 [GeForce GTX 1080]"
            // Chercher le ':' après le type de contrôleur
            char *controller_start = strstr(line, "controller:");
            if (controller_start != NULL) {
                controller_start += 12; // Sauter "controller: "
                while (*controller_start == ' ') controller_start++;
                
                // Nettoyer la ligne
                strncpy(gpu_buffer, controller_start, sizeof(gpu_buffer) - 1);
                char *newline = strchr(gpu_buffer, '\n');
                if (newline) *newline = '\0';
                
                // Simplifier si entre crochets (ex: "NVIDIA ... [GeForce GTX 1080]")
                char *bracket_open = strchr(gpu_buffer, '[');
                char *bracket_close = strchr(gpu_buffer, ']');
                if (bracket_open && bracket_close) {
                    // Extraire le contenu entre crochets
                    bracket_open++;
                    *bracket_close = '\0';
                    
                    // Garder fabricant (premier mot)
                    char vendor[64] = {0};
                    char *space = strchr(gpu_buffer, ' ');
                    if (space) {
                        size_t vendor_len = space - gpu_buffer;
                        if (vendor_len < sizeof(vendor)) {
                            strncpy(vendor, gpu_buffer, vendor_len);
                            vendor[vendor_len] = '\0';
                        }
                    }
                    
                    // Combiner: "Fabricant Modèle"
                    if (vendor[0] != '\0') {
                        snprintf(gpu_buffer, sizeof(gpu_buffer), "%s %s", vendor, bracket_open);
                    } else {
                        strncpy(gpu_buffer, bracket_open, sizeof(gpu_buffer) - 1);
                    }
                }
                
                pclose(fp);
                return gpu_buffer;
            }
        }
        pclose(fp);
    }
    
    // Méthode 2: Essayer sysfs pour GPU (Linux moderne)
    fp = fopen("/sys/class/drm/card0/device/vendor", "r");
    if (fp != NULL) {
        char vendor_id[16] = {0};
        if (fgets(vendor_id, sizeof(vendor_id), fp) != NULL) {
            unsigned int vendor = 0;
            sscanf(vendor_id, "0x%x", &vendor);
            
            const char *vendor_name = "Unknown";
            switch (vendor) {
                case 0x10de: vendor_name = "NVIDIA"; break;
                case 0x1002: vendor_name = "AMD Radeon"; break;
                case 0x8086: vendor_name = "Intel"; break;
                case 0x14e4: vendor_name = "Broadcom"; break;
            }
            
            strncpy(gpu_buffer, vendor_name, sizeof(gpu_buffer) - 1);
            fclose(fp);
            return gpu_buffer;
        }
        fclose(fp);
    }
    
    // Méthode 3: Détecter GPU Raspberry Pi via hardware model
    const char *hardware = get_hardware_model();
    if (strstr(hardware, "Raspberry Pi 5") || strstr(hardware, "Raspberry Pi 500")) {
        strncpy(gpu_buffer, "Broadcom VideoCore VII", sizeof(gpu_buffer) - 1);
        return gpu_buffer;
    } else if (strstr(hardware, "Raspberry Pi 4")) {
        strncpy(gpu_buffer, "Broadcom VideoCore VI", sizeof(gpu_buffer) - 1);
        return gpu_buffer;
    } else if (strstr(hardware, "Raspberry Pi")) {
        strncpy(gpu_buffer, "Broadcom VideoCore IV", sizeof(gpu_buffer) - 1);
        return gpu_buffer;
    }
    
    // Fallback
    strncpy(gpu_buffer, "Unknown GPU", sizeof(gpu_buffer) - 1);
    return gpu_buffer;
}

const char* get_kernel_version(void) {
    static char kernel_buffer[256] = {0};
    
    // Si déjà lu, retourner le cache
    if (kernel_buffer[0] != '\0') {
        return kernel_buffer;
    }
    
    // Lire depuis /proc/version
    FILE *fp = fopen("/proc/version", "r");
    if (fp != NULL) {
        if (fgets(kernel_buffer, sizeof(kernel_buffer), fp) != NULL) {
            // Format: "Linux version 6.6.51+rpt-rpi-2712 ..."
            // Extraire juste la version
            char *version_start = strstr(kernel_buffer, "version ");
            if (version_start != NULL) {
                version_start += 8; // Sauter "version "
                char *space = strchr(version_start, ' ');
                if (space != NULL) {
                    *space = '\0'; // Terminer à l'espace
                    memmove(kernel_buffer, version_start, strlen(version_start) + 1);
                }
            }
        }
        fclose(fp);
    }
    
    // Fallback si échec
    if (kernel_buffer[0] == '\0') {
        strncpy(kernel_buffer, "Unknown", sizeof(kernel_buffer) - 1);
    }
    
    return kernel_buffer;
}

const char* get_distro_info(void) {
    static char distro_buffer[256] = {0};
    
    // Si déjà lu, retourner le cache
    if (distro_buffer[0] != '\0') {
        return distro_buffer;
    }
    
    // Lire PRETTY_NAME depuis /etc/os-release
    FILE *fp = fopen("/etc/os-release", "r");
    if (fp != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                // Format: PRETTY_NAME="Debian GNU/Linux 13 (trixie)"
                char *start = strchr(line, '"');
                if (start != NULL) {
                    start++; // Sauter le premier guillemet
                    char *end = strchr(start, '"');
                    if (end != NULL) {
                        *end = '\0';
                        strncpy(distro_buffer, start, sizeof(distro_buffer) - 1);
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }
    
    // Fallback si échec
    if (distro_buffer[0] == '\0') {
        strncpy(distro_buffer, "Unknown", sizeof(distro_buffer) - 1);
    }
    
    return distro_buffer;
}

const char* get_architecture_info(void) {
    static char arch_buffer[64] = {0};
    
    // Si déjà lu, retourner le cache
    if (arch_buffer[0] != '\0') {
        return arch_buffer;
    }
    
    // Récupérer l'architecture via uname
    FILE *fp = popen("uname -m 2>/dev/null", "r");
    if (fp != NULL) {
        char arch[32] = {0};
        if (fgets(arch, sizeof(arch), fp) != NULL) {
            char *newline = strchr(arch, '\n');
            if (newline) *newline = '\0';
            
            // Déterminer le type (32-bit ou 64-bit)
            const char *bitness = "Unknown";
            if (strcmp(arch, "aarch64") == 0 || strcmp(arch, "x86_64") == 0 || 
                strcmp(arch, "ppc64") == 0 || strcmp(arch, "ppc64le") == 0 ||
                strcmp(arch, "s390x") == 0) {
                bitness = "64-bit";
            } else if (strcmp(arch, "armv7l") == 0 || strcmp(arch, "armv6l") == 0 ||
                       strcmp(arch, "i386") == 0 || strcmp(arch, "i686") == 0) {
                bitness = "32-bit";
            }
            
            // Noms lisibles pour les architectures
            const char *arch_name = arch;  // Par défaut, nom brut
            if (strcmp(arch, "aarch64") == 0) {
                arch_name = "ARM 64-bit";
            } else if (strcmp(arch, "armv7l") == 0) {
                arch_name = "ARM 32-bit";
            } else if (strcmp(arch, "armv6l") == 0) {
                arch_name = "ARM 32-bit";
            } else if (strcmp(arch, "x86_64") == 0) {
                arch_name = "x86 64-bit";
            } else if (strcmp(arch, "i386") == 0 || strcmp(arch, "i686") == 0) {
                arch_name = "x86 32-bit";
            } else {
                // Pour les architectures non reconnues, afficher le nom brut avec le type de bit
                snprintf(arch_buffer, sizeof(arch_buffer), "%s (%s)", arch, bitness);
                pclose(fp);
                return arch_buffer;
            }
            
            snprintf(arch_buffer, sizeof(arch_buffer), "%s", arch_name);
        }
        pclose(fp);
    }
    
    // Fallback
    if (arch_buffer[0] == '\0') {
        strncpy(arch_buffer, "Unknown", sizeof(arch_buffer) - 1);
    }
    
    return arch_buffer;
}

const char* get_cpu_cores(void) {
    static char cores_buffer[64] = {0};
    
    // Si déjà lu, retourner le cache
    if (cores_buffer[0] != '\0') {
        return cores_buffer;
    }
    
    // Méthode 1: Utiliser nproc (le plus rapide et universellement disponible)
    FILE *fp = popen("nproc 2>/dev/null", "r");
    if (fp != NULL) {
        char line[16];
        if (fgets(line, sizeof(line), fp) != NULL) {
            int cores = atoi(line);
            pclose(fp);
            if (cores > 0) {
                snprintf(cores_buffer, sizeof(cores_buffer), "%d", cores);
                return cores_buffer;
            }
        }
        pclose(fp);
    }
    
    // Méthode 2: Compter les lignes "processor" dans /proc/cpuinfo
    fp = fopen("/proc/cpuinfo", "r");
    if (fp != NULL) {
        int cores = 0;
        char line[256];
        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strncmp(line, "processor", 9) == 0) {
                cores++;
            }
        }
        fclose(fp);
        if (cores > 0) {
            snprintf(cores_buffer, sizeof(cores_buffer), "%d", cores);
            return cores_buffer;
        }
    }
    
    // Fallback
    strncpy(cores_buffer, "Unknown", sizeof(cores_buffer) - 1);
    return cores_buffer;
}

const char* get_uptime_string(void) {
    static char uptime_buffer[128] = {0};
    
    FILE *fp = fopen("/proc/uptime", "r");
    if (fp != NULL) {
        double uptime_seconds_double;
        if (fscanf(fp, "%lf", &uptime_seconds_double) == 1) {
            fclose(fp);
            
            unsigned long uptime_seconds = (unsigned long)uptime_seconds_double;
            unsigned long days = uptime_seconds / (24 * 3600);
            unsigned long hours = (uptime_seconds % (24 * 3600)) / 3600;
            unsigned long minutes = (uptime_seconds % 3600) / 60;
            unsigned long secs = uptime_seconds % 60;
            
            if (days > 0) {
                snprintf(uptime_buffer, sizeof(uptime_buffer), 
                         "%lu day%s, %lu hour%s",
                         days, days > 1 ? "s" : "",
                         hours, hours > 1 ? "s" : "");
            } else if (hours > 0) {
                snprintf(uptime_buffer, sizeof(uptime_buffer), 
                         "%lu hour%s, %lu minute%s",
                         hours, hours > 1 ? "s" : "",
                         minutes, minutes > 1 ? "s" : "");
            } else if (minutes > 0) {
                snprintf(uptime_buffer, sizeof(uptime_buffer), 
                         "%lu minute%s, %lu second%s",
                         minutes, minutes > 1 ? "s" : "",
                         secs, secs > 1 ? "s" : "");
            } else {
                snprintf(uptime_buffer, sizeof(uptime_buffer), 
                         "%lu second%s",
                         secs, secs > 1 ? "s" : "");
            }
            return uptime_buffer;
        }
        fclose(fp);
    }
    
    // Fallback
    strncpy(uptime_buffer, "Unknown", sizeof(uptime_buffer) - 1);
    return uptime_buffer;
}

const char* get_desktop_environment(void) {
    static char desktop_buffer[256] = {0};
    
    // Si déjà lu, retourner le cache
    if (desktop_buffer[0] != '\0') {
        return desktop_buffer;
    }
    
    char desktop_name[128] = {0};
    char display_server[32] = {0};
    
    // Détecter le serveur d'affichage (Wayland ou X11)
    const char* session_type = getenv("XDG_SESSION_TYPE");
    if (session_type != NULL && strcmp(session_type, "wayland") == 0) {
        strncpy(display_server, "Wayland", sizeof(display_server) - 1);
    } else if (session_type != NULL && strcmp(session_type, "x11") == 0) {
        strncpy(display_server, "X11", sizeof(display_server) - 1);
    } else {
        // Fallback: vérifier WAYLAND_DISPLAY ou DISPLAY
        const char* wayland_display = getenv("WAYLAND_DISPLAY");
        const char* x_display = getenv("DISPLAY");
        
        if (wayland_display != NULL && wayland_display[0] != '\0') {
            strncpy(display_server, "Wayland", sizeof(display_server) - 1);
        } else if (x_display != NULL && x_display[0] != '\0') {
            strncpy(display_server, "X11", sizeof(display_server) - 1);
        } else {
            strncpy(display_server, "Unknown", sizeof(display_server) - 1);
        }
    }
    
    // Méthode 1: Variables d'environnement standard pour le DE
    const char* de_name = getenv("XDG_CURRENT_DESKTOP");
    if (de_name == NULL || de_name[0] == '\0') {
        de_name = getenv("DESKTOP_SESSION");
    }
    
    // Méthode 2: Détection par processus en cours (si variables vides)
    if (de_name == NULL || de_name[0] == '\0') {
        FILE *fp = popen("ps aux 2>/dev/null", "r");
        if (fp != NULL) {
            char line[512];
            while (fgets(line, sizeof(line), fp) != NULL) {
                if (strstr(line, "gnome-shell")) { de_name = "GNOME"; break; }
                if (strstr(line, "plasmashell")) { de_name = "KDE Plasma"; break; }
                if (strstr(line, "xfce4-session")) { de_name = "XFCE"; break; }
                if (strstr(line, "mate-session")) { de_name = "MATE"; break; }
                if (strstr(line, "cinnamon-session")) { de_name = "Cinnamon"; break; }
                if (strstr(line, "lxsession")) { de_name = "LXDE"; break; }
                if (strstr(line, "/usr/bin/labwc")) { de_name = "labwc"; break; }
                if (strstr(line, "wayfire")) { de_name = "Wayfire"; break; }
                if (strstr(line, "sway")) { de_name = "Sway"; break; }
            }
            pclose(fp);
        }
    }
    
    if (de_name == NULL || de_name[0] == '\0') {
        de_name = "Unknown";
    }
    
    // Copier le nom de base
    strncpy(desktop_name, de_name, sizeof(desktop_name) - 1);
    
    // Essayer d'obtenir la version selon l'environnement
    FILE *fp = NULL;
    
    if (strcasestr(de_name, "gnome") != NULL) {
        fp = popen("gnome-shell --version 2>/dev/null", "r");
    } else if (strcasestr(de_name, "xfce") != NULL) {
        fp = popen("xfce4-session --version 2>/dev/null | head -n1", "r");
    } else if (strcasestr(de_name, "kde") != NULL || strcasestr(de_name, "plasma") != NULL) {
        fp = popen("plasmashell --version 2>/dev/null", "r");
    } else if (strcasestr(de_name, "mate") != NULL) {
        fp = popen("mate-session --version 2>/dev/null", "r");
    } else if (strcasestr(de_name, "cinnamon") != NULL) {
        fp = popen("cinnamon --version 2>/dev/null", "r");
    } else if (strcasestr(de_name, "labwc") != NULL) {
        fp = popen("labwc --version 2>/dev/null | head -n1", "r");
    } else if (strcasestr(de_name, "wayfire") != NULL) {
        fp = popen("wayfire --version 2>/dev/null", "r");
    }
    
    if (fp != NULL) {
        char version_line[256];
        if (fgets(version_line, sizeof(version_line), fp) != NULL) {
            // Extraire la version (dernier nombre dans la ligne)
            char *ver = strrchr(version_line, ' ');
            if (ver != NULL) {
                ver++;
                char *newline = strchr(ver, '\n');
                if (newline) *newline = '\0';
                // Si c'est un nombre valide, ajouter la version
                if (ver[0] >= '0' && ver[0] <= '9') {
                    snprintf(desktop_name, sizeof(desktop_name), "%s %s", de_name, ver);
                }
            }
        }
        pclose(fp);
    }
    
    // Combiner Desktop Environment et Display Server
    snprintf(desktop_buffer, sizeof(desktop_buffer), "%s / %s", desktop_name, display_server);
    
    return desktop_buffer;
}

const char* get_locale_info(void) {
    static char locale_buffer[64] = {0};
    
    // Si déjà lu, retourner le cache
    if (locale_buffer[0] != '\0') {
        return locale_buffer;
    }
    
    // Méthode 1: Variable d'environnement LC_ALL
    const char* locale = getenv("LC_ALL");
    if (locale != NULL && locale[0] != '\0') {
        strncpy(locale_buffer, locale, sizeof(locale_buffer) - 1);
        return locale_buffer;
    }
    
    // Méthode 2: Variable d'environnement LANG
    locale = getenv("LANG");
    if (locale != NULL && locale[0] != '\0') {
        strncpy(locale_buffer, locale, sizeof(locale_buffer) - 1);
        return locale_buffer;
    }
    
    // Méthode 3: Exécuter la commande locale pour obtenir la locale courante
    FILE *fp = popen("locale | grep '^LANG=' | cut -d= -f2 | tr -d '\"'", "r");
    if (fp != NULL) {
        if (fgets(locale_buffer, sizeof(locale_buffer), fp) != NULL) {
            // Supprimer la newline
            size_t len = strlen(locale_buffer);
            if (len > 0 && locale_buffer[len-1] == '\n') {
                locale_buffer[len-1] = '\0';
            }
            pclose(fp);
            if (locale_buffer[0] != '\0') {
                return locale_buffer;
            }
        }
        pclose(fp);
    }
    
    // Fallback
    strncpy(locale_buffer, "Unknown", sizeof(locale_buffer) - 1);
    return locale_buffer;
}

float get_cpu_usage_percent(void) {
    static unsigned long long prev_idle = 0, prev_total = 0;
    unsigned long long idle, total;
    unsigned long long user, nice, system, idle_time, iowait, irq, softirq, steal;
    
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        return -1.0f;
    }
    
    // Lecture de la ligne "cpu" : user nice system idle iowait irq softirq steal
    if (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal) != 8) {
        fclose(fp);
        return -1.0f;
    }
    fclose(fp);
    
    // Calcul du temps total et idle
    idle = idle_time + iowait;
    total = user + nice + system + idle_time + iowait + irq + softirq + steal;
    
    // Première lecture : initialiser les valeurs précédentes
    if (prev_total == 0) {
        prev_idle = idle;
        prev_total = total;
        return 0.0f;  // Pas de données pour calculer le %
    }
    
    // Calcul du delta
    unsigned long long total_diff = total - prev_total;
    unsigned long long idle_diff = idle - prev_idle;
    
    // Sauvegarder pour la prochaine fois
    prev_idle = idle;
    prev_total = total;
    
    // Calculer le pourcentage d'utilisation
    if (total_diff == 0) {
        return 0.0f;
    }
    
    float usage = 100.0f * (float)(total_diff - idle_diff) / (float)total_diff;
    return usage;
}

float get_gpu_usage_percent(void) {
    FILE *fp;
    char buffer[256];
    int i;
    
    // ===== MÉTHODE 1: NVIDIA GPU (nvidia-smi) =====
    fp = popen("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            pclose(fp);
            float usage = atof(buffer);
            return usage;
        }
        pclose(fp);
    }
    
    // ===== MÉTHODE 2: AMD GPU (sysfs) =====
    // Vérifier card0, card1, card2...
    for (i = 0; i < 4; i++) {
        char path[256];
        
        // AMD: gpu_busy_percent
        snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/gpu_busy_percent", i);
        fp = fopen(path, "r");
        if (fp != NULL) {
            unsigned int usage;
            if (fscanf(fp, "%u", &usage) == 1) {
                fclose(fp);
                return (float)usage;
            }
            fclose(fp);
        }
        
        // AMD alternative: utilization
        snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/utilization", i);
        fp = fopen(path, "r");
        if (fp != NULL) {
            unsigned int usage;
            if (fscanf(fp, "%u", &usage) == 1) {
                fclose(fp);
                return (float)usage;
            }
            fclose(fp);
        }
    }
    
    // ===== MÉTHODE 3: INTEL GPU (sysfs) =====
    for (i = 0; i < 4; i++) {
        char path[256];
        snprintf(path, sizeof(path), "/sys/class/drm/card%d/gt/gt0/rps_cur_freq_mhz", i);
        
        fp = fopen(path, "r");
        if (fp != NULL) {
            unsigned int cur_freq, max_freq;
            if (fscanf(fp, "%u", &cur_freq) == 1) {
                fclose(fp);
                
                // Lire fréquence max
                snprintf(path, sizeof(path), "/sys/class/drm/card%d/gt/gt0/rps_max_freq_mhz", i);
                fp = fopen(path, "r");
                if (fp != NULL && fscanf(fp, "%u", &max_freq) == 1) {
                    fclose(fp);
                    if (max_freq > 0) {
                        float usage = 100.0f * (float)cur_freq / (float)max_freq;
                        return usage > 100.0f ? 100.0f : usage;
                    }
                }
                if (fp != NULL) fclose(fp);
            } else {
                fclose(fp);
            }
        }
    }
    
    // ===== MÉTHODE 4: RASPBERRY PI (vcgencmd) =====
    fp = popen("vcgencmd measure_clock core 2>/dev/null", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // Format: frequency(45)=500000000
            char *freq_str = strchr(buffer, '=');
            if (freq_str != NULL) {
                freq_str++;
                unsigned long current_freq = strtoul(freq_str, NULL, 10);
                pclose(fp);
                
                // Lire la fréquence maximale
                fp = popen("vcgencmd get_config core_freq 2>/dev/null", "r");
                if (fp != NULL && fgets(buffer, sizeof(buffer), fp) != NULL) {
                    // Format: core_freq=500
                    char *max_str = strchr(buffer, '=');
                    if (max_str != NULL) {
                        max_str++;
                        unsigned long max_freq_mhz = strtoul(max_str, NULL, 10);
                        unsigned long max_freq = max_freq_mhz * 1000000;
                        pclose(fp);
                        
                        if (max_freq > 0) {
                            float usage = 100.0f * (float)current_freq / (float)max_freq;
                            return usage > 100.0f ? 100.0f : usage;
                        }
                    }
                    pclose(fp);
                }
            }
        }
        pclose(fp);
    }
    
    // GPU usage non disponible ou GPU pas supporté
    return 0.0f;
}

float get_memory_usage_percent(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        return -1.0f;
    }
    
    unsigned long mem_total = 0;
    unsigned long mem_available = 0;
    char line[256];
    
    // Lire MemTotal et MemAvailable
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) {
            continue;
        }
        if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1) {
            break;  // On a tout ce qu'il faut
        }
    }
    fclose(fp);
    
    if (mem_total == 0) {
        return -1.0f;
    }
    
    // Calculer le pourcentage utilisé
    unsigned long mem_used = mem_total - mem_available;
    float usage = 100.0f * (float)mem_used / (float)mem_total;
    
    return usage;
}

float get_memory_available_gb(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        return -1.0f;
    }
    
    unsigned long mem_available_kb = 0;
    char line[256];
    
    // Lire MemAvailable
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "MemAvailable: %lu kB", &mem_available_kb) == 1) {
            break;
        }
    }
    fclose(fp);
    
    if (mem_available_kb == 0) {
        return -1.0f;
    }
    
    // Convertir KB en GB
    float mem_available_gb = (float)mem_available_kb / (1024.0f * 1024.0f);
    
    return mem_available_gb;
}

float get_memory_total_gb(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        return -1.0f;
    }
    
    unsigned long mem_total_kb = 0;
    char line[256];
    
    // Lire MemTotal
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "MemTotal: %lu kB", &mem_total_kb) == 1) {
            break;
        }
    }
    fclose(fp);
    
    if (mem_total_kb == 0) {
        return -1.0f;
    }
    
    // Convertir KB en GB (1 GB = 1024 * 1024 KB)
    float mem_total_gb = (float)mem_total_kb / (1024.0f * 1024.0f);
    
    return mem_total_gb;
}

