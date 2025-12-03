/*
 * system_info.h
 * Bibliothèque pour lire les informations système (température, CPU, RAM, etc.)
 */

#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stdbool.h>
#include <stddef.h>
#include "storage_info.h"  // Pour les structures stockage
#include "network_info.h"  // Pour les fonctions réseau

/*
 * Lit la température du CPU
 * Retourne la température en degrés Celsius
 * Retourne -1.0 en cas d'erreur
 */
float get_cpu_temperature_celsius(void);

/*
 * Obtient une chaîne formatée de la température
 * Le buffer doit avoir au moins 64 caractères
 * Retourne true en cas de succès, false sinon
 */
bool get_cpu_temperature_string(char *buffer, size_t buffer_size);

/*
 * Fonctions mockées - retournent des valeurs constantes pour le moment
 */

// System Info
const char* get_hardware_model(void);
const char* get_processor_type(void);
const char* get_architecture_info(void);
const char* get_cpu_cores(void);
const char* get_gpu_info(void);
const char* get_kernel_version(void);
const char* get_locale_info(void);
const char* get_distro_info(void);
const char* get_desktop_environment(void);
const char* get_uptime_string(void);

// CPU & GPU
float get_cpu_usage_percent(void);
float get_gpu_usage_percent(void);

// Mémoire
float get_memory_usage_percent(void);
float get_memory_available_gb(void);
float get_memory_total_gb(void);

#endif // SYSTEM_INFO_H

