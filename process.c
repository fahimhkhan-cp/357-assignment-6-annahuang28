#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define MAX_TOKENS 100
#define MAX_RECORDS 4000  // Maximum number of records to store

// Define a structure to hold the processed demographic data
typedef struct {
    char county[100];
    char state[3];
    float education[2];
    float ethnicities[8];
    int median_household_income;
    int per_capita_income;
    float below_poverty_level;
    int population_2014;
} Demographics;

// Define a structure to hold the valid fields and print formats
typedef struct {
    const char **valid_fields;
    const char **print_formats;
    int valid_fields_count;
} Config;

// Global definition of valid fields and their corresponding print formats
const char *valid_fields[] = {
    "County", "State", "Education.Bachelor's Degree or Higher", "Education.High School or Higher",
    "Ethnicities.American Indian and Alaska Native Alone", "Ethnicities.Asian Alone", "Ethnicities.Black Alone",
    "Ethnicities.Hispanic or Latino", "Ethnicities.Native Hawaiian and Other Pacific Islander Alone",
    "Ethnicities.Two or More Races", "Ethnicities.White Alone", "Ethnicities.White Alone not Hispanic or Latino",
    "Income.Median Household Income", "Income.Per Capita Income", "Income.Persons Below Poverty Level", 
    "Population.2014 Population"
};

const char *print_formats[] = {
    "County: %s\n", "State: %s\n", "Education (Bachelor's Degree or Higher): %f%%\n",
    "Education (High School or Higher): %f%%\n", "Ethnicity (American Indian and Alaska Native Alone): %f%%\n", "Ethnicity (Asian Alone): %f%%\n",
    "Ethnicity (Black Alone): %f%%\n", "Ethnicity (Hispanic or Latino): %f%%\n", "Ethnicity (Native Hawaiian and Other Pacific Islander Alone: %f%%\n",
    "Ethnicity (Two or More Races): %f%%\n", "Ethnicity (White Alone): %f%%\n",
    "Ethnicity (White Alone not Hispanic or Latino): %f%%\n", "Median Household Income: %d\n",
    "Per Capita Income: %d\n", "Income Below Poverty Level: %f%%\n", "Population 2014: %d\n\n"
};

// Function to parse a single CSV token, handling quoted fields correctly.
char *parse_csv_token(char *line) {
    static char *ptr = NULL;
    if (line != NULL) {
        ptr = line;  // Reset pointer for new line
    }
    if (ptr == NULL || *ptr == '\0') return NULL;

    // Skip leading spaces
    while (*ptr == ' ' || *ptr == '\t') ptr++;

    char *start = ptr;

    // Handle quoted fields
    if (*ptr == '"') {
        // Skip opening quote
        start = ++ptr;
        // Find the closing quote
        while (*ptr && *ptr != '"') ptr++;
        if (*ptr) ptr++;  // Skip closing quote
    } else {
        // Find next comma or end of string
        while (*ptr && *ptr != ',') ptr++;
    }

    // Null-terminate the token and return it
    if (*ptr == ',') {
        *ptr++ = '\0';
    }

    // Strip any trailing quotes that may have been incorrectly included in the token
    if (*start == '"') {
        start++;  // Skip the leading quote
    }
    char *end = start + strlen(start) - 1;
    if (*end == '"') {
        *end = '\0';  // Remove the trailing quote
    }

    return start;
}

// Function to strip leading and trailing spaces from a string
void strip_spaces(char *str) {
    char *end;

    // Trim leading spaces
    while (isspace((unsigned char)*str)) str++;

    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
}

// Function to read header line and find the required field indices
int read_header(FILE *file, char **header_tokens, int *field_indices, const char **valid_fields, int valid_fields_count) {
    char *line = malloc(2048 * sizeof(char));  // Increased line buffer size
    if (line == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }

    char *token;
    int column_count = 0;

    // Read the header line
    if (fgets(line, 2048, file) == NULL) {
        free(line);
        return 0;
    }

    token = parse_csv_token(line);

    // Process each token in the header
    while (token != NULL) {
        strip_spaces(token);
        header_tokens[column_count] = strdup(token);  // Store the token

        // Compare the token to valid field names and store the column index
        for (int i = 0; i < valid_fields_count; i++) {
            if (strcmp(token, valid_fields[i]) == 0) {
                field_indices[i] = column_count;
                break;
            }
        }

        token = parse_csv_token(NULL);
        column_count++;
    }

    free(line);
    return 1;
}

int process_demographics_file(const char *demographics_file, Demographics *data, int max_records, Config *config) {
    FILE *file = fopen(demographics_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file: %s\n", demographics_file);
        return -1;
    }

    char *line = malloc(2048 * sizeof(char));  // Increased line buffer size
    if (line == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    // Allocate memory for header tokens and field indices
    char **header_tokens = malloc(MAX_TOKENS * sizeof(char *));
    int *field_indices = malloc(config->valid_fields_count * sizeof(int));
    for (int i = 0; i < config->valid_fields_count; i++) {
        field_indices[i] = -1;
    }

    // Read the header line and find the indices of required fields
    if (!read_header(file, header_tokens, field_indices, config->valid_fields, config->valid_fields_count)) {
        fprintf(stderr, "Failed to read header or invalid format\n");
        free(line);
        free(header_tokens);
        free(field_indices);
        fclose(file);
        return -1;
    }

    // Process each row in the demographics file
    int record_count = 0;
    int line_number = 1;  // Start counting from the first data line
    while (fgets(line, 2048, file) && record_count < max_records) {
        Demographics *record = &data[record_count];
        char *token;
        int column_count = 0;
        int malformed_entry = 0;  // Flag to detect malformed entries

        token = parse_csv_token(line);
        while (token != NULL) {
            strip_spaces(token);

            // Store values in the appropriate fields of the Demographics struct
            for (int i = 0; i < config->valid_fields_count; i++) {
                if (column_count == field_indices[i]) {
                    switch (i) {
                        case 0: strcpy(record->county, token); break;
                        case 1: strcpy(record->state, token); break;
                        case 2: 
                            if (sscanf(token, "%f", &record->education[0]) != 1) malformed_entry = 1; break;
                        case 3: 
                            if (sscanf(token, "%f", &record->education[1]) != 1) malformed_entry = 1; break;
                        case 4: 
                            if (sscanf(token, "%f", &record->ethnicities[0]) != 1) malformed_entry = 1; break;
                        case 5: 
                            if (sscanf(token, "%f", &record->ethnicities[1]) != 1) malformed_entry = 1; break;
                        case 6: 
                            if (sscanf(token, "%f", &record->ethnicities[2]) != 1) malformed_entry = 1; break;
                        case 7: 
                            if (sscanf(token, "%f", &record->ethnicities[3]) != 1) malformed_entry = 1; break;
                        case 8: 
                            if (sscanf(token, "%f", &record->ethnicities[4]) != 1) malformed_entry = 1; break;
                        case 9: 
                            if (sscanf(token, "%f", &record->ethnicities[5]) != 1) malformed_entry = 1; break;
                        case 10: 
                            if (sscanf(token, "%f", &record->ethnicities[6]) != 1) malformed_entry = 1; break;
                        case 11: 
                            if (sscanf(token, "%f", &record->ethnicities[7]) != 1) malformed_entry = 1; break;
                        case 12: 
                            if (sscanf(token, "%d", &record->median_household_income) != 1) malformed_entry = 1; break;
                        case 13: 
                            if (sscanf(token, "%d", &record->per_capita_income) != 1) malformed_entry = 1; break;
                        case 14: 
                            if (sscanf(token, "%f", &record->below_poverty_level) != 1) malformed_entry = 1; break;
                        case 15: 
                            if (sscanf(token, "%d", &record->population_2014) != 1) malformed_entry = 1; break;
                    }
                    break;
                }
            }

            token = parse_csv_token(NULL);
            column_count++;
        }

        if (malformed_entry) {
            fprintf(stderr, "Malformed entry at line %d. Skipping entry.\n", line_number);
            line_number++;
            continue;  // Skip the current record and move to the next line
        }

        record_count++;
        line_number++;  // Increment line number
    }

    free(line);
    free(header_tokens);
    free(field_indices);
    fclose(file);

    return record_count;
}


// Function to display the data
void display(Demographics *data, int record_count, const char **print_formats) {
    for (int i = 0; i < record_count; i++) {
        Demographics *record = &data[i];
        printf(print_formats[0], record->county);
        printf(print_formats[1], record->state);
        printf(print_formats[2], record->education[0]);
        printf(print_formats[3], record->education[1]);
        printf(print_formats[4], record->ethnicities[0]);
        printf(print_formats[5], record->ethnicities[1]);
        printf(print_formats[6], record->ethnicities[2]);
        printf(print_formats[7], record->ethnicities[3]);
        printf(print_formats[8], record->ethnicities[4]);
        printf(print_formats[9], record->ethnicities[5]);
        printf(print_formats[10], record->ethnicities[6]);
        printf(print_formats[11], record->ethnicities[7]);
        printf(print_formats[12], record->median_household_income);
        printf(print_formats[13], record->per_capita_income);
        printf(print_formats[14], record->below_poverty_level);
        printf(print_formats[15], record->population_2014);
    }
}

// Function to filter by state abbreviation
void filter_state(Demographics *data, int *record_count, const char *state_abbr) {
    int filtered_count = 0;
    
    // Iterate over records and filter based on state
    for (int i = 0; i < *record_count; i++) {
        if (strcmp(data[i].state, state_abbr) == 0) {
            data[filtered_count++] = data[i];  // Copy matching record to the front
        }
    }

    // Update record count to the new filtered count
    *record_count = filtered_count;  // Dereference to update the actual value

    // Print the result
    printf("Filter: state == %s (%d entries)\n", state_abbr, *record_count);  // Dereference to print the value
}

void filter_field(Demographics *data, int *record_count, const char *field, const char *comparison, double number, const char **valid_fields) {
    int field_index = -1;
    // Find the field index
    for (int i = 0; i < MAX_TOKENS; i++) {
        if (strcmp(field, valid_fields[i]) == 0) {
            field_index = i;
            break;
        }
    }

    if (field_index == -1) {
        fprintf(stderr, "Field not found: %s\n", field);
        return;
    }

    // Filter based on the field type
    int filtered_count = 0;
    for (int i = 0; i < *record_count; i++) {
        Demographics *record = &data[i];
        double value = -1;

        // Extract the appropriate field value
        switch (field_index) {
            case 2: value = record->education[0]; break;
            case 3: value = record->education[1]; break;
            case 4: value = record->ethnicities[0]; break;
            case 5: value = record->ethnicities[1]; break;
            case 6: value = record->ethnicities[2]; break;
            case 7: value = record->ethnicities[3]; break;
            case 8: value = record->ethnicities[4]; break;
            case 9: value = record->ethnicities[5]; break;
            case 10: value = record->ethnicities[6]; break;
            case 11: value = record->ethnicities[7]; break;
            case 12: value = record->median_household_income; break;
            case 13: value = record->per_capita_income; break;
            case 14: value = record->below_poverty_level; break;
            case 15: value = (double) record->population_2014; break;
            default: continue;  // Non-numeric fields, skip
        }

        // Apply the filter based on comparison
        int condition_met = 0;
        if (strcmp(comparison, "ge") == 0) {
            condition_met = value >= number;
        } else if (strcmp(comparison, "le") == 0) {
            condition_met = value <= number;
        }

        if (condition_met) {
            data[filtered_count++] = data[i];  // Copy matching record to the front
        }
    }

    // Update record count and print the result
    *record_count = filtered_count;

    // Corrected filter print output
    printf("Filter: %s %s %.2f (%d entries)\n", field, comparison, number, *record_count);
}

// Function to print the total 2014 population across all entries
void population_total(Demographics *data, int *record_count) {
    int total_population = 0;
    for (int i = 0; i < *record_count; i++) {
        total_population += data[i].population_2014;
    }
    printf("2014 population: %d\n", total_population);
}

// Helper function to compute the sub-population based on the field for a record
float compute_sub_population(Demographics *record, const char *field) {
    if (strcmp(field, "Education.Bachelor's Degree or Higher") == 0) {
        return (record->education[0] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Education.High School or Higher") == 0) {
        return (record->education[1] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Ethnicities.American Indian and Alaska Native Alone") == 0) {
        return (record->ethnicities[0] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Ethnicities.Asian Alone") == 0) {
        return (record->ethnicities[1] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Ethnicities.Black Alone") == 0) {
        return (record->ethnicities[2] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Ethnicities.Hispanic or Latino") == 0) {
        return (record->ethnicities[3] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Ethnicities.Native Hawaiian and Other Pacific Islander Alone") == 0) {
        return (record->ethnicities[4] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Ethnicities.Two or More Races") == 0) {
        return (record->ethnicities[5] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Ethnicities.White Alone") == 0) {
        return (record->ethnicities[6] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Ethnicities.White Alone, not Hispanic or Latino") == 0) {
        return (record->ethnicities[7] / 100.0) * record->population_2014;
    } else if (strcmp(field, "Income.Persons Below Poverty Level") == 0) {
        return (record->below_poverty_level / 100.0) * record->population_2014;
    } else {
        printf("Unknown field: %s\n", field);
        return 0.0;
    }
}

// Modified population_field using the helper function
void population_field(Demographics *data, int *record_count, const char *field) {
    float total_population = 0.0;

    for (int i = 0; i < *record_count; i++) {
        total_population += compute_sub_population(&data[i], field); // Get sub-population for each record
    }

    printf("2014 %s population: %f\n", field, total_population);
}

// Modified percent_field using the helper function
void percent_field(Demographics *data, int *record_count, const char *field) {
    float total_population = 0.0;
    float sub_population = 0.0;

    // Calculate total population
    for (int i = 0; i < *record_count; i++) {
        total_population += data[i].population_2014;
    }

    // Calculate sub-population for the specified field
    for (int i = 0; i < *record_count; i++) {
        sub_population += compute_sub_population(&data[i], field);
    }

    if (total_population > 0) {
        float percentage = (sub_population / total_population) * 100;
        printf("2014 %s percentage: %f\n", field, percentage);
    } else {
        printf("Total population is 0, cannot compute percentage.\n");
    }
}

// Function to process the operations file
void process_operations(const char *operations_file, Demographics *data, int *record_count) {
    FILE *file = fopen(operations_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file: %s\n", operations_file);
        return;
    }

    char line[2048];
    int line_number = 0;
    int display_flag = 0;

    // Read the operations file line by line
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        strip_spaces(line);  // Strip any leading/trailing spaces

        if (strlen(line) == 0) {
            // Skip empty lines
            continue;
        }
        if (strstr(line, "display")) {
            display_flag = 1;
        } else if (strstr(line, "filter-state:")) {
            char state_abbr[3];
            sscanf(line, "filter-state:%2s", state_abbr);
            filter_state(data, record_count, state_abbr);  // Pass the address of record_count
        } else if (strstr(line, "filter:")) {
            char field[100], comparison[3];
            double number;
            // Parse the filter operation line
            sscanf(line, "filter:%99[^:]:%2s:%lf", field, comparison, &number);
            if ((strcmp(field, "County") != 0) && (strcmp(field, "State") != 0)) {
                filter_field(data, record_count, field, comparison, number, valid_fields);
            } else {
                printf("Not a valid field.");
            }
        } else if (strstr(line, "population-total")) {
            population_total(data, record_count);
        } else if (strstr(line, "population:")) {
            char field[100];
            sscanf(line, "population:%99[^\n]", field);
            if ((strcmp(field, "County") != 0) && (strcmp(field, "State") != 0) 
                && (strcmp(field, "Income.Per Capita Income") != 0) 
                && (strcmp(field, "Income.Median Household Income") != 0)
                && (strcmp(field, "Population.2014 Population") != 0)) {
                population_field(data, record_count, field);
            } else {
                printf("Not a viable field for this.");
            }
        }else if (strstr(line, "percent:")) {
            char field[100];
            sscanf(line, "percent:%99[^\n]", field);
            percent_field(data, record_count, field);  // Call the new function for percentage calculation
        } else {
            fprintf(stderr, "Error processing line %d: Invalid filter format.\n", line_number);
            continue;
        }
    }
    // If "display" is found, call the display function
    if (display_flag) {
        display(data, *record_count, print_formats);  // Dereference to get the value
    }

    fclose(file);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <demographics_file> <operations_file>\n", argv[0]);
        return 1;
    }

    const char *demographics_file = argv[1];
    const char *operations_file = argv[2];

    // Create a Config struct for valid fields and formats
    Config config = { valid_fields, print_formats, sizeof(valid_fields) / sizeof(valid_fields[0]) };

    // Allocate memory for storing demographics data
    Demographics data[MAX_RECORDS];

    // Process the demographics file and store the data
    int record_count = process_demographics_file(demographics_file, data, MAX_RECORDS, &config);
    if (record_count == -1) {
        return 1;  // Error loading demographics data
    }

    printf("%d records loaded\n", record_count);

    // Process the operations file (including displaying data if requested)
    process_operations(operations_file, data, &record_count);

    return 0;
}
