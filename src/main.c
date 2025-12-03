/*
 * main.c
 * Point d'entrée de l'application SysWatch
 */

#include <gtk/gtk.h>
#include "gui.h"
#include "system_info.h"

int main(int argc, char *argv[]) {
    // Initialiser GTK
    gtk_init(&argc, &argv);
    
    // Créer l'interface graphique
    AppWidgets *widgets = create_gui();
    if (widgets == NULL) {
        g_printerr("Erreur: Impossible de créer l'interface graphique\n");
        return 1;
    }
    
    // Lancer la boucle principale
    run_gui(widgets);
    
    // Nettoyer les ressources
    cleanup_gui(widgets);
    
    return 0;
}
