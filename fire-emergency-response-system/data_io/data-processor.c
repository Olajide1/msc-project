#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data-processor.h"

#define SENSOR_DATA_FILENAME "../mqtt-client/sensor_data.json"

// Function to write JSON data to a file
void write_json_data_to_file(const char *json_data) {
    FILE *file = fopen(SENSOR_DATA_FILENAME, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    size_t written_size = fwrite(json_data, sizeof(char), strlen(json_data), file);
    if (written_size != strlen(json_data)) {
        perror("Error writing JSON data to file");
    }

    fclose(file);
    printf("JSON data written to %s successfully.\n", SENSOR_DATA_FILENAME);
}

// Function to read JSON data from a file and return it
char* read_json_data_from_file() {
    FILE *file = fopen(SENSOR_DATA_FILENAME, "r");
    if (file == NULL) {
        perror("Error opening file for reading");
        return NULL;
    }

    // Move file pointer to the end to determine the file size
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Error seeking in file");
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);
    if (file_size == -1L) {
        perror("Error getting file size");
        fclose(file);
        return NULL;
    }

    rewind(file);  // Move file pointer back to the beginning

    // Allocate memory to hold the file contents, including space for the null terminator
    char *json_data = (char *)malloc(sizeof(char) * (file_size + 1));
    if (json_data == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return NULL;
    }

    // Read the file contents
    size_t read_size = fread(json_data, sizeof(char), file_size, file);
    if (read_size != file_size) {
        perror("Error reading JSON data from file");
        free(json_data);
        fclose(file);
        return NULL;
    }

    json_data[file_size] = '\0';  // Null-terminate the string

    fclose(file);
    return json_data;
}

// Function to generate and return a JSON string
char* generate_json_data(int temperature, int humidity, int pressure) {
    // Calculate the required size of the JSON string dynamically
    size_t json_size = snprintf(NULL, 0, "{\"Temperature\": %d, \"Humidity\": %d, \"Pressure\": %d}", temperature, humidity, pressure);
    
    // Allocate memory based on calculated size
    char *json_data = (char *)malloc((json_size + 1) * sizeof(char));  // +1 for null terminator
    if (json_data == NULL) {
        perror("Memory allocation error");
        return NULL;
    }

    // Format the JSON string
    snprintf(json_data, json_size + 1, "{\"Temperature\": %d, \"Humidity\": %d, \"Pressure\": %d}", temperature, humidity, pressure);

    return json_data;  // Return the generated JSON string (must be freed by the caller)
}

FireData read_fire_data(const char *filename, int index) {
    FireData data = {0}; // Initialize struct with default values

    // Open the JSON file
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open file");
        return data;
    }

    // Read the entire file into a string
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *fileContents = malloc(fileSize + 1);
    if (!fileContents) {
        perror("Memory allocation failed");
        fclose(file);
        return data;
    }
    fread(fileContents, 1, fileSize, file);
    fileContents[fileSize] = '\0';
    fclose(file);

    // Parse the JSON
    cJSON *jsonArray = cJSON_Parse(fileContents);
    free(fileContents);

    if (!jsonArray || !cJSON_IsArray(jsonArray)) {
        fprintf(stderr, "Invalid JSON format\n");
        if (jsonArray) cJSON_Delete(jsonArray);
        return data;
    }

    // Get the item at the specified index
    cJSON *item = cJSON_GetArrayItem(jsonArray, index);
    if (item) {
        data.CO = cJSON_GetObjectItem(item, "CO")->valuedouble;
        data.Humidity = cJSON_GetObjectItem(item, "Humidity")->valuedouble;
        data.LPG = cJSON_GetObjectItem(item, "LPG")->valuedouble;
        data.Smoke = cJSON_GetObjectItem(item, "Smoke")->valuedouble;
        data.Temperature = cJSON_GetObjectItem(item, "Temperature")->valuedouble;
    } else {
        fprintf(stderr, "Index %d out of bounds\n", index);
    }

    // Clean up
    cJSON_Delete(jsonArray);

    return data;
}

// Function to get the size of the JSON array
int get_json_array_size(const char *filename) {
    // Open the JSON file
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open file");
        return -1; // Indicate error
    }

    // Read the entire file into a string
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *fileContents = malloc(fileSize + 1);
    if (!fileContents) {
        perror("Memory allocation failed");
        fclose(file);
        return -1; // Indicate error
    }
    fread(fileContents, 1, fileSize, file);
    fileContents[fileSize] = '\0';
    fclose(file);

    // Parse the JSON
    cJSON *jsonArray = cJSON_Parse(fileContents);
    free(fileContents);

    if (!jsonArray || !cJSON_IsArray(jsonArray)) {
        fprintf(stderr, "Invalid JSON format or not an array\n");
        if (jsonArray) cJSON_Delete(jsonArray);
        return -1; // Indicate error
    }

    // Get the array size
    int size = cJSON_GetArraySize(jsonArray);

    // Clean up
    cJSON_Delete(jsonArray);

    return size;
}
