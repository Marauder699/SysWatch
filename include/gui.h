/*
 * gui.h
 * Interface graphique GTK pour l'application PI_Info
 */

#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "system_info.h"

// Structure pour stocker les widgets d'une interface réseau
typedef struct {
    char interface_name[64];
    GtkWidget *ip_label;
    GtkWidget *upload_label;
    GtkWidget *download_label;
} NetworkInterfaceWidgets;

// Structure pour stocker les labels d'un stockage physique
typedef struct {
    char storage_name[32];
    GtkWidget *used_label;
    GtkWidget *available_label;
    GtkWidget *total_label;
    GtkWidget *percent_label;
    GtkWidget *read_label;
    GtkWidget *write_label;
    GtkWidget *speed_test_button;
    float read_speed;
    float write_speed;
} StorageWidgets;

// Structure contenant tous les widgets de l'application
typedef struct {
    GtkWidget *window;
    
    // Labels System Info
    GtkWidget *hardware_label;
    GtkWidget *processor_label;
    GtkWidget *architecture_label;
    GtkWidget *cpu_cores_label;
    GtkWidget *gpu_label;
    GtkWidget *kernel_label;
    GtkWidget *distro_label;
    GtkWidget *display_label;
    GtkWidget *locale_label;
    GtkWidget *uptime_label;
    
    // Labels Processeur
    GtkWidget *temp_label;
    GtkWidget *cpu_usage_label;
    GtkWidget *gpu_usage_label;
    
    // Labels Mémoire
    GtkWidget *mem_usage_label;
    GtkWidget *mem_available_label;
    GtkWidget *mem_total_label;
    
    // Labels Réseau
    GtkWidget *network_hostname_label;
    GtkWidget *network_ip_label;
    GtkWidget *network_vbox;  // Conteneur dynamique pour les interfaces
    
    // Interfaces réseau dynamiques
    NetworkInterfaceWidgets *network_interfaces;
    int network_interface_count;
    
    // Stockages physiques dynamiques
    StorageWidgets *storages;
    int storage_count;
    PhysicalStorage *physical_storages;  // Données brutes des stockages
    
    // Labels Stockage (anciens, à garder pour compatibilité)
    GtkWidget *storage_used_label;
    GtkWidget *storage_available_label;
    GtkWidget *storage_total_label;
    GtkWidget *storage_percent_label;
    GtkWidget *storage_read_speed_label;
    GtkWidget *storage_write_speed_label;
    GtkWidget *speed_test_button;
    GtkWidget *storage_vbox;  // Conteneur pour la liste des stockages
    
    // Boutons
    GtkWidget *about_button;
    GtkWidget *quit_button;
} AppWidgets;

/*
 * Crée et initialise l'interface graphique
 * Retourne un pointeur vers la structure AppWidgets
 */
AppWidgets* create_gui(void);

/*
 * Met à jour uniquement la section System Info (kernel, distro, desktop)
 */
void update_system_info_display(AppWidgets *widgets);

/*
 * Met à jour tous les affichages dynamiques (temp, CPU, mémoire, réseau, disque)
 */
void update_all_displays(AppWidgets *widgets);

/*
 * Démarre la boucle principale GTK
 */
void run_gui(AppWidgets *widgets);

/*
 * Libère les ressources de l'interface
 */
void cleanup_gui(AppWidgets *widgets);

#endif // GUI_H
