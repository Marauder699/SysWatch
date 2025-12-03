/*
 * storage_info.h
 * Bibliothèque pour lire les informations de stockage physique et effectuer des tests de vitesse
 */

#ifndef STORAGE_INFO_H
#define STORAGE_INFO_H

// Structure pour représenter un stockage physique
typedef struct {
    char name[32];           // ex: "sda", "nvme0n1"
    char type[16];           // ex: "HDD", "SSD", "NVMe", "USB"
    char interface[16];      // ex: "SATA", "USB2.0", "USB3.0", "NVMe"
    char model[128];         // ex: "Samsung 870 EVO"
    float capacity_gb;       // Capacité totale en GB
    float used_gb;           // Espace utilisé en GB
    float available_gb;      // Espace disponible en GB
} PhysicalStorage;

/*
 * Récupérer la liste des stockages physiques détectés
 * Retourne un tableau de structures PhysicalStorage
 * count : pointeur pour stocker le nombre de stockages trouvés
 * Retourne NULL si aucun stockage trouvé
 * IMPORTANT: Libérer la mémoire avec free_physical_storages()
 */
PhysicalStorage* get_physical_storages(int *count);

/*
 * Libérer la mémoire allouée par get_physical_storages()
 */
void free_physical_storages(PhysicalStorage *storages);

/*
 * Effectuer un test de vitesse pour un stockage spécifique
 * storage_name : nom du stockage (ex: "sda", "nvme0n1")
 * read_mbps : pointeur pour stocker la vitesse de lecture en MB/s
 * write_mbps : pointeur pour stocker la vitesse d'écriture en MB/s
 * 
 * Note: Le test crée un fichier temporaire de 20-100 MB selon l'espace disponible
 * Les valeurs seront 0.0f si le test échoue ou si le stockage n'est pas accessible
 */
void get_storage_speed_test(const char *storage_name, float *read_mbps, float *write_mbps);

/*
 * Effectuer un test de vitesse stockage global (sur /tmp)
 * read_speed_mbps : pointeur pour stocker la vitesse de lecture en MB/s
 * write_speed_mbps : pointeur pour stocker la vitesse d'écriture en MB/s
 */
void perform_storage_speed_test(float *read_speed_mbps, float *write_speed_mbps);

/*
 * Récupérer l'espace utilisé sur le stockage principal
 * Retourne la taille en GB
 */
float get_storage_used_gb(void);

/*
 * Récupérer l'espace disponible sur le stockage principal
 * Retourne la taille en GB
 */
float get_storage_available_gb(void);

#endif // STORAGE_INFO_H
