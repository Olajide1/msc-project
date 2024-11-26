#include <gtk/gtk.h>
#include "../data_io/cJSON.h"
#include "../data_io/data-processor.h"

#define NORMAL_SENSOR_DATA_FILENAME "../mqtt-client/normal_sim.json"
#define FIRE_SENSOR_DATA_FILENAME "../mqtt-client/fire_sim.json"

/* Create labels to display sensor values */
static GtkWidget *lpg_label;
static GtkWidget *co_label;
static GtkWidget *smoke_label;
static GtkWidget *temperature_label;
static GtkWidget *humidity_label;

typedef struct {
    char activeSignalType[50];
    char activeFileName[100];
    int activeIndex;
} AppLogger;

FireData sensor_data = {0};
AppLogger logger;

static void pull_sensor_data(char *filename, int json_length) {
    if (strcmp(logger.activeFileName, filename) == 0) {
        if (logger.activeIndex + 1 < json_length) {
            logger.activeIndex = logger.activeIndex + 1;
        } else {
            logger.activeIndex = 0;
        }
    } else {
        strcpy(logger.activeFileName, filename);
        logger.activeIndex = 0;
    }
    sensor_data = read_fire_data(logger.activeFileName, logger.activeIndex);
}

/* Function to update sensor labels */
static void update_labels() {
    char temp_str[32], hum_str[32], smk_str[32], co_str[32], lpg_str[32];

    snprintf(lpg_str, sizeof(lpg_str), "LPG: %.2f kg/m3", sensor_data.LPG);
    snprintf(co_str, sizeof(co_str), "CO: %.2f ppm", sensor_data.CO);
    snprintf(smk_str, sizeof(smk_str), "Smoke: %.2f μg/m3", sensor_data.Smoke);
    snprintf(temp_str, sizeof(temp_str), "Temperature: %.2f °C", sensor_data.Temperature);
    snprintf(hum_str, sizeof(hum_str), "Humidity: %.2f RH", sensor_data.Humidity);

    gtk_label_set_text(GTK_LABEL(lpg_label), lpg_str);
    gtk_label_set_text(GTK_LABEL(co_label), co_str);
    gtk_label_set_text(GTK_LABEL(smoke_label), smk_str);
    gtk_label_set_text(GTK_LABEL(temperature_label), temp_str);
    gtk_label_set_text(GTK_LABEL(humidity_label), hum_str);

    /* Save updated sensor data */
    char json_data[100];
    snprintf(json_data, sizeof(json_data),
             "{\"LPG\": %.2f, \"CO\": %.2f, \"Smoke\": %.2f, \"Temperature\": %.2f, \"Humidity\": %.2f}",
             sensor_data.LPG, sensor_data.CO, sensor_data.Smoke, sensor_data.Temperature, sensor_data.Humidity);
    write_json_data_to_file(json_data);
}

/* Callback for Increase button */
static void on_fire_sensor_data_requested_button_clicked(GtkWidget *widget, gpointer data) {
    int json_size = get_json_array_size(FIRE_SENSOR_DATA_FILENAME);
    pull_sensor_data(FIRE_SENSOR_DATA_FILENAME, json_size);
    update_labels();
}

/* Callback for Decrease button */
static void on_non_fire_sensor_data_requested_button_clicked(GtkWidget *widget, gpointer data) {
    int json_size = get_json_array_size(NORMAL_SENSOR_DATA_FILENAME);
    pull_sensor_data(NORMAL_SENSOR_DATA_FILENAME, json_size);
    update_labels();
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *title_label;
    GtkWidget *increase_button, *decrease_button;

    // Initialize struct members
    strcpy(logger.activeSignalType, "normal");
    strcpy(logger.activeFileName, FIRE_SENSOR_DATA_FILENAME);
    logger.activeIndex = 0;

    /* Initialize GTK */
    gtk_init(&argc, &argv);

    /* Create a new window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IoT Data Simulation");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Create a grid layout */
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);  /* Add spacing between rows */
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20); /* Add spacing between columns */
    gtk_container_add(GTK_CONTAINER(window), grid);

    /* Create title label */
    title_label = gtk_label_new("<b><big>IoT Data Controller</big></b>");
    gtk_label_set_use_markup(GTK_LABEL(title_label), TRUE);
    gtk_grid_attach(GTK_GRID(grid), title_label, 0, 0, 2, 1);  /* Attach title label */

    /* Create sensor labels */

    lpg_label = gtk_label_new("LPG: 300 Pa");
    co_label = gtk_label_new("CO: 300 Pa");
    smoke_label = gtk_label_new("Smoke: 300 Pa");
    temperature_label = gtk_label_new("Temperature: 20 °C");
    humidity_label = gtk_label_new("Humidity: 100 RH");

    /* Add sensor labels to the grid */
    gtk_grid_attach(GTK_GRID(grid), temperature_label, 0, 1, 2, 1); 
    gtk_grid_attach(GTK_GRID(grid), humidity_label, 0, 2, 2, 1);  
    gtk_grid_attach(GTK_GRID(grid), smoke_label, 0, 3, 2, 1);  
    gtk_grid_attach(GTK_GRID(grid), lpg_label, 0, 4, 2, 1); 
    gtk_grid_attach(GTK_GRID(grid), co_label, 0, 5, 2, 1); 

    int json_size = get_json_array_size(NORMAL_SENSOR_DATA_FILENAME);
    pull_sensor_data(NORMAL_SENSOR_DATA_FILENAME, json_size);
    update_labels();

    /* Create Increase button */
    increase_button = gtk_button_new_with_label("Send Fire Signal Sensor Data");
    g_signal_connect(increase_button, "clicked", G_CALLBACK(on_fire_sensor_data_requested_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), increase_button, 0, 6, 1, 1);

    /* Create Decrease button */
    decrease_button = gtk_button_new_with_label("Send Normal Signal Sensor Data");
    g_signal_connect(decrease_button, "clicked", G_CALLBACK(on_non_fire_sensor_data_requested_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), decrease_button, 1, 6, 1, 1);

    /* Show all widgets */
    gtk_widget_show_all(window);

    /* Main GTK loop */
    gtk_main();

    return 0;
}
