/*
 * main.c
 * Application entry point for SysWatch
 */

#include <gtk/gtk.h>
#include "gui.h"
#include "system_info.h"

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create the graphical interface
    AppWidgets *widgets = create_gui();
    if (widgets == NULL) {
        g_printerr("Error: Unable to create graphical interface\n");
        return 1;
    }
    
    // Start the main event loop
    run_gui(widgets);
    
    // Clean up resources
    cleanup_gui(widgets);
    
    return 0;
}
