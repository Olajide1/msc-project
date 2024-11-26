#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

    #include "cJSON.h" // Include the cJSON header

    typedef struct {
        double LPG;
        double CO;
        double Smoke;
        double Temperature;
        double Humidity;
    } FireData;

    /* Writes JSON data to a file */
    void write_json_data_to_file(const char *json_data);

    /* Reads JSON data from a file */
    char* read_json_data_from_file();

    /* Generates JSON data based on sensor values */
    char* generate_json_data(int temperature, int humidity, int pressure);

    /* Get the size of the JSON array in a file */
    int get_json_array_size(const char *filename);

    /* Read a specific item's data from a JSON array in a file */
    FireData read_fire_data(const char *filename, int index);

#endif  // DATA_PROCESSOR_H
