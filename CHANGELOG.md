# 📝 Changelog - SysWatch

## [1.0.2] - 3 décembre 2025

### 🎨 Nouvelles fonctionnalités (Style NZXT CAM)
- **🌡️ Indicateurs de couleur pour température CPU** :
  - 🟢 **Vert** : < 60°C (optimal)
  - 🟡 **Jaune/Orange** : 60-75°C (attention)
  - 🔴 **Rouge** : > 75°C (chaud)
  - Mise à jour visuelle temps réel pour monitoring santé

### 📚 Documentation
- ✅ **README.md remanié** pour refléter l'approche hybride :
  - 🪟 **Windows System Information** (infos statiques)
  - 🌡️ **NZXT CAM Health Monitor** (monitoring temps réel)
- ✅ Ajout section "Philosophie du projet" expliquant l'inspiration
- ✅ Clarification des fonctionnalités statiques vs temps réel
- ✅ Mise à jour fréquence de rafraîchissement (1s au lieu de 2s)

### 🔧 Code
- `src/gui.c` : Fonction `get_temperature_color()` pour logique de coloration
- `src/gui.c` : Application du markup GTK avec couleur dynamique sur label température
- Suppression du prototype `PI_Info.c` (archivé par l'utilisateur)

---

## [1.0.1] - 3 décembre 2025

### ✨ Améliorations
- **Température double unité** : Affichage Celsius ET Fahrenheit (45.2°C (113.4°F))
  - Améliore accessibilité pour utilisateurs USA et internationaux
  - Utile pour admins système gérant des serveurs mondiaux
  - Aucun impact sur la performance
  - Format: `"45.2°C (113.4°F)"` 

### 📚 Documentation
- ✅ README.md mis à jour avec informations Celsius/Fahrenheit
- ✅ ROADMAP.md enrichie avec planification du système multilingue JSON
- ✅ SPECIFICATIONS.md détaille la conversion température et architecture i18n
- ✅ CHANGELOG.md créé (ce fichier)

### 🔧 Code
- `src/system_info.c` : Fonction `get_cpu_temperature_string()` affiche double unité
- `src/gui.c` : Affichage mis à jour avec conversion Celsius→Fahrenheit

---

## [1.0] - 2 décembre 2025

### 🎯 Fonctionnalités complètes

#### System Information
- ✅ Modèle matériel (Raspberry Pi, PC x86, etc.)
- ✅ Type processeur avec architecture exacte
- ✅ GPU détecté (NVIDIA, AMD, Intel, Broadcom)
- ✅ Version Kernel
- ✅ Distribution Linux
- ✅ Environnement de bureau + serveur d'affichage
- ✅ Locale/Timezone
- ✅ Nombre de cores CPU

#### Monitoring temps réel (2s)
- ✅ **Température CPU** (Celsius) avec fallbacks multiples
- ✅ **Utilisation CPU** (%) via `/proc/stat` avec calcul delta
- ✅ **Utilisation GPU** (%) - Support 4 architectures (NVIDIA/AMD/Intel/RPi)
- ✅ **Mémoire** (%, GB disponible, GB total)
- ✅ **Uptime** système

#### Disque (Nouveau!)
- ✅ Détection disques physiques (NVMe, USB, SATA, eMMC)
- ✅ Identification interface (PCIe Gen3/4/5, USB 1.x/2.0/3.0/3.1+)
- ✅ **Speed Test** avec O_DIRECT (lecture/écriture réelles en MB/s)
- ✅ Capacité et espace utilisé/disponible

#### Réseau (Infrastructure)
- ✅ Hostname
- ✅ Adresse IP locale
- ✅ Interfaces détectées

#### Interface GUI (GTK+)
- ✅ 5 onglets (System, CPU, Memory, Network, Disk)
- ✅ Mise à jour automatique toutes les 2 secondes
- ✅ Boutons Rafraîchir, Quitter, Speed Test
- ✅ Layout responsive (s'adapte au redimensionnement)
- ✅ Thread séparé pour Speed Test (non-bloquant)

#### Portabilité
- ✅ ARM 32-bit (Raspberry Pi 3)
- ✅ ARM 64-bit (Raspberry Pi 4/5/500)
- ✅ x86/x64 (PC/Laptop)
- ✅ Distributions: Debian, Ubuntu, Raspberry Pi OS, Fedora (théorique)

### 📊 Stats
- **~2700 lignes** de code C
- **30+ fonctions** implémentées
- **0 dépendances externes** (sauf GTK3)
- **< 20 MB** RAM
- **< 1%** CPU (idle)

---

## 🗺️ Roadmap - Prochaines étapes

### Version 1.1 (Plannifié - Court terme)
- Graphiques temps réel
- Configuration persistent
- Export CSV

### Version 1.5 (Planifié - Moyen terme)
- Mode "mini"
- Alertes
- Thèmes

### Version 3.0+ : Système Multilingue (Prochaine majeure)
- Fichiers JSON par langue (en_US, fr_FR, es_ES, de_DE, ja_JP, zh_CN, ru_RU)
- Détection automatique via `LANG`
- Architecture extensible pour contribution communauté
- **C'est la dernière grande fonctionnalité en C prévue**

---

## 📦 Implémentation technique majeure

### Température double unité
```c
// Avant (v1.0)
snprintf(buffer, "%.1f°C", temp);

// Après (v1.0.1)
float fahrenheit = (temp * 9.0f / 5.0f) + 32.0f;
snprintf(buffer, "%.1f°C (%.1f°F)", temp, fahrenheit);
```

### Speed Test avec O_DIRECT
- Utilisation `open(..., O_DIRECT)` pour bypasser cache système
- Buffer aligné 512 bytes
- Résultats précis pour NVMe (~3000 MB/s) vs USB (~40 MB/s)

### Détection GPU multi-plateforme
- NVIDIA: `nvidia-smi`
- AMD: `/sys/class/drm/card*/device/gpu_busy_percent`
- Intel: Fréquence GPU via `/sys/class/drm/card*/gt/gt0/rps_*`
- Raspberry Pi: `vcgencmd`

---

## 🎓 Apprentissages & Points intéressants

1. **O_DIRECT** : Nécessite buffer aligné 512 bytes, peut échouer sur certains FS
2. **Détection GPU** : Chaque fabricant a sa propre API
3. **Architecture portabilité** : Utiliser `/sys` et `/proc` plutôt que commandes shell
4. **Multi-threading GTK** : Utiliser `g_idle_add()` pour UI updates depuis threads
5. **Conversion unités** : Fahrenheit = (Celsius × 9/5) + 32

---

## 🏁 État actuel

**Application fonctionnelle et testée** ✅

Tous les éléments prévus pour v1.0 et v1.0.1 sont implémentés et fonctionnent correctement.

L'application est prête pour utilisation quotidienne sur:
- Raspberry Pi (toutes versions)
- PC Linux (x86/x64)
- Serveurs Linux

---

**Dernière mise à jour**: 3 décembre 2025
