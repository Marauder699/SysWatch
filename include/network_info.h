/*
 * network_info.h
 * Bibliothèque pour lire les informations réseau et la bande passante
 */

#ifndef NETWORK_INFO_H
#define NETWORK_INFO_H

/*
 * Récupérer le nom d'hôte de la machine
 * Retourne une chaîne de caractères (buffer statique)
 */
const char* get_hostname(void);

/*
 * Récupérer l'adresse IP locale de la première interface réseau
 * Retourne une chaîne de caractères (buffer statique)
 */
const char* get_local_ip_address(void);

/*
 * Récupérer l'adresse IP d'une interface réseau spécifique
 * interface_name : nom de l'interface (ex: "eth0", "wlan0")
 * Retourne une chaîne de caractères (buffer dynamique, réutilisé entre appels)
 */
const char* get_interface_ip_address(const char *interface_name);

/*
 * Récupérer la liste des interfaces réseau disponibles
 * Retourne une chaîne formatée avec les noms et types (Ethernet, WiFi, Mobile)
 * Format: "eth0 (Ethernet), wlan0 (WiFi), ..."
 */
const char* get_network_interfaces(void);

/*
 * Récupérer la vitesse de téléchargement réseau globale en KB/s
 * Note: Fonction actuellement mockée (retourne valeur constante)
 */
float get_network_upload_kbps(void);

/*
 * Récupérer la vitesse de chargement réseau globale en KB/s
 * Note: Fonction actuellement mockée (retourne valeur constante)
 */
float get_network_download_kbps(void);

/*
 * Récupérer la vitesse de téléchargement pour une interface spécifique en KB/s
 * interface_name : nom de l'interface (ex: "eth0", "wlan0")
 * Retourne la vitesse en KB/s (moyenne depuis la dernière lecture)
 */
float get_interface_download_kbps(const char *interface_name);

/*
 * Récupérer la vitesse de chargement pour une interface spécifique en KB/s
 * interface_name : nom de l'interface (ex: "eth0", "wlan0")
 * Retourne la vitesse en KB/s (moyenne depuis la dernière lecture)
 */
float get_interface_upload_kbps(const char *interface_name);

#endif // NETWORK_INFO_H
