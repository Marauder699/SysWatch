/*
 * disk_info.c
 * Implémentation des fonctions de lecture des informations de disques physiques
 */

#define _GNU_SOURCE
#include "storage_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

float get_storage_used_gb(void) {
    return 45.2f;  // Mock: 45.2 GB
}

float get_storage_available_gb(void) {
    return 210.8f;  // Mock: 210.8 GB
}

// ============================================================================
// DISK SPEED TEST - GLOBAL
// ============================================================================

// Effectuer un test de vitesse de lecture/écriture disque global
void perform_storage_speed_test(float *read_speed_mbps, float *write_speed_mbps) {
    if (read_speed_mbps == NULL || write_speed_mbps == NULL) {
        return;
    }
    
    // Valeurs par défaut si le test échoue
    *read_speed_mbps = 0.0f;
    *write_speed_mbps = 0.0f;
    
    // Test d'écriture: créer un fichier de 100MB et mesurer le temps
    FILE *fp = fopen("/tmp/syswatch_speed_test.bin", "wb");
    if (fp == NULL) {
        return;
    }
    
    // Mesurer l'écriture
    char buffer[1024 * 1024];  // 1MB buffer
    memset(buffer, 'A', sizeof(buffer));
    
    struct timespec start_write, end_write;
    clock_gettime(CLOCK_MONOTONIC, &start_write);
    
    // Écrire 100 MB
    for (int i = 0; i < 100; i++) {
        if (fwrite(buffer, 1, sizeof(buffer), fp) != sizeof(buffer)) {
            fclose(fp);
            unlink("/tmp/syswatch_speed_test.bin");
            return;
        }
    }
    fflush(fp);
    
    clock_gettime(CLOCK_MONOTONIC, &end_write);
    fclose(fp);
    
    // Calculer la vitesse d'écriture
    double write_time = (end_write.tv_sec - start_write.tv_sec) + 
                       (end_write.tv_nsec - start_write.tv_nsec) / 1e9;
    *write_speed_mbps = (write_time > 0) ? (100.0f / write_time) : 0.0f;
    
    // Mesurer la lecture
    fp = fopen("/tmp/syswatch_speed_test.bin", "rb");
    if (fp == NULL) {
        unlink("/tmp/syswatch_speed_test.bin");
        return;
    }
    
    char read_buffer[1024 * 1024];
    
    struct timespec start_read, end_read;
    clock_gettime(CLOCK_MONOTONIC, &start_read);
    
    // Lire 100 MB
    for (int i = 0; i < 100; i++) {
        if (fread(read_buffer, 1, sizeof(read_buffer), fp) != sizeof(read_buffer)) {
            fclose(fp);
            unlink("/tmp/syswatch_speed_test.bin");
            return;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_read);
    fclose(fp);
    
    // Calculer la vitesse de lecture
    double read_time = (end_read.tv_sec - start_read.tv_sec) + 
                      (end_read.tv_nsec - start_read.tv_nsec) / 1e9;
    *read_speed_mbps = (read_time > 0) ? (100.0f / read_time) : 0.0f;
    
    // Nettoyer le fichier de test
    unlink("/tmp/syswatch_speed_test.bin");
}

// ============================================================================
// DISQUES PHYSIQUES
// ============================================================================

// Récupérer la liste des stockages physiques
PhysicalStorage* get_physical_storages(int *count) {
    if (count == NULL) {
        return NULL;
    }
    
    *count = 0;
    
    // Lire depuis /sys/block pour identifier les disques
    FILE *fp = popen("ls /sys/block/ | grep -E '^(sd|nvme|hd|mmcblk)'", "r");
    if (fp == NULL) {
        return NULL;
    }
    
    char storage_name[32];
    int storage_count = 0;
    PhysicalStorage *storages = malloc(sizeof(PhysicalStorage) * 10);  // Max 10 stockages
    
    // Lire les noms de stockages
    while (fgets(storage_name, sizeof(storage_name), fp) != NULL && storage_count < 10) {
        // Supprimer la newline
        size_t len = strlen(storage_name);
        if (len > 0 && storage_name[len-1] == '\n') {
            storage_name[len-1] = '\0';
        }
        
        // Ignorer les partitions:
        // - sda1, sdb2, etc. (partition SD/SATA)
        // - nvme0n1p1, etc. (partition NVMe) - contient 'p' suivi de chiffres
        // - hda1, hdb2, etc. (partition IDE)
        // - mmcblk0p1, etc. (partition SD card)
        // Les vrais disques: sda, nvme0n1, hda, mmcblk0
        
        // Vérifier si c'est une partition
        bool is_partition = false;
        if (strncmp(storage_name, "nvme", 4) == 0) {
            // Pour NVMe: nvme0n1 est OK, nvme0n1p1 n'est pas OK
            if (strchr(storage_name, 'p') != NULL && isdigit(storage_name[strlen(storage_name)-1])) {
                is_partition = true;  // C'est nvme0n1p1, nvme0n1p2, etc.
            }
        } else if (strncmp(storage_name, "mmcblk", 6) == 0) {
            // Pour SD card: mmcblk0 est OK, mmcblk0p1 n'est pas OK
            if (strchr(storage_name, 'p') != NULL && isdigit(storage_name[strlen(storage_name)-1])) {
                is_partition = true;  // C'est mmcblk0p1, mmcblk0p2, etc.
            }
        } else {
            // Pour SD/HD: sda, sdb sont OK, sda1, sda2 ne sont pas OK
            if (strlen(storage_name) > 0 && isdigit(storage_name[strlen(storage_name)-1])) {
                is_partition = true;
            }
        }
        
        if (is_partition) {
            continue;
        }
        
        // Ignorer les loops et autres
        if (strncmp(storage_name, "loop", 4) == 0 || 
            strncmp(storage_name, "dm-", 3) == 0 ||
            strncmp(storage_name, "ram", 3) == 0 ||
            strncmp(storage_name, "zram", 4) == 0) {
            continue;
        }
        
        // Initialiser la structure
        memset(&storages[storage_count], 0, sizeof(PhysicalStorage));
        strncpy(storages[storage_count].name, storage_name, sizeof(storages[storage_count].name) - 1);
        
        // Déterminer le type de disque
        if (strncmp(storage_name, "nvme", 4) == 0) {
            strncpy(storages[storage_count].type, "NVMe", sizeof(storages[storage_count].type) - 1);
            
            // Détecter la génération PCIe (Gen3, Gen4, Gen5)
            char pcie_speed_path[256];
            snprintf(pcie_speed_path, sizeof(pcie_speed_path), 
                     "/sys/block/%s/device/device/current_link_speed", storage_name);
            
            FILE *speed_fp = fopen(pcie_speed_path, "r");
            if (speed_fp != NULL) {
                char speed_str[32];
                if (fgets(speed_str, sizeof(speed_str), speed_fp) != NULL) {
                    // Format typique: "8.0 GT/s" (Gen3) ou "16.0 GT/s" (Gen4)
                    float speed_gt = 0.0f;
                    if (sscanf(speed_str, "%f GT/s", &speed_gt) == 1) {
                        if (speed_gt >= 32.0f) {
                            strncpy(storages[storage_count].interface, "PCIe Gen5", sizeof(storages[storage_count].interface) - 1);
                        } else if (speed_gt >= 16.0f) {
                            strncpy(storages[storage_count].interface, "PCIe Gen4", sizeof(storages[storage_count].interface) - 1);
                        } else if (speed_gt >= 8.0f) {
                            strncpy(storages[storage_count].interface, "PCIe Gen3", sizeof(storages[storage_count].interface) - 1);
                        } else if (speed_gt >= 5.0f) {
                            strncpy(storages[storage_count].interface, "PCIe Gen2", sizeof(storages[storage_count].interface) - 1);
                        } else {
                            strncpy(storages[storage_count].interface, "PCIe Gen1", sizeof(storages[storage_count].interface) - 1);
                        }
                    } else {
                        strncpy(storages[storage_count].interface, "PCIe", sizeof(storages[storage_count].interface) - 1);
                    }
                } else {
                    strncpy(storages[storage_count].interface, "PCIe", sizeof(storages[storage_count].interface) - 1);
                }
                fclose(speed_fp);
            } else {
                // Fallback si le fichier n'existe pas
                strncpy(storages[storage_count].interface, "PCIe", sizeof(storages[storage_count].interface) - 1);
            }
        } else if (strncmp(storage_name, "sd", 2) == 0 || strncmp(storage_name, "hd", 2) == 0) {
            // Vérifier si c'est USB
            char usb_path[256];
            snprintf(usb_path, sizeof(usb_path), "/sys/block/%s/device", storage_name);
            char *resolved = realpath(usb_path, NULL);
            
            if (resolved != NULL && strstr(resolved, "usb") != NULL) {
                strncpy(storages[storage_count].type, "USB", sizeof(storages[storage_count].type) - 1);
                
                // Déterminer la version USB en lisant la vitesse du port USB
                char speed_path[512];
                char *usb_device = strstr(resolved, "/usb");
                
                if (usb_device != NULL) {
                    // Extraire le chemin jusqu'au device USB
                    char *next_slash = strchr(usb_device + 4, '/');
                    if (next_slash != NULL) {
                        next_slash = strchr(next_slash + 1, '/');
                        if (next_slash != NULL) {
                            size_t len = next_slash - resolved;
                            snprintf(speed_path, sizeof(speed_path), "%.*s/speed", (int)len, resolved);
                            
                            // Lire la vitesse
                            FILE *speed_fp = fopen(speed_path, "r");
                            if (speed_fp != NULL) {
                                int speed = 0;
                                if (fscanf(speed_fp, "%d", &speed) == 1) {
                                    // USB 1.x = 1.5 ou 12 Mbps
                                    // USB 2.0 = 480 Mbps
                                    // USB 3.0 = 5000 Mbps
                                    // USB 3.1 = 10000 Mbps
                                    // USB 3.2 = 20000 Mbps
                                    if (speed >= 10000) {
                                        strncpy(storages[storage_count].interface, "USB 3.1+", sizeof(storages[storage_count].interface) - 1);
                                    } else if (speed >= 5000) {
                                        strncpy(storages[storage_count].interface, "USB 3.0", sizeof(storages[storage_count].interface) - 1);
                                    } else if (speed >= 480) {
                                        strncpy(storages[storage_count].interface, "USB 2.0", sizeof(storages[storage_count].interface) - 1);
                                    } else {
                                        strncpy(storages[storage_count].interface, "USB 1.x", sizeof(storages[storage_count].interface) - 1);
                                    }
                                } else {
                                    strncpy(storages[storage_count].interface, "USB", sizeof(storages[storage_count].interface) - 1);
                                }
                                fclose(speed_fp);
                            } else {
                                strncpy(storages[storage_count].interface, "USB", sizeof(storages[storage_count].interface) - 1);
                            }
                        } else {
                            strncpy(storages[storage_count].interface, "USB", sizeof(storages[storage_count].interface) - 1);
                        }
                    } else {
                        strncpy(storages[storage_count].interface, "USB", sizeof(storages[storage_count].interface) - 1);
                    }
                } else {
                    strncpy(storages[storage_count].interface, "USB", sizeof(storages[storage_count].interface) - 1);
                }
                free(resolved);
            } else {
                // SATA ou IDE
                strncpy(storages[storage_count].type, "HDD", sizeof(storages[storage_count].type) - 1);
                strncpy(storages[storage_count].interface, "SATA", sizeof(storages[storage_count].interface) - 1);
                if (resolved != NULL) free(resolved);
            }
        } else if (strncmp(storage_name, "mmcblk", 6) == 0) {
            strncpy(storages[storage_count].type, "SD Card", sizeof(storages[storage_count].type) - 1);
            strncpy(storages[storage_count].interface, "SD/MMC", sizeof(storages[storage_count].interface) - 1);
        }
        
        // Lire la capacité depuis /sys/block/[storage]/size (fallback)
        char size_path[256];
        snprintf(size_path, sizeof(size_path), "/sys/block/%s/size", storage_name);
        FILE *size_fp = fopen(size_path, "r");
        if (size_fp != NULL) {
            unsigned long sectors = 0;
            fscanf(size_fp, "%lu", &sectors);
            fclose(size_fp);
            storages[storage_count].capacity_gb = (sectors * 512) / (1024.0 * 1024.0 * 1024.0);
        }
        
        // Lire le modèle du stockage
        char model_path[256];
        snprintf(model_path, sizeof(model_path), "/sys/block/%s/device/model", storage_name);
        FILE *model_fp = fopen(model_path, "r");
        if (model_fp != NULL) {
            if (fgets(storages[storage_count].model, sizeof(storages[storage_count].model), model_fp) != NULL) {
                // Supprimer la newline
                size_t model_len = strlen(storages[storage_count].model);
                if (model_len > 0 && storages[storage_count].model[model_len-1] == '\n') {
                    storages[storage_count].model[model_len-1] = '\0';
                }
            }
            fclose(model_fp);
        }
        
        // Lire les données d'utilisation via df (prioritaire pour la capacité réelle)
        storages[storage_count].used_gb = 0.0f;
        storages[storage_count].available_gb = 0.0f;
        
        // Chercher toutes les partitions du stockage et additionner leurs stats
        // Utiliser -B 1M pour obtenir les tailles en MB, puis convertir en GB
        char partition_pattern[512];
        if (strncmp(storage_name, "nvme", 4) == 0) {
            snprintf(partition_pattern, sizeof(partition_pattern), 
                     "df -B 1M | grep '/dev/%sp' | awk '{total+=$2; used+=$3; avail+=$4} END {if (NR>0) print total, used, avail}'", storage_name);
        } else if (strncmp(storage_name, "mmcblk", 6) == 0) {
            snprintf(partition_pattern, sizeof(partition_pattern), 
                     "df -B 1M | grep '/dev/%sp' | awk '{total+=$2; used+=$3; avail+=$4} END {if (NR>0) print total, used, avail}'", storage_name);
        } else {
            snprintf(partition_pattern, sizeof(partition_pattern),
                     "df -B 1M | grep '/dev/%s' | grep -E '[0-9]' | awk '{total+=$2; used+=$3; avail+=$4} END {if (NR>0) print total, used, avail}'", storage_name);
        }
        
        FILE *df_fp = popen(partition_pattern, "r");
        if (df_fp != NULL) {
            char df_line[256];
            if (fgets(df_line, sizeof(df_line), df_fp) != NULL) {
                unsigned long total, used, available;
                // Format: size used available (en MB grâce à -B 1M)
                if (sscanf(df_line, "%lu %lu %lu", &total, &used, &available) == 3) {
                    storages[storage_count].capacity_gb = (float)total / 1024.0f;
                    storages[storage_count].used_gb = (float)used / 1024.0f;
                    storages[storage_count].available_gb = (float)available / 1024.0f;
                }
            }
            pclose(df_fp);
        }
        
        storage_count++;
    }
    pclose(fp);
    
    *count = storage_count;
    
    if (storage_count == 0) {
        free(storages);
        return NULL;
    }
    
    return storages;
}

// Libérer la mémoire des stockages
void free_physical_storages(PhysicalStorage *storages) {
    if (storages != NULL) {
        free(storages);
    }
}

// Trouver le point de montage pour un disque donné
static bool find_storage_mount_point(const char *storage_name, char *mount_point, size_t max_len) {
    if (storage_name == NULL || mount_point == NULL) {
        return false;
    }
    
    // Construire le pattern de recherche pour les partitions
    char partition_pattern[128];
    if (strncmp(storage_name, "nvme", 4) == 0 || strncmp(storage_name, "mmcblk", 6) == 0) {
        snprintf(partition_pattern, sizeof(partition_pattern), "/dev/%sp", storage_name);
    } else {
        snprintf(partition_pattern, sizeof(partition_pattern), "/dev/%s", storage_name);
    }
    
    // Lire /proc/mounts pour trouver une partition montée de ce disque
    FILE *fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        return false;
    }
    
    char line[512];
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        char device[256], mpoint[256], fstype[64];
        if (sscanf(line, "%255s %255s %63s", device, mpoint, fstype) == 3) {
            // Vérifier si c'est une partition de notre disque
            if (strncmp(device, partition_pattern, strlen(partition_pattern)) == 0) {
                // Éviter les pseudo-filesystems
                if (strcmp(fstype, "tmpfs") != 0 && 
                    strcmp(fstype, "devtmpfs") != 0 &&
                    strcmp(fstype, "proc") != 0 &&
                    strcmp(fstype, "sysfs") != 0) {
                    // Prendre le premier point de montage valide trouvé
                    strncpy(mount_point, mpoint, max_len - 1);
                    mount_point[max_len - 1] = '\0';
                    fclose(fp);
                    return true;
                }
            }
        }
    }
    fclose(fp);
    
    return false;
}

// Effectuer un test de vitesse pour un disque spécifique
void get_storage_speed_test(const char *storage_name, float *read_mbps, float *write_mbps) {
    if (storage_name == NULL || read_mbps == NULL || write_mbps == NULL) {
        return;
    }
    
    *read_mbps = 0.0f;
    *write_mbps = 0.0f;
    
    // Trouver un point de montage pour ce disque
    char mount_point[256];
    if (!find_storage_mount_point(storage_name, mount_point, sizeof(mount_point))) {
        // Pas de point de montage trouvé, le test ne peut pas être effectué
        return;
    }
    
    // Si le point de montage est la racine ("/"), utiliser /tmp pour éviter les problèmes de permissions
    char test_dir[512];
    if (strcmp(mount_point, "/") == 0) {
        snprintf(test_dir, sizeof(test_dir), "/tmp");
    } else {
        snprintf(test_dir, sizeof(test_dir), "%s", mount_point);
    }
    
    // Vérifier les permissions d'écriture sur le répertoire de test
    char test_write_path[512];
    snprintf(test_write_path, sizeof(test_write_path), "%s/.syswatch_perm_test", test_dir);
    FILE *perm_test = fopen(test_write_path, "w");
    if (perm_test == NULL) {
        // Impossible d'écrire sur ce disque
        return;
    }
    fclose(perm_test);
    unlink(test_write_path);
    
    // Créer un fichier de test
    char test_file[512];
    snprintf(test_file, sizeof(test_file), "%s/.syswatch_speed_test_%s.bin", 
             test_dir, storage_name);
    
    // Vérifier l'espace disponible pour ajuster la taille du test
    struct statvfs stat;
    int test_size_mb = 100;
    if (statvfs(test_dir, &stat) == 0) {
        unsigned long available_mb = (stat.f_bavail * stat.f_bsize) / (1024 * 1024);
        if (available_mb < 200) {
            test_size_mb = 20;
        }
    }
    
    // Détecter le système de fichiers
    bool use_direct_io = true;
    bool use_aligned_buffer = false;
    
    FILE *mounts_fp = fopen("/proc/mounts", "r");
    if (mounts_fp != NULL) {
        char line[512];
        while (fgets(line, sizeof(line), mounts_fp) != NULL) {
            char device[256], mpoint[256], fs[64];
            if (sscanf(line, "%255s %255s %63s", device, mpoint, fs) == 3) {
                if (strcmp(mpoint, mount_point) == 0) {
                    // VFAT/FAT32 ne supporte pas O_DIRECT
                    if (strcmp(fs, "vfat") == 0 || strcmp(fs, "msdos") == 0 || strcmp(fs, "fat") == 0) {
                        use_direct_io = false;
                    } else {
                        use_direct_io = true;
                        use_aligned_buffer = true;
                    }
                    break;
                }
            }
        }
        fclose(mounts_fp);
    }
    
    const size_t buffer_size = 1024 * 1024;
    const int iterations = test_size_mb;
    
    // Allouer un buffer
    void *buffer = NULL;
    if (use_aligned_buffer) {
        if (posix_memalign(&buffer, 512, buffer_size) != 0) {
            return;
        }
    } else {
        buffer = malloc(buffer_size);
        if (buffer == NULL) {
            return;
        }
    }
    
    // Remplir le buffer avec des données
    unsigned char *data = (unsigned char *)buffer;
    for (size_t i = 0; i < buffer_size; i++) {
        data[i] = (unsigned char)(i % 256);
    }
    
    struct timespec start, end;
    
    // ========== TEST D'ÉCRITURE ==========
    int fd;
    if (use_direct_io) {
        fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0644);
        if (fd < 0) {
            fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
        }
    } else {
        fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
    }
    
    if (fd < 0) {
        free(buffer);
        return;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        data[0] = (unsigned char)i;
        ssize_t written = write(fd, buffer, buffer_size);
        if (written != (ssize_t)buffer_size) {
            close(fd);
            unlink(test_file);
            free(buffer);
            return;
        }
    }
    fsync(fd);
    clock_gettime(CLOCK_MONOTONIC, &end);
    close(fd);
    
    double write_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    *write_mbps = (write_time > 0) ? ((float)iterations / write_time) : 0.0f;
    
    // ========== TEST DE LECTURE ==========
    sync();
    
    if (use_direct_io) {
        usleep(100000);
    } else {
        usleep(500000);
    }
    
    if (use_direct_io) {
        fd = open(test_file, O_RDONLY | O_DIRECT);
        if (fd < 0) {
            fd = open(test_file, O_RDONLY);
        }
    } else {
        fd = open(test_file, O_RDONLY);
    }
    
    if (fd < 0) {
        unlink(test_file);
        free(buffer);
        return;
    }
    
    if (!use_direct_io) {
        posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        ssize_t bytes_read = read(fd, buffer, buffer_size);
        if (bytes_read != (ssize_t)buffer_size) {
            close(fd);
            unlink(test_file);
            free(buffer);
            return;
        }
        if (!use_direct_io) {
            posix_fadvise(fd, i * buffer_size, buffer_size, POSIX_FADV_DONTNEED);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    close(fd);
    
    double read_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    *read_mbps = (read_time > 0) ? ((float)iterations / read_time) : 0.0f;
    
    unlink(test_file);
    free(buffer);
}
