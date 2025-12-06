/*
 * network_info.c
 * Network information reading functions implementation
 */

#include "network_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>

const char* get_hostname(void) {
    static char hostname_buffer[256] = {0};
    
    // If already read, return cached value
    if (hostname_buffer[0] != '\0') {
        return hostname_buffer;
    }
    
    // Read hostname from /etc/hostname
    FILE *fp = fopen("/etc/hostname", "r");
    if (fp != NULL) {
        if (fgets(hostname_buffer, sizeof(hostname_buffer), fp) != NULL) {
            // Remove newline
            size_t len = strlen(hostname_buffer);
            if (len > 0 && hostname_buffer[len-1] == '\n') {
                hostname_buffer[len-1] = '\0';
            }
            fclose(fp);
            return hostname_buffer;
        }
        fclose(fp);
    }
    
    // Fallback: use gethostname()
    if (gethostname(hostname_buffer, sizeof(hostname_buffer)) == 0) {
        return hostname_buffer;
    }
    
    strncpy(hostname_buffer, "Unknown", sizeof(hostname_buffer) - 1);
    return hostname_buffer;
}

const char* get_local_ip_address(void) {
    static char ip_buffer[256] = {0};
    
    // Si déjà lu, retourner le cache
    if (ip_buffer[0] != '\0') {
        return ip_buffer;
    }
    
    // Utiliser 'hostname -I' pour obtenir l'adresse IP locale
    FILE *fp = popen("hostname -I 2>/dev/null | awk '{print $1}'", "r");
    if (fp != NULL) {
        if (fgets(ip_buffer, sizeof(ip_buffer), fp) != NULL) {
            // Supprimer la newline
            size_t len = strlen(ip_buffer);
            if (len > 0 && ip_buffer[len-1] == '\n') {
                ip_buffer[len-1] = '\0';
            }
            pclose(fp);
            if (strlen(ip_buffer) > 0) {
                return ip_buffer;
            }
        }
        pclose(fp);
    }
    
    // Fallback
    strncpy(ip_buffer, "No IP", sizeof(ip_buffer) - 1);
    return ip_buffer;
}

const char* get_interface_ip_address(const char *interface_name) {
    static char ip_buffers[10][64] = {0};
    static char interface_names[10][64] = {0};
    static int buffer_count = 0;
    
    if (interface_name == NULL) {
        return "N/A";
    }
    
    // Chercher si on a déjà un buffer pour cette interface
    int buffer_index = -1;
    for (int i = 0; i < buffer_count; i++) {
        if (strcmp(interface_names[i], interface_name) == 0) {
            buffer_index = i;
            break;
        }
    }
    
    // Créer un nouveau buffer si nécessaire
    if (buffer_index == -1 && buffer_count < 10) {
        buffer_index = buffer_count;
        strncpy(interface_names[buffer_index], interface_name, sizeof(interface_names[0]) - 1);
        buffer_count++;
    }
    
    if (buffer_index == -1) {
        return "N/A";  // Trop d'interfaces
    }
    
    // Utiliser 'ip addr show' pour obtenir l'IP de cette interface
    char command[256];
    snprintf(command, sizeof(command), 
             "ip addr show %s 2>/dev/null | grep 'inet ' | awk '{print $2}' | cut -d/ -f1 | head -n1",
             interface_name);
    
    FILE *fp = popen(command, "r");
    if (fp != NULL) {
        if (fgets(ip_buffers[buffer_index], sizeof(ip_buffers[0]), fp) != NULL) {
            // Supprimer la newline
            size_t len = strlen(ip_buffers[buffer_index]);
            if (len > 0 && ip_buffers[buffer_index][len-1] == '\n') {
                ip_buffers[buffer_index][len-1] = '\0';
            }
            pclose(fp);
            
            // Si on a une IP valide, la retourner
            if (strlen(ip_buffers[buffer_index]) > 0) {
                return ip_buffers[buffer_index];
            }
        }
        pclose(fp);
    }
    
    // Pas d'IP assignée
    strncpy(ip_buffers[buffer_index], "No IP", sizeof(ip_buffers[0]) - 1);
    return ip_buffers[buffer_index];
}

const char* get_network_interfaces(void) {
    static char interfaces_buffer[512] = {0};
    
    // Si déjà lu, retourner le cache
    if (interfaces_buffer[0] != '\0') {
        return interfaces_buffer;
    }
    
    // Lire les interfaces réseau depuis /sys/class/net/
    FILE *fp = popen("ls /sys/class/net/ 2>/dev/null", "r");
    if (fp == NULL) {
        strncpy(interfaces_buffer, "Unknown", sizeof(interfaces_buffer) - 1);
        return interfaces_buffer;
    }
    
    char interface[64];
    char temp_buffer[512] = {0};
    int count = 0;
    
    while (fgets(interface, sizeof(interface), fp) != NULL && count < 5) {
        // Supprimer la newline
        size_t len = strlen(interface);
        if (len > 0 && interface[len-1] == '\n') {
            interface[len-1] = '\0';
        }
        
        // Ignorer loopback
        if (strcmp(interface, "lo") == 0) {
            continue;
        }
        
        // Ignorer les interfaces virtuelles par préfixe
        if (strncmp(interface, "docker", 6) == 0 ||
            strncmp(interface, "veth", 4) == 0 ||
            strncmp(interface, "br-", 3) == 0 ||
            strncmp(interface, "virbr", 5) == 0 ||
            strncmp(interface, "vmnet", 5) == 0 ||
            strncmp(interface, "vbox", 4) == 0 ||
            strncmp(interface, "tun", 3) == 0 ||
            strncmp(interface, "tap", 3) == 0) {
            continue;
        }
        
        // Vérifier si c'est une interface physique via le lien device
        char device_path[256];
        snprintf(device_path, sizeof(device_path), "/sys/class/net/%s/device", interface);
        
        struct stat stat_buf;
        if (lstat(device_path, &stat_buf) != 0 || !S_ISLNK(stat_buf.st_mode)) {
            // Pas un symlink ou n'existe pas -> interface virtuelle
            continue;
        }
        
        // Vérifier le type d'interface (type 1 = Ethernet, type 801 = WiFi 802.11)
        char type_path[256];
        snprintf(type_path, sizeof(type_path), "/sys/class/net/%s/type", interface);
        FILE *type_file = fopen(type_path, "r");
        int iface_type = 0;
        if (type_file != NULL) {
            fscanf(type_file, "%d", &iface_type);
            fclose(type_file);
        }
        
        // Si type n'est ni 1 (Ethernet) ni 801 (WiFi), ignorer
        if (iface_type != 1 && iface_type != 801) {
            continue;
        }
        
        // Déterminer le type d'interface de manière plus précise
        const char *type = "Unknown";
        
        // Vérifier si c'est du WiFi via /sys/class/net/*/wireless
        char wireless_path[256];
        snprintf(wireless_path, sizeof(wireless_path), "/sys/class/net/%s/wireless", interface);
        struct stat wireless_stat;
        if (stat(wireless_path, &wireless_stat) == 0 && S_ISDIR(wireless_stat.st_mode)) {
            type = "WiFi";
        } else if (iface_type == 801) {
            // Type 801 = WiFi même sans répertoire wireless
            type = "WiFi";
        } else {
            // C'est probablement Ethernet si type == 1 et pas WiFi
            if (strncmp(interface, "eth", 3) == 0 || strncmp(interface, "enp", 3) == 0 || 
                strncmp(interface, "eno", 3) == 0 || strncmp(interface, "ens", 3) == 0) {
                type = "Ethernet";
            } else if (strncmp(interface, "wlan", 4) == 0 || strncmp(interface, "wlp", 3) == 0 ||
                       strncmp(interface, "wlo", 3) == 0 || strncmp(interface, "wls", 3) == 0) {
                type = "WiFi";
            } else if (strncmp(interface, "ww", 2) == 0 || strncmp(interface, "usb", 3) == 0) {
                type = "Mobile";
            } else {
                // Par défaut, si type 1, c'est Ethernet
                type = "Ethernet";
            }
        }
        
        // Ajouter à la liste
        if (count > 0) {
            strncat(temp_buffer, ", ", sizeof(temp_buffer) - strlen(temp_buffer) - 1);
        }
        
        char iface_entry[128];
        snprintf(iface_entry, sizeof(iface_entry), "%s (%s)", interface, type);
        strncat(temp_buffer, iface_entry, sizeof(temp_buffer) - strlen(temp_buffer) - 1);
        count++;
    }
    pclose(fp);
    
    if (count == 0) {
        strncpy(interfaces_buffer, "No interfaces found", sizeof(interfaces_buffer) - 1);
    } else {
        strncpy(interfaces_buffer, temp_buffer, sizeof(interfaces_buffer) - 1);
    }
    
    return interfaces_buffer;
}

float get_network_upload_kbps(void) {
    return 125.4f;  // Mock: 125.4 KB/s
}

float get_network_download_kbps(void) {
    return 1524.8f;  // Mock: 1524.8 KB/s
}

// ============================================================================
// STATISTIQUES RÉSEAU PAR INTERFACE
// ============================================================================

// Structure pour stocker les stats réseau d'une interface
typedef struct {
    char interface_name[64];
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    unsigned long rx_bytes_prev;
    unsigned long tx_bytes_prev;
} NetworkStats;

// Cache pour les statistiques réseau par interface
static NetworkStats net_stats[10] = {0};
static int net_stats_count = 0;

// Lire les bytes reçus et transmis d'une interface depuis /proc/net/dev
static bool read_interface_stats(const char *interface_name, unsigned long *rx_bytes, unsigned long *tx_bytes) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (fp == NULL) {
        return false;
    }
    
    char line[256];
    bool found = false;
    
    // Ignorer les 2 premières lignes (headers)
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Format: "interface: rx_bytes rx_packets rx_errors ... tx_bytes tx_packets ..."
        char name[64];
        unsigned long rx, rx_packets, rx_errors, rx_drops, rx_fifo, rx_frame, rx_compressed, rx_multicast;
        unsigned long tx, tx_packets, tx_errors, tx_drops, tx_fifo, tx_colls, tx_carrier, tx_compressed;
        
        // Extraire le nom de l'interface (avant le ':')
        char *colon = strchr(line, ':');
        if (colon == NULL) continue;
        
        int name_len = colon - line;
        strncpy(name, line, name_len);
        name[name_len] = '\0';
        
        // Nettoyer les espaces au début
        char *clean_name = name;
        while (*clean_name == ' ') clean_name++;
        
        if (strcmp(clean_name, interface_name) == 0) {
            // Parser les statistiques
            sscanf(colon + 1, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &rx, &rx_packets, &rx_errors, &rx_drops, &rx_fifo, &rx_frame, &rx_compressed, &rx_multicast,
                   &tx, &tx_packets, &tx_errors, &tx_drops, &tx_fifo, &tx_colls, &tx_carrier, &tx_compressed);
            
            *rx_bytes = rx;
            *tx_bytes = tx;
            found = true;
            break;
        }
    }
    
    fclose(fp);
    return found;
}

float get_interface_download_kbps(const char *interface_name) {
    if (interface_name == NULL) {
        return 0.0f;
    }
    
    // Chercher ou créer une entrée pour cette interface
    int index = -1;
    for (int i = 0; i < net_stats_count; i++) {
        if (strcmp(net_stats[i].interface_name, interface_name) == 0) {
            index = i;
            break;
        }
    }
    
    // Créer nouvelle entrée si nécessaire
    if (index == -1 && net_stats_count < 10) {
        index = net_stats_count;
        strcpy(net_stats[index].interface_name, interface_name);
        net_stats_count++;
    }
    
    if (index == -1) {
        return 0.0f;  // Trop d'interfaces
    }
    
    unsigned long rx_bytes = 0, tx_bytes = 0;
    if (!read_interface_stats(interface_name, &rx_bytes, &tx_bytes)) {
        return 0.0f;
    }
    
    // Calculer la différence avec la lecture précédente
    if (net_stats[index].rx_bytes_prev == 0) {
        // Première lecture
        net_stats[index].rx_bytes_prev = rx_bytes;
        net_stats[index].rx_bytes = rx_bytes;
        return 0.0f;
    }
    
    unsigned long diff = rx_bytes - net_stats[index].rx_bytes_prev;
    net_stats[index].rx_bytes_prev = rx_bytes;
    net_stats[index].rx_bytes = rx_bytes;
    
    // Convertir bytes en KB/s (diff en 1 seconde)
    float kbps = diff / 1024.0f;
    return kbps;
}

float get_interface_upload_kbps(const char *interface_name) {
    if (interface_name == NULL) {
        return 0.0f;
    }
    
    // Chercher ou créer une entrée pour cette interface
    int index = -1;
    for (int i = 0; i < net_stats_count; i++) {
        if (strcmp(net_stats[i].interface_name, interface_name) == 0) {
            index = i;
            break;
        }
    }
    
    // Créer nouvelle entrée si nécessaire
    if (index == -1 && net_stats_count < 10) {
        index = net_stats_count;
        strcpy(net_stats[index].interface_name, interface_name);
        net_stats_count++;
    }
    
    if (index == -1) {
        return 0.0f;  // Trop d'interfaces
    }
    
    unsigned long rx_bytes = 0, tx_bytes = 0;
    if (!read_interface_stats(interface_name, &rx_bytes, &tx_bytes)) {
        return 0.0f;
    }
    
    // Calculer la différence avec la lecture précédente
    if (net_stats[index].tx_bytes_prev == 0) {
        // Première lecture
        net_stats[index].tx_bytes_prev = tx_bytes;
        net_stats[index].tx_bytes = tx_bytes;
        return 0.0f;
    }
    
    unsigned long diff = tx_bytes - net_stats[index].tx_bytes_prev;
    net_stats[index].tx_bytes_prev = tx_bytes;
    net_stats[index].tx_bytes = tx_bytes;
    
    // Convertir bytes en KB/s (diff en 1 seconde)
    float kbps = diff / 1024.0f;
    return kbps;
}
