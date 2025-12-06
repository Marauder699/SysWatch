/*
 * gui.c
 * GTK graphical user interface implementation
 * 
 * Naming conventions:
 * - GTK functions: gtk_*(), g_*()         [GTK Library]
 * - Public functions: create_gui(), run_gui(), etc. [Public API]
 * - Private functions: on_*(), update_*()          [Internal Callbacks]
 */

#include "gui.h"
#include "system_info.h"
#include <stdlib.h>
#include <glib.h>

// Structure to pass disk speed test data to the thread
typedef struct {
    AppWidgets *widgets;
    GtkWidget *button;
} DiskSpeedTestData;

// Macro to convert a number to string
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static gboolean update_storage_speed_test_results(gpointer data);
static void init_physical_storages(AppWidgets *widgets);

// ============================================================================
// PRIVATE FUNCTIONS (CALLBACKS)
// ============================================================================

// Thread to perform disk speed tests
static gpointer storage_speed_test_thread(gpointer data) {
    DiskSpeedTestData *test_data = (DiskSpeedTestData *)data;
    AppWidgets *widgets = test_data->widgets;
    
    // Loop through all disks and perform tests
    for (int i = 0; i < widgets->storage_count; i++) {
        float read_speed = 0.0f;
        float write_speed = 0.0f;
        
        // Perform the speed test for this disk
        get_storage_speed_test(widgets->physical_storages[i].name, &read_speed, &write_speed);
        
        // Store results in the structure
        widgets->storages[i].read_speed = read_speed;
        widgets->storages[i].write_speed = write_speed;
    }
    
    // Request UI update (thread-safe via g_idle_add)
    g_idle_add(update_storage_speed_test_results, test_data);
    
    return NULL;
}

// Callback to update disk speed test results
static gboolean update_storage_speed_test_results(gpointer data) {
    DiskSpeedTestData *test_data = (DiskSpeedTestData *)data;
    AppWidgets *widgets = test_data->widgets;
    
    char buffer[64];
    
    // Update Read/Write labels for each disk
    for (int i = 0; i < widgets->storage_count; i++) {
        // Check if this is an NVMe (results are approximate due to controller cache)
        bool is_nvme = (strncmp(widgets->physical_storages[i].name, "nvme", 4) == 0);
        const char *suffix = is_nvme ? " ~" : "";  // ~ indicates approximate value
        
        // Update Read label
        if (widgets->storages[i].read_speed > 0) {
            snprintf(buffer, sizeof(buffer), "%.1f MB/s%s", widgets->storages[i].read_speed, suffix);
        } else {
            snprintf(buffer, sizeof(buffer), "N/A");
        }
        gtk_label_set_text(GTK_LABEL(widgets->storages[i].read_label), buffer);
        
        // Update Write label
        if (widgets->storages[i].write_speed > 0) {
            snprintf(buffer, sizeof(buffer), "%.1f MB/s%s", widgets->storages[i].write_speed, suffix);
        } else {
            snprintf(buffer, sizeof(buffer), "N/A");
        }
        gtk_label_set_text(GTK_LABEL(widgets->storages[i].write_label), buffer);
        
        // Add tooltip for NVMe explaining the approximation
        if (is_nvme) {
            gtk_widget_set_tooltip_text(widgets->storages[i].read_label, 
                "~ Approximate: NVMe speeds are limited by PCIe bandwidth.\n"
                "Actual speeds may vary due to controller cache.");
            gtk_widget_set_tooltip_text(widgets->storages[i].write_label, 
                "~ Approximate: NVMe speeds are limited by PCIe bandwidth.\n"
                "Actual speeds may vary due to controller cache.");
        }
    }
    
    // Re-enable the button
    gtk_widget_set_sensitive(test_data->button, TRUE);
    gtk_button_set_label(GTK_BUTTON(test_data->button), "⚡ Speed Test");
    
    free(test_data);
    return FALSE;
}

// Automatic callback every second
static gboolean update_all_callback(gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    update_all_displays(widgets);
    return TRUE;
}

// Callback when clicking "About"
static void on_about_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    
    // Create a custom dialog to control labels
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "About SysWatch",
        NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 550);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 20);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    // Title and version
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), 
        "<span size='x-large' weight='bold'>SysWatch</span>\n"
        "<span size='small'>Version " TOSTRING(APP_VERSION) "</span>");
    gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 5);
    
    // Séparateur
    gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);
    
    // Description
    GtkWidget *desc = gtk_label_new(
        "System Information & Health Monitor for Linux\n\n"
        "Displays hardware specifications, real-time temperature monitoring,\n"
        "CPU/GPU usage, memory statistics, network bandwidth, and storage\n"
        "information with speed testing capabilities.\n\n"
        "Built with C and GTK3");
    gtk_label_set_justify(GTK_LABEL(desc), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(desc), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), desc, FALSE, FALSE, 5);
    
    // Separator
    gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);
    
    // Written by: (custom label)
    GtkWidget *credits_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(credits_label), "<b>Written by:</b>");
    gtk_widget_set_halign(credits_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), credits_label, FALSE, FALSE, 0);
    
    // Developer history
    GtkWidget *author_info = gtk_label_new(
        "Stéphane Corriveau\n"
        "Full-Stack Developer\n"
        "(Jack of all trades, master of some)\n\n"
        "Early Years: Vic-20, Commodore 64, Amiga 500\n"
        "University: Pascal, C/C++\n"
        "Early Career: Delphi, C++ Builder\n"
        "Mid Career: 10+ years SAP\n"
        "Recent 10 years: TypeScript, Angular, C#, Python\n"
        "Now: Exploring Linux systems\n\n"
        "Made with passion and fun ❤️");
    gtk_label_set_justify(GTK_LABEL(author_info), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(author_info, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), author_info, FALSE, FALSE, 5);
    
    // Separator
    gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);
    
    // Copyright and license
    GtkWidget *copyright = gtk_label_new(
        "Copyright © 2025 " TOSTRING(APP_AUTHOR) "\n"
        "Licensed under MIT License");
    gtk_label_set_justify(GTK_LABEL(copyright), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), copyright, FALSE, FALSE, 5);
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Callback when clicking "Quit"
static void on_quit_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    gtk_main_quit();
}

// Callback when clicking "Refresh" (refresh disk list)
static void on_storage_refresh_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    // Reset the physical storages list
    init_physical_storages(widgets);
}

// Callback when clicking "Speed Test" (disk)
static void on_storage_speed_test_clicked(GtkWidget *widget, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    if (widgets->storage_count == 0) {
        return;
    }
    
    // Disable button during test
    gtk_widget_set_sensitive(widget, FALSE);
    gtk_button_set_label(GTK_BUTTON(widget), "🔄 Testing...");
    
    // Create structure to pass data to thread
    DiskSpeedTestData *test_data = malloc(sizeof(DiskSpeedTestData));
    test_data->widgets = widgets;
    test_data->button = widget;
    
    // Initialize speeds
    for (int i = 0; i < widgets->storage_count; i++) {
        widgets->storages[i].read_speed = 0.0f;
        widgets->storages[i].write_speed = 0.0f;
    }
    
    // Launch test in a thread
    GThread *thread = g_thread_new("storage_speed_test", storage_speed_test_thread, test_data);
    g_thread_unref(thread);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get temperature color based on value (NZXT CAM style)
static const char* get_temperature_color(float temp_celsius) {
    if (temp_celsius < 60.0f) {
        return "#00FF00";  // Green: optimal
    } else if (temp_celsius < 75.0f) {
        return "#FFA500";  // Orange/Yellow: attention
    } else {
        return "#FF0000";  // Red: hot
    }
}

// Create a frame (frame) with title
static GtkWidget* create_frame(const char *title) {
    GtkWidget *frame = gtk_frame_new(title);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    return frame;
}

// ============================================================================
// PRIVATE FUNCTIONS - NETWORK INITIALIZATION
// ============================================================================

// Initialize network interface list (called once)
static void init_network_interfaces(AppWidgets *widgets) {
    if (widgets == NULL) {
        return;
    }
    
    // Create a SINGLE grid for the entire table (header + data)
    GtkWidget *table_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(table_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(table_grid), 5);
    
    // Ligne 0: Headers
    GtkWidget *header_iface = gtk_label_new("Interface");
    gtk_label_set_xalign(GTK_LABEL(header_iface), 0.0);
    
    GtkWidget *header_ip = gtk_label_new("IP Address");
    gtk_label_set_xalign(GTK_LABEL(header_ip), 0.0);
    gtk_widget_set_hexpand(header_ip, TRUE);
    
    GtkWidget *header_upload = gtk_label_new("Upload");
    gtk_label_set_xalign(GTK_LABEL(header_upload), 1.0);
    gtk_widget_set_hexpand(header_upload, TRUE);
    
    GtkWidget *header_download = gtk_label_new("Download");
    gtk_label_set_xalign(GTK_LABEL(header_download), 1.0);
    gtk_widget_set_hexpand(header_download, TRUE);
    
    gtk_grid_attach(GTK_GRID(table_grid), header_iface, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_ip, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_upload, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_download, 3, 0, 1, 1);
    
    // Ligne 1: Séparateur
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(table_grid), separator, 0, 1, 4, 1);
    
    // Parse interfaces and add to the same grid
    const char *interfaces_str = get_network_interfaces();
    char interfaces_copy[512];
    strncpy(interfaces_copy, interfaces_str, sizeof(interfaces_copy) - 1);
    interfaces_copy[sizeof(interfaces_copy) - 1] = '\0';
    
    char *token = strtok(interfaces_copy, ",");
    int row = 2;
    int interface_count = 0;
    
    // Allocate memory to store interfaces
    widgets->network_interface_count = 0;
    
    // Count interfaces first
    char interfaces_count_copy[512];
    strncpy(interfaces_count_copy, interfaces_str, sizeof(interfaces_count_copy) - 1);
    char *count_token = strtok(interfaces_count_copy, ",");
    while (count_token != NULL) {
        interface_count++;
        count_token = strtok(NULL, ",");
    }
    
    if (interface_count > 0) {
        widgets->network_interfaces = malloc(interface_count * sizeof(NetworkInterfaceWidgets));
        widgets->network_interface_count = interface_count;
    }
    
    // Reset token for traversal
    strncpy(interfaces_copy, interfaces_str, sizeof(interfaces_copy) - 1);
    token = strtok(interfaces_copy, ",");
    interface_count = 0;
    
    while (token != NULL) {
        // Clean spaces at start
        while (*token == ' ') token++;
        
        // Extract interface name (before parenthesis)
        char iface_name[64] = {0};
        char iface_type[64] = {0};
        sscanf(token, "%s (%[^)])", iface_name, iface_type);
        
        // Column 0: Icon (image) + interface name + type (in HBox)
        GtkWidget *iface_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_halign(iface_hbox, GTK_ALIGN_START);
        
        // Load icon from system theme
        const char *icon_name = "network-wired";  // Default
        if (strstr(iface_type, "WiFi") != NULL) {
            icon_name = "network-wireless";
        } else if (strstr(iface_type, "Ethernet") != NULL) {
            icon_name = "network-wired";
        } else if (strstr(iface_type, "Mobile") != NULL) {
            icon_name = "network-mobile";
        }
        
        GtkWidget *icon_image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
        
        // Create text for interface
        char iface_text[128];
        snprintf(iface_text, sizeof(iface_text), "%s (%s)", iface_name, iface_type);
        GtkWidget *iface_label = gtk_label_new(iface_text);
        gtk_label_set_xalign(GTK_LABEL(iface_label), 0.0);
        
        // Add icon and label to HBox
        gtk_box_pack_start(GTK_BOX(iface_hbox), icon_image, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(iface_hbox), iface_label, FALSE, FALSE, 0);
        
        // Column 1: IP Address
        GtkWidget *ip_label = gtk_label_new("Loading...");
        gtk_label_set_xalign(GTK_LABEL(ip_label), 0.0);
        gtk_widget_set_hexpand(ip_label, TRUE);
        
        // Column 2: Upload
        GtkWidget *upload_label = gtk_label_new("0 KB/s");
        gtk_label_set_xalign(GTK_LABEL(upload_label), 1.0);
        gtk_widget_set_hexpand(upload_label, TRUE);
        
        // Column 3: Download
        GtkWidget *download_label = gtk_label_new("0 KB/s");
        gtk_label_set_xalign(GTK_LABEL(download_label), 1.0);
        gtk_widget_set_hexpand(download_label, TRUE);
        
        // Store labels for later update
        if (interface_count < widgets->network_interface_count) {
            strncpy(widgets->network_interfaces[interface_count].interface_name, iface_name, 63);
            widgets->network_interfaces[interface_count].ip_label = ip_label;
            widgets->network_interfaces[interface_count].upload_label = upload_label;
            widgets->network_interfaces[interface_count].download_label = download_label;
        }
        
        // Attach to main grid
        gtk_grid_attach(GTK_GRID(table_grid), iface_hbox, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), ip_label, 1, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), upload_label, 2, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), download_label, 3, row, 1, 1);
        
        row++;
        interface_count++;
        token = strtok(NULL, ",");
    }
    
    // Add complete grid to vbox
    gtk_box_pack_start(GTK_BOX(widgets->network_vbox), table_grid, FALSE, FALSE, 2);
    
    gtk_widget_show_all(widgets->network_vbox);
}

// Initialize physical storage list (called once)
static void init_physical_storages(AppWidgets *widgets) {
    if (widgets == NULL) {
        return;
    }
    
    // Clear the box if it exists
    GList *children = gtk_container_get_children(GTK_CONTAINER(widgets->storage_vbox));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Get physical storages list
    int storage_count = 0;
    widgets->physical_storages = get_physical_storages(&storage_count);
    
    if (storage_count == 0 || widgets->physical_storages == NULL) {
        GtkWidget *no_storage_label = gtk_label_new("No physical storages found");
        gtk_box_pack_start(GTK_BOX(widgets->storage_vbox), no_storage_label, FALSE, FALSE, 2);
        gtk_widget_show_all(widgets->storage_vbox);
        return;
    }
    
    // Allocate memory for disk widgets
    widgets->storage_count = storage_count;
    widgets->storages = malloc(sizeof(StorageWidgets) * storage_count);
    
    // Create button box (above the table)
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_hexpand(button_box, TRUE);
    
    // Refresh button (on the left)
    GtkWidget *refresh_button = gtk_button_new_with_label("🔄 Refresh");
    g_signal_connect(refresh_button, "clicked",
                     G_CALLBACK(on_storage_refresh_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(button_box), refresh_button, FALSE, FALSE, 0);
    
    // Speed Test button (on the right)
    widgets->speed_test_button = gtk_button_new_with_label("⚡ Speed Test");
    g_signal_connect(widgets->speed_test_button, "clicked",
                     G_CALLBACK(on_storage_speed_test_clicked), widgets);
    gtk_box_pack_end(GTK_BOX(button_box), widgets->speed_test_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(widgets->storage_vbox), button_box, FALSE, FALSE, 5);
    
    // Create a SINGLE grid for the entire table (header + data)
    GtkWidget *table_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(table_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(table_grid), 5);
    
    // Ligne 0: Headers
    GtkWidget *header_name = gtk_label_new("Storage");
    gtk_label_set_xalign(GTK_LABEL(header_name), 0.0);
    
    GtkWidget *header_type = gtk_label_new("Type");
    gtk_label_set_xalign(GTK_LABEL(header_type), 0.0);
    gtk_widget_set_hexpand(header_type, FALSE);
    
    GtkWidget *header_interface = gtk_label_new("Interface");
    gtk_label_set_xalign(GTK_LABEL(header_interface), 0.0);
    gtk_widget_set_hexpand(header_interface, FALSE);
    
    GtkWidget *header_used = gtk_label_new("Used");
    gtk_label_set_xalign(GTK_LABEL(header_used), 1.0);
    gtk_widget_set_hexpand(header_used, TRUE);
    
    GtkWidget *header_available = gtk_label_new("Available");
    gtk_label_set_xalign(GTK_LABEL(header_available), 1.0);
    gtk_widget_set_hexpand(header_available, TRUE);
    
    GtkWidget *header_total = gtk_label_new("Total");
    gtk_label_set_xalign(GTK_LABEL(header_total), 1.0);
    gtk_widget_set_hexpand(header_total, TRUE);
    
    GtkWidget *header_usage = gtk_label_new("Usage");
    gtk_label_set_xalign(GTK_LABEL(header_usage), 1.0);
    gtk_widget_set_hexpand(header_usage, TRUE);
    
    GtkWidget *header_read = gtk_label_new("Read");
    gtk_label_set_xalign(GTK_LABEL(header_read), 1.0);
    gtk_widget_set_hexpand(header_read, TRUE);
    
    GtkWidget *header_write = gtk_label_new("Write");
    gtk_label_set_xalign(GTK_LABEL(header_write), 1.0);
    gtk_widget_set_hexpand(header_write, TRUE);
    
    gtk_grid_attach(GTK_GRID(table_grid), header_name, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_type, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_interface, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_used, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_available, 4, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_total, 5, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_usage, 6, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_read, 7, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table_grid), header_write, 8, 0, 1, 1);
    
    // Ligne 1: Séparateur
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(table_grid), separator, 0, 1, 9, 1);
    
    // Add each disk
    for (int i = 0; i < storage_count; i++) {
        PhysicalStorage *disk = &widgets->physical_storages[i];
        int row = i + 2;
        
        // Initialize structure
        memset(&widgets->storages[i], 0, sizeof(StorageWidgets));
        strncpy(widgets->storages[i].storage_name, disk->name, sizeof(widgets->storages[i].storage_name) - 1);
        
        // Column 0: Disk name
        GtkWidget *name_label = gtk_label_new(disk->name);
        gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);
        
        // Column 1: Type
        GtkWidget *type_label = gtk_label_new(disk->type);
        gtk_label_set_xalign(GTK_LABEL(type_label), 0.0);
        
        // Column 2: Interface
        GtkWidget *interface_label = gtk_label_new(disk->interface);
        gtk_label_set_xalign(GTK_LABEL(interface_label), 0.0);
        
        // Columns 3-8: Used, Available, Total, Usage, Read, Write
        char used_str[64], available_str[64], total_str[64], usage_str[64];
        
        // Display in MB if < 1 GB, otherwise in GB
        if (disk->used_gb < 1.0f) {
            snprintf(used_str, sizeof(used_str), "%.0f MB", disk->used_gb * 1024.0f);
        } else {
            snprintf(used_str, sizeof(used_str), "%.1f GB", disk->used_gb);
        }
        
        if (disk->available_gb < 1.0f) {
            snprintf(available_str, sizeof(available_str), "%.0f MB", disk->available_gb * 1024.0f);
        } else {
            snprintf(available_str, sizeof(available_str), "%.1f GB", disk->available_gb);
        }
        
        if (disk->capacity_gb < 1.0f) {
            snprintf(total_str, sizeof(total_str), "%.0f MB", disk->capacity_gb * 1024.0f);
        } else {
            snprintf(total_str, sizeof(total_str), "%.1f GB", disk->capacity_gb);
        }
        
        // Calculate usage percentage
        float usage_percent = 0.0f;
        if (disk->capacity_gb > 0) {
            usage_percent = (disk->used_gb / disk->capacity_gb) * 100.0f;
        }
        snprintf(usage_str, sizeof(usage_str), "%.1f%%", usage_percent);
        
        widgets->storages[i].used_label = gtk_label_new(used_str);
        gtk_label_set_xalign(GTK_LABEL(widgets->storages[i].used_label), 1.0);
        gtk_widget_set_hexpand(widgets->storages[i].used_label, TRUE);
        
        widgets->storages[i].available_label = gtk_label_new(available_str);
        gtk_label_set_xalign(GTK_LABEL(widgets->storages[i].available_label), 1.0);
        gtk_widget_set_hexpand(widgets->storages[i].available_label, TRUE);
        
        widgets->storages[i].total_label = gtk_label_new(total_str);
        gtk_label_set_xalign(GTK_LABEL(widgets->storages[i].total_label), 1.0);
        gtk_widget_set_hexpand(widgets->storages[i].total_label, TRUE);
        
        widgets->storages[i].percent_label = gtk_label_new(usage_str);
        gtk_label_set_xalign(GTK_LABEL(widgets->storages[i].percent_label), 1.0);
        gtk_widget_set_hexpand(widgets->storages[i].percent_label, TRUE);
        
        widgets->storages[i].read_label = gtk_label_new("NA");
        gtk_label_set_xalign(GTK_LABEL(widgets->storages[i].read_label), 1.0);
        gtk_widget_set_hexpand(widgets->storages[i].read_label, TRUE);
        
        widgets->storages[i].write_label = gtk_label_new("NA");
        gtk_label_set_xalign(GTK_LABEL(widgets->storages[i].write_label), 1.0);
        gtk_widget_set_hexpand(widgets->storages[i].write_label, TRUE);
        
        // Attacher les widgets au grid
        gtk_grid_attach(GTK_GRID(table_grid), name_label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), type_label, 1, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), interface_label, 2, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), widgets->storages[i].used_label, 3, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), widgets->storages[i].available_label, 4, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), widgets->storages[i].total_label, 5, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), widgets->storages[i].percent_label, 6, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), widgets->storages[i].read_label, 7, row, 1, 1);
        gtk_grid_attach(GTK_GRID(table_grid), widgets->storages[i].write_label, 8, row, 1, 1);
    }
    
    // Ajouter le grid à la boîte Disk
    gtk_box_pack_start(GTK_BOX(widgets->storage_vbox), table_grid, FALSE, FALSE, 2);
    
    gtk_widget_show_all(widgets->storage_vbox);
}

// ============================================================================
// FONCTIONS PUBLIQUES
// ============================================================================

// Créer et initialiser toute l'interface graphique
AppWidgets* create_gui(void) {
    AppWidgets *widgets = malloc(sizeof(AppWidgets));
    if (widgets == NULL) {
        return NULL;
    }
    
    // Initialiser les pointeurs réseau
    widgets->network_interfaces = NULL;
    widgets->network_interface_count = 0;
    
    // Initialiser les pointeurs disques
    widgets->storages = NULL;
    widgets->storage_count = 0;
    widgets->physical_storages = NULL;
    
    // -------- FENÊTRE PRINCIPALE --------
    widgets->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);  // [GTK] Créer fenêtre
    gtk_window_set_title(GTK_WINDOW(widgets->window), "SysWatch");  // [GTK]
    gtk_window_set_position(GTK_WINDOW(widgets->window), GTK_WIN_POS_CENTER);  // [GTK] Centrer
    gtk_container_set_border_width(GTK_CONTAINER(widgets->window), 10);  // [GTK] Marges
    gtk_window_set_resizable(GTK_WINDOW(widgets->window), TRUE);  // [GTK] Redimensionnable
    
    // [GTK] Connecter événement fermeture
    g_signal_connect(widgets->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // -------- CONTENEUR PRINCIPAL --------
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  // [GTK] VBox principal
    gtk_container_add(GTK_CONTAINER(widgets->window), main_vbox);  // [GTK] Ajouter à fenêtre
    
    // ============ SECTION 1: SYSTEM INFO (pleine largeur) ============
    GtkWidget *system_frame = create_frame("System Information");
    GtkWidget *system_grid = gtk_grid_new();  // [GTK] Grid pour 4 colonnes
    gtk_grid_set_column_spacing(GTK_GRID(system_grid), 60);  // [GTK] Espace entre colonnes
    gtk_grid_set_row_spacing(GTK_GRID(system_grid), 5);  // [GTK] Espace entre lignes
    gtk_container_add(GTK_CONTAINER(system_frame), system_grid);  // [GTK]
    gtk_container_set_border_width(GTK_CONTAINER(system_grid), 10);  // [GTK]
    
    // --- Colonne 0-1: Hardware Information (labels + valeurs) ---
    GtkWidget *hardware_lbl = gtk_label_new("System Model:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(hardware_lbl), 0.0);  // [GTK] Aligner à gauche
    widgets->hardware_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->hardware_label), 1.0);  // [GTK] Aligner à droite
    gtk_widget_set_hexpand(widgets->hardware_label, TRUE);  // [GTK] Expansion horizontale
    
    GtkWidget *processor_lbl = gtk_label_new("Processor:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(processor_lbl), 0.0);  // [GTK]
    widgets->processor_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->processor_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->processor_label, TRUE);  // [GTK]
    
    GtkWidget *architecture_lbl = gtk_label_new("Architecture:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(architecture_lbl), 0.0);  // [GTK]
    widgets->architecture_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->architecture_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->architecture_label, TRUE);  // [GTK]
    
    GtkWidget *cpu_cores_lbl = gtk_label_new("CPU Cores:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(cpu_cores_lbl), 0.0);  // [GTK]
    widgets->cpu_cores_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->cpu_cores_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->cpu_cores_label, TRUE);  // [GTK]
    
    GtkWidget *gpu_lbl = gtk_label_new("GPU:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(gpu_lbl), 0.0);  // [GTK]
    widgets->gpu_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->gpu_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->gpu_label, TRUE);  // [GTK]
    
    // --- Colonne 2-3: Software Information (labels + valeurs) ---
    GtkWidget *kernel_lbl = gtk_label_new("Kernel:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(kernel_lbl), 0.0);  // [GTK] Aligner à gauche
    widgets->kernel_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->kernel_label), 1.0);  // [GTK] Aligner à droite
    gtk_widget_set_hexpand(widgets->kernel_label, TRUE);  // [GTK] Expansion horizontale
    
    GtkWidget *distro_lbl = gtk_label_new("Distribution:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(distro_lbl), 0.0);  // [GTK]
    widgets->distro_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->distro_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->distro_label, TRUE);  // [GTK]
    
    GtkWidget *display_lbl = gtk_label_new("Desktop:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(display_lbl), 0.0);  // [GTK]
    widgets->display_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->display_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->display_label, TRUE);  // [GTK]
    
    GtkWidget *locale_lbl = gtk_label_new("Locale:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(locale_lbl), 0.0);  // [GTK]
    widgets->locale_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->locale_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->locale_label, TRUE);  // [GTK]
    
    GtkWidget *uptime_lbl = gtk_label_new("Uptime:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(uptime_lbl), 0.0);  // [GTK]
    widgets->uptime_label = gtk_label_new("Loading...");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->uptime_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->uptime_label, TRUE);  // [GTK]
    
    // Attacher les widgets au Grid (col, row, width, height)
    // Colonne 0-1: Hardware Info (4 lignes)
    gtk_grid_attach(GTK_GRID(system_grid), hardware_lbl, 0, 0, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->hardware_label, 1, 0, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), processor_lbl, 0, 1, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->processor_label, 1, 1, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), architecture_lbl, 0, 2, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->architecture_label, 1, 2, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), cpu_cores_lbl, 0, 3, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->cpu_cores_label, 1, 3, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), gpu_lbl, 0, 4, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->gpu_label, 1, 4, 1, 1);  // [GTK]
    
    // Colonne 2-3: Software Info (4 lignes)
    gtk_grid_attach(GTK_GRID(system_grid), kernel_lbl, 2, 0, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->kernel_label, 3, 0, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), distro_lbl, 2, 1, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->distro_label, 3, 1, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), display_lbl, 2, 2, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->display_label, 3, 2, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), locale_lbl, 2, 3, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->locale_label, 3, 3, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), uptime_lbl, 2, 4, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(system_grid), widgets->uptime_label, 3, 4, 1, 1);  // [GTK]
    
    gtk_box_pack_start(GTK_BOX(main_vbox), system_frame, FALSE, FALSE, 5);  // [GTK]
    
    // ============ SECTION 2: CPU | MEMORY ============
    GtkWidget *row2_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);  // [GTK] HBox
    gtk_box_pack_start(GTK_BOX(main_vbox), row2_hbox, TRUE, TRUE, 5);  // [GTK]
    
    // --- Cadre CPU ---
    GtkWidget *cpu_frame = create_frame("CPU");
    GtkWidget *cpu_grid = gtk_grid_new();  // [GTK] Utiliser Grid pour colonnes
    gtk_grid_set_column_spacing(GTK_GRID(cpu_grid), 60);  // [GTK] Espace entre colonnes
    gtk_grid_set_row_spacing(GTK_GRID(cpu_grid), 5);  // [GTK] Espace entre lignes
    gtk_container_add(GTK_CONTAINER(cpu_frame), cpu_grid);  // [GTK]
    gtk_container_set_border_width(GTK_CONTAINER(cpu_grid), 10);  // [GTK]
    
    // Colonne 0: Labels, Colonne 1: Valeurs
    GtkWidget *cpu_temp_lbl = gtk_label_new("Temperature:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(cpu_temp_lbl), 0.0);  // [GTK] Aligner à gauche
    widgets->temp_label = gtk_label_new("--°C");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->temp_label), 1.0);  // [GTK] Aligner à droite
    gtk_widget_set_hexpand(widgets->temp_label, TRUE);  // [GTK] Expansion horizontale
    
    GtkWidget *cpu_usage_lbl = gtk_label_new("CPU Usage:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(cpu_usage_lbl), 0.0);  // [GTK]
    widgets->cpu_usage_label = gtk_label_new("--%");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->cpu_usage_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->cpu_usage_label, TRUE);  // [GTK] Expansion horizontale
    
    GtkWidget *gpu_usage_lbl = gtk_label_new("GPU Usage:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(gpu_usage_lbl), 0.0);  // [GTK]
    widgets->gpu_usage_label = gtk_label_new("--%");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->gpu_usage_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->gpu_usage_label, TRUE);  // [GTK] Expansion horizontale
    
    gtk_grid_attach(GTK_GRID(cpu_grid), cpu_temp_lbl, 0, 0, 1, 1);  // [GTK] (col, row, width, height)
    gtk_grid_attach(GTK_GRID(cpu_grid), widgets->temp_label, 1, 0, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(cpu_grid), cpu_usage_lbl, 0, 1, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(cpu_grid), widgets->cpu_usage_label, 1, 1, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(cpu_grid), gpu_usage_lbl, 0, 2, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(cpu_grid), widgets->gpu_usage_label, 1, 2, 1, 1);  // [GTK]
    
    gtk_box_pack_start(GTK_BOX(row2_hbox), cpu_frame, TRUE, TRUE, 0);  // [GTK]
    
    // --- Cadre MEMORY ---
    GtkWidget *mem_frame = create_frame("Memory");
    GtkWidget *mem_grid = gtk_grid_new();  // [GTK] Utiliser Grid pour colonnes
    gtk_grid_set_column_spacing(GTK_GRID(mem_grid), 60);  // [GTK] Espace entre colonnes
    gtk_grid_set_row_spacing(GTK_GRID(mem_grid), 5);  // [GTK] Espace entre lignes
    gtk_container_add(GTK_CONTAINER(mem_frame), mem_grid);  // [GTK]
    gtk_container_set_border_width(GTK_CONTAINER(mem_grid), 10);  // [GTK]
    
    // Colonne 0: Labels, Colonne 1: Valeurs
    GtkWidget *mem_usage_lbl = gtk_label_new("Used:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(mem_usage_lbl), 0.0);  // [GTK]
    widgets->mem_usage_label = gtk_label_new("--%");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->mem_usage_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->mem_usage_label, TRUE);  // [GTK] Expansion horizontale
    
    GtkWidget *mem_available_lbl = gtk_label_new("Available:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(mem_available_lbl), 0.0);  // [GTK]
    widgets->mem_available_label = gtk_label_new("-- GB");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->mem_available_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->mem_available_label, TRUE);  // [GTK] Expansion horizontale
    
    GtkWidget *mem_total_lbl = gtk_label_new("Total:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(mem_total_lbl), 0.0);  // [GTK]
    widgets->mem_total_label = gtk_label_new("-- GB");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->mem_total_label), 1.0);  // [GTK]
    gtk_widget_set_hexpand(widgets->mem_total_label, TRUE);  // [GTK] Expansion horizontale
    
    gtk_grid_attach(GTK_GRID(mem_grid), mem_usage_lbl, 0, 0, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(mem_grid), widgets->mem_usage_label, 1, 0, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(mem_grid), mem_available_lbl, 0, 1, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(mem_grid), widgets->mem_available_label, 1, 1, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(mem_grid), mem_total_lbl, 0, 2, 1, 1);  // [GTK]
    gtk_grid_attach(GTK_GRID(mem_grid), widgets->mem_total_label, 1, 2, 1, 1);  // [GTK]
    
    gtk_box_pack_start(GTK_BOX(row2_hbox), mem_frame, TRUE, TRUE, 0);  // [GTK]
    
    // ============ SECTION 3: NETWORK | DISK (en vertical) ============
    GtkWidget *row3_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);  // [GTK] VBox
    gtk_box_pack_start(GTK_BOX(main_vbox), row3_vbox, TRUE, TRUE, 5);  // [GTK]
    
    // --- Cadre NETWORK ---
    GtkWidget *net_frame = create_frame("Network");
    GtkWidget *net_main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);  // [GTK]
    gtk_container_add(GTK_CONTAINER(net_frame), net_main_vbox);  // [GTK]
    gtk_container_set_border_width(GTK_CONTAINER(net_main_vbox), 10);  // [GTK]
    
    // Hostname en haut (centré)
    GtkWidget *hostname_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(hostname_hbox, GTK_ALIGN_CENTER);
    
    GtkWidget *net_hostname_lbl = gtk_label_new("Hostname:");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(net_hostname_lbl), 0.0);  // [GTK]
    widgets->network_hostname_label = gtk_label_new("--");  // [GTK]
    gtk_label_set_xalign(GTK_LABEL(widgets->network_hostname_label), 0.0);  // [GTK]
    
    gtk_box_pack_start(GTK_BOX(hostname_hbox), net_hostname_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hostname_hbox), widgets->network_hostname_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(net_main_vbox), hostname_hbox, FALSE, FALSE, 2);
    
    // Conteneur dynamique pour les interfaces (sera rempli dans update)
    widgets->network_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);  // [GTK]
    gtk_box_pack_start(GTK_BOX(net_main_vbox), widgets->network_vbox, TRUE, TRUE, 2);
    
    gtk_box_pack_start(GTK_BOX(row3_vbox), net_frame, TRUE, TRUE, 0);  // [GTK]
    
    // --- Cadre DISK ---
    GtkWidget *disk_frame = create_frame("Storage");
    widgets->storage_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);  // [GTK] VBox
    gtk_container_add(GTK_CONTAINER(disk_frame), widgets->storage_vbox);  // [GTK]
    gtk_container_set_border_width(GTK_CONTAINER(widgets->storage_vbox), 10);  // [GTK]
    
    gtk_box_pack_start(GTK_BOX(row3_vbox), disk_frame, TRUE, TRUE, 0);  // [GTK]
    
    // ============ SECTION 4: BOUTONS ============
    GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);  // [GTK]
    gtk_box_pack_start(GTK_BOX(main_vbox), button_hbox, FALSE, FALSE, 5);  // [GTK]
    
    widgets->about_button = gtk_button_new_with_label("ℹ️ About");  // [GTK]
    g_signal_connect(widgets->about_button, "clicked",  // [GTK]
                     G_CALLBACK(on_about_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_hbox), widgets->about_button, TRUE, TRUE, 5);  // [GTK]
    
    widgets->quit_button = gtk_button_new_with_label("❌ Quit");  // [GTK]
    g_signal_connect(widgets->quit_button, "clicked",  // [GTK]
                     G_CALLBACK(on_quit_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_hbox), widgets->quit_button, TRUE, TRUE, 5);  // [GTK]
    
    // -------- FINALISATION --------
    gtk_widget_show_all(widgets->window);  // [GTK] Afficher tout
    
    update_system_info_display(widgets);  // Lecture initiale System Info
    init_network_interfaces(widgets);     // Initialiser les interfaces réseau (une seule fois)
    init_physical_storages(widgets);         // Initialiser les disques physiques (une seule fois)
    update_all_displays(widgets);         // Lecture initiale données dynamiques
    
    // [GTK] Timer: mise à jour toutes les 2 secondes
    g_timeout_add(1000, update_all_callback, widgets);
    
    return widgets;
}

// Mettre à jour les débits réseau de chaque interface
static void update_network_bandwidth(AppWidgets *widgets) {
    if (widgets == NULL || widgets->network_interfaces == NULL) {
        return;
    }
    
    char buffer[64];
    
    for (int i = 0; i < widgets->network_interface_count; i++) {
        // Récupérer l'adresse IP
        const char *ip = get_interface_ip_address(widgets->network_interfaces[i].interface_name);
        gtk_label_set_text(GTK_LABEL(widgets->network_interfaces[i].ip_label), ip);
        
        // Récupérer les débits pour cette interface
        float upload = get_interface_upload_kbps(widgets->network_interfaces[i].interface_name);
        float download = get_interface_download_kbps(widgets->network_interfaces[i].interface_name);
        
        // Formater et afficher les débits
        snprintf(buffer, sizeof(buffer), "%.1f KB/s", upload);
        gtk_label_set_text(GTK_LABEL(widgets->network_interfaces[i].upload_label), buffer);
        
        snprintf(buffer, sizeof(buffer), "%.1f KB/s", download);
        gtk_label_set_text(GTK_LABEL(widgets->network_interfaces[i].download_label), buffer);
    }
}

// Mettre à jour uniquement la section System Info
void update_system_info_display(AppWidgets *widgets) {
    if (widgets == NULL) {
        return;
    }
    
    char buffer[256];
    
    // Hardware Info (colonne 1)
    snprintf(buffer, sizeof(buffer), "%s", get_hardware_model());
    gtk_label_set_text(GTK_LABEL(widgets->hardware_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%s", get_processor_type());
    gtk_label_set_text(GTK_LABEL(widgets->processor_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%s", get_architecture_info());
    gtk_label_set_text(GTK_LABEL(widgets->architecture_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%s", get_cpu_cores());
    gtk_label_set_text(GTK_LABEL(widgets->cpu_cores_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%s", get_gpu_info());
    gtk_label_set_text(GTK_LABEL(widgets->gpu_label), buffer);  // [GTK]
    
    // Software Info (colonne 2)
    snprintf(buffer, sizeof(buffer), "%s", get_kernel_version());
    gtk_label_set_text(GTK_LABEL(widgets->kernel_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%s", get_distro_info());
    gtk_label_set_text(GTK_LABEL(widgets->distro_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%s", get_desktop_environment());
    gtk_label_set_text(GTK_LABEL(widgets->display_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%s", get_locale_info());
    gtk_label_set_text(GTK_LABEL(widgets->locale_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%s", get_uptime_string());
    gtk_label_set_text(GTK_LABEL(widgets->uptime_label), buffer);  // [GTK]
}

// Mettre à jour tous les affichages
void update_all_displays(AppWidgets *widgets) {
    if (widgets == NULL) {
        return;
    }
    
    char buffer[128];
    
    // Processeur
    float temp = get_cpu_temperature_celsius();
    if (temp >= 0) {
        float temp_fahrenheit = (temp * 9.0f / 5.0f) + 32.0f;
        
        // Appliquer la couleur selon la température (style NZXT CAM)
        const char *color = get_temperature_color(temp);
        char markup[256];
        snprintf(markup, sizeof(markup), 
                 "<span foreground=\"%s\">%.1f°C (%.1f°F)</span>", 
                 color, temp, temp_fahrenheit);
        gtk_label_set_markup(GTK_LABEL(widgets->temp_label), markup);
    } else {
        snprintf(buffer, sizeof(buffer), "N/A");
        gtk_label_set_text(GTK_LABEL(widgets->temp_label), buffer);
    }
    
    snprintf(buffer, sizeof(buffer), "%.1f%%", get_cpu_usage_percent());
    gtk_label_set_text(GTK_LABEL(widgets->cpu_usage_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%.1f%%", get_gpu_usage_percent());
    gtk_label_set_text(GTK_LABEL(widgets->gpu_usage_label), buffer);  // [GTK]
    
    // Memory
    snprintf(buffer, sizeof(buffer), "%.1f%%", get_memory_usage_percent());
    gtk_label_set_text(GTK_LABEL(widgets->mem_usage_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%.1f GB", get_memory_available_gb());
    gtk_label_set_text(GTK_LABEL(widgets->mem_available_label), buffer);  // [GTK]
    
    snprintf(buffer, sizeof(buffer), "%.1f GB", get_memory_total_gb());
    gtk_label_set_text(GTK_LABEL(widgets->mem_total_label), buffer);  // [GTK]
    
    // System - Uptime (dynamic)
    snprintf(buffer, sizeof(buffer), "%s", get_uptime_string());
    gtk_label_set_text(GTK_LABEL(widgets->uptime_label), buffer);  // [GTK]
    
    // Network - Hostname
    snprintf(buffer, sizeof(buffer), "%s", get_hostname());
    gtk_label_set_text(GTK_LABEL(widgets->network_hostname_label), buffer);  // [GTK]
    
    // Network - Débits et IPs par interface
    update_network_bandwidth(widgets);
}

// Lancer la boucle principale GTK
void run_gui(AppWidgets *widgets) {
    if (widgets == NULL) {
        return;
    }
    gtk_main();  // [GTK] Boucle événements
}

// Libérer la mémoire
void cleanup_gui(AppWidgets *widgets) {
    if (widgets != NULL) {
        if (widgets->network_interfaces != NULL) {
            free(widgets->network_interfaces);
        }
        if (widgets->storages != NULL) {
            free(widgets->storages);
        }
        if (widgets->physical_storages != NULL) {
            free_physical_storages(widgets->physical_storages);
        }
        free(widgets);
        // Note: GTK gère automatiquement la mémoire de ses widgets
    }
}
