#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE_LEN 1024
#define MAX_FIELD_LEN 50
#define MAX_ENTRIES 10000
#define MAX_OP_LEN 100

// Struct to store demographic data
typedef struct {
    char county[100];
    char state[3];
    float education[2];  // [Bachelor's Degree or Higher, High School or Higher]
    float ethnicities[8];  // [American Indian, Asian, Black, Hispanic, Native Hawaiian, Two or More, White Alone, White Alone not Hispanic]
    int income_median_household;
    int income_per_capita;
    float income_below_poverty;
    int population_2014;
} Demographics;

// Array of valid field names (with corresponding indices)
const char *valid_fields[] = {
    "Education.Bachelor's Degree or Higher",
    "Education.High School or Higher",
    "Ethnicities.American Indian and Alaska Native Alone",
    "Ethnicities.Asian Alone",
    "Ethnicities.Black Alone",
    "Ethnicities.Hispanic or Latino",
    "Ethnicities.Native Hawaiian and Other Pacific Islander Alone",
    "Ethnicities.Two or More Races",
    "Ethnicities.White Alone",
    "Ethnicities.White Alone not Hispanic or Latino",
    "Income.Median Household Income",
    "Income.Per Capita Income",
    "Income.Persons Below Poverty Level",
    "Population.2014 Population",
};

// Function to get the field index by comparing field names
int get_field_index(const char *field) {
    for (int i = 0; i < sizeof(valid_fields) / sizeof(valid_fields[0]); i++) {
        if (strcmp(valid_fields[i], field) == 0) {
            return i;
        }
    }
    return -1;  // Return -1 if the field is invalid
}

// Function to parse a line of CSV and populate Demographics structure
int parse_line(char *line, Demographics *data) {
    char *token;
    int field_idx = 0;

    token = strtok(line, ",");
    while (token != NULL) {
        // Remove surrounding quotes
        if (token[0] == '"') {
            token++;
            token[strlen(token) - 1] = '\0';
        }

        // Ensure that field_idx doesn't exceed the size of valid_fields
        if (field_idx < sizeof(valid_fields) / sizeof(valid_fields[0])) {
            // Compare the field name with the valid fields
            if (strcmp(valid_fields[field_idx], "Education.Bachelor's Degree or Higher") == 0) {
                data->education[0] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Education.High School or Higher") == 0) {
                data->education[1] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Ethnicities.American Indian and Alaska Native Alone") == 0) {
                data->ethnicities[0] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Ethnicities.Asian Alone") == 0) {
                data->ethnicities[1] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Ethnicities.Black Alone") == 0) {
                data->ethnicities[2] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Ethnicities.Hispanic or Latino") == 0) {
                data->ethnicities[3] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Ethnicities.Native Hawaiian and Other Pacific Islander Alone") == 0) {
                data->ethnicities[4] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Ethnicities.Two or More Races") == 0) {
                data->ethnicities[5] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Ethnicities.White Alone") == 0) {
                data->ethnicities[6] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Ethnicities.White Alone not Hispanic or Latino") == 0) {
                data->ethnicities[7] = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Income.Median Household Income") == 0) {
                data->income_median_household = atoi(token);
            } else if (strcmp(valid_fields[field_idx], "Income.Per Capita Income") == 0) {
                data->income_per_capita = atoi(token);
            } else if (strcmp(valid_fields[field_idx], "Income.Persons Below Poverty Level") == 0) {
                data->income_below_poverty = atof(token);
            } else if (strcmp(valid_fields[field_idx], "Population.2014 Population") == 0) {
                data->population_2014 = atoi(token);
            }
        }
        // Move to the next field
        token = strtok(NULL, ",");
        field_idx++;
    }

    return 0;  // Successful parsing
}

int parse_demographics_file(const char *filename, Demographics **data) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return -1;
    }

    // Allocate memory for the data array
    int num_entries = 0;
    *data = malloc(MAX_ENTRIES * sizeof(Demographics));
    if (*data == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    char line[MAX_LINE_LEN];
    int line_number = 0;
    while (fgets(line, MAX_LINE_LEN, file)) {
        line_number++;

        // Skip the header line (first line)
        if (line_number == 1) continue;

        // Ensure the line is valid (non-empty)
        if (strlen(line) == 0) continue;

        // Parse the line and store it in the data array
        if (parse_line(line, &(*data)[num_entries]) == -1) {
            fprintf(stderr, "Error parsing line %d, skipping.\n", line_number);
            continue;
        }

        num_entries++;
        if (num_entries >= MAX_ENTRIES) {
            fprintf(stderr, "Exceeded max entries\n");
            break;
        }
    }

    fclose(file);
    return num_entries;
}

void display(Demographics *data, int num_entries) {
    for (int i = 0; i < num_entries; i++) {
        printf("County: %s, State: %s, Population (2014): %d\n", 
            data[i].county, data[i].state, data[i].population_2014);
        // Print more fields as needed
    }
}

int filter_state(Demographics *data, int *num_entries, const char *state_abbr) {
    int count = 0;
    for (int i = 0; i < *num_entries; i++) {
        if (strcmp(data[i].state, state_abbr) == 0) {
            // Keep matching entries, shift data left if necessary
            if (i != count) {
                data[count] = data[i];
            }
            count++;
        }
    }
    *num_entries = count;
    printf("Filter: state == %s (%d entries)\n", state_abbr, count);
    return count;
}

int filter_field(Demographics *data, int *num_entries, const char *field, const char *operator, float number) {
    int count = 0;
    for (int i = 0; i < *num_entries; i++) {
        float value = 0;
        if (strcmp(field, "Income.Persons Below Poverty Level") == 0) {
            value = data[i].income_below_poverty;
        }
        // Check for education fields (e.g., "Education.HighSchool", "Education.Bachelor", etc.)
        else if (strncmp(field, "Education.", 10) == 0) {
            // Get the specific education level (e.g., "HighSchool", "Bachelor", etc.)
            if (strcmp(field, "Education.Bachelor's Degree or Higher") == 0) {
                value = data[i].education[0];
            } 
            else if (strcmp(field, "Education.High School or Higher") == 0) {
                value = data[i].education[1];
            }
        }
        // Check for ethnicity fields (e.g., "Ethnicity.White", "Ethnicity.Black", etc.)
        else if (strncmp(field, "Ethnicity.", 10) == 0) {
            // Get the specific ethnicity (e.g., "White", "Black", etc.)
            if (strcmp(field, "Ethnicities.American Indian and Alaska Native Alone") == 0) {
                value = data[i].ethnicities[0];
            }
            else if (strcmp(field, "Ethnicities.Asian Alone") == 0) {
                value = data[i].ethnicities[1];
            }
            else if (strcmp(field, "Ethnicity.Black Alone") == 0) {
                value = data[i].ethnicities[2];
            }
            else if (strcmp(field, "Ethnicity.Hispanic or Latino") == 0) {
                value = data[i].ethnicities[3];
            }
            else if (strcmp(field, "Native Hawaiian and Other Pacific Islander Alone") == 0) {
                value = data[i].ethnicities[4];
            }
            else if (strcmp(field, "Ethnicity.Ethnicities.Two or More Races") == 0) {
                value = data[i].ethnicities[5];
            }
            else if (strcmp(field, "Ethnicity.White Alone") == 0) {
                value = data[i].ethnicities[6];
            }
            else if (strcmp(field, "Ethnicity.White Alone not Hispanic or Latino") == 0) {
                value = data[i].ethnicities[7];
            }
        }

        // Apply comparison
        if ((strcmp(operator, "ge") == 0 && value >= number) ||
            (strcmp(operator, "le") == 0 && value <= number)) {
            // Keep matching entries
            if (i != count) {
                data[count] = data[i];
            }
            count++;
        }
    }
    *num_entries = count;
    printf("Filter: %s %s %.2f (%d entries)\n", field, operator, number, count);
    return count;
}

void population_total(Demographics *data, int num_entries) {
    int total_population = 0;
    for (int i = 0; i < num_entries; i++) {
        total_population += data[i].population_2014;
    }
    printf("2014 population: %d\n", total_population);
}

void population_field(Demographics *data, int num_entries, const char *field) {
    float total_sub_population = 0;

    for (int i = 0; i < num_entries; i++) {
        float sub_population = 0;

        // Check for "Income.Persons Below Poverty Level"
        if (strcmp(field, "Income.Persons Below Poverty Level") == 0) {
            sub_population = data[i].income_below_poverty * data[i].population_2014 / 100.0f;
        }
        // Check for education fields (e.g., "Education.HighSchool", "Education.Bachelor", etc.)
        else if (strncmp(field, "Education.", 10) == 0) {
            // Get the specific education level (e.g., "HighSchool", "Bachelor", etc.)
            if (strcmp(field, "Education.Bachelor's Degree or Higher") == 0) {
                sub_population = data[i].education[0] * data[i].population_2014 / 100.0f;
            } 
            else if (strcmp(field, "Education.High School or Higher") == 0) {
                sub_population = data[i].education[1] * data[i].population_2014 / 100.0f;
            }
        }
        // Check for ethnicity fields (e.g., "Ethnicity.White", "Ethnicity.Black", etc.)
        else if (strncmp(field, "Ethnicity.", 10) == 0) {
            // Get the specific ethnicity (e.g., "White", "Black", etc.)
            if (strcmp(field, "Ethnicities.American Indian and Alaska Native Alone") == 0) {
                sub_population = data[i].ethnicities[0] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicities.Asian Alone") == 0) {
                sub_population = data[i].ethnicities[1] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.Black Alone") == 0) {
                sub_population = data[i].ethnicities[2] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.Hispanic or Latino") == 0) {
                sub_population = data[i].ethnicities[3] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Native Hawaiian and Other Pacific Islander Alone") == 0) {
                sub_population = data[i].ethnicities[4] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.Ethnicities.Two or More Races") == 0) {
                sub_population = data[i].ethnicities[5] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.White Alone") == 0) {
                sub_population = data[i].ethnicities[6] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.White Alone not Hispanic or Latino") == 0) {
                sub_population = data[i].ethnicities[7] * data[i].population_2014 / 100.0f;
            }
        }

        // Accumulate the sub-population for the field across all entries
        total_sub_population += sub_population;
    }

    // Print the result for the specified field
    printf("2014 %s population: %.0f\n", field, total_sub_population);
}

void percent_field(Demographics *data, int num_entries, const char *field) {
    float total_population = 0;
    float total_sub_population = 0;

    // Calculate total population
    for (int i = 0; i < num_entries; i++) {
        total_population += data[i].population_2014;
    }

    // Calculate sub-population based on the field
    for (int i = 0; i < num_entries; i++) {
                float sub_population = 0;

        // Check for "Income.Persons Below Poverty Level"
        if (strcmp(field, "Income.Persons Below Poverty Level") == 0) {
            sub_population = data[i].income_below_poverty * data[i].population_2014 / 100.0f;
        }
        // Check for education fields (e.g., "Education.HighSchool", "Education.Bachelor", etc.)
        else if (strncmp(field, "Education.", 10) == 0) {
            // Get the specific education level (e.g., "HighSchool", "Bachelor", etc.)
            if (strcmp(field, "Education.Bachelor's Degree or Higher") == 0) {
                sub_population = data[i].education[0] * data[i].population_2014 / 100.0f;
            } 
            else if (strcmp(field, "Education.High School or Higher") == 0) {
                sub_population = data[i].education[1] * data[i].population_2014 / 100.0f;
            }
        }
        // Check for ethnicity fields (e.g., "Ethnicity.White", "Ethnicity.Black", etc.)
        else if (strncmp(field, "Ethnicity.", 10) == 0) {
            // Get the specific ethnicity (e.g., "White", "Black", etc.)
            if (strcmp(field, "Ethnicities.American Indian and Alaska Native Alone") == 0) {
                sub_population = data[i].ethnicities[0] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicities.Asian Alone") == 0) {
                sub_population = data[i].ethnicities[1] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.Black Alone") == 0) {
                sub_population = data[i].ethnicities[2] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.Hispanic or Latino") == 0) {
                sub_population = data[i].ethnicities[3] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Native Hawaiian and Other Pacific Islander Alone") == 0) {
                sub_population = data[i].ethnicities[4] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.Ethnicities.Two or More Races") == 0) {
                sub_population = data[i].ethnicities[5] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.White Alone") == 0) {
                sub_population = data[i].ethnicities[6] * data[i].population_2014 / 100.0f;
            }
            else if (strcmp(field, "Ethnicity.White Alone not Hispanic or Latino") == 0) {
                sub_population = data[i].ethnicities[7] * data[i].population_2014 / 100.0f;
            }
        }

        // Accumulate the sub-population for the field across all entries
        total_sub_population += sub_population;
    }
    if (total_population > 0) {
        printf("2014 %s percentage: %.2f%%\n", field, (total_sub_population / total_population) * 100);
    }
}

void process_operations(const char *filename, Demographics *data, int *num_entries) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening operations file\n");
        return;
    }

    char line[MAX_LINE_LEN];
    int line_number = 0;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (line[0] == '\0' || line[0] == '\n') continue; // Skip empty lines

        char operation[MAX_OP_LEN];
        if (sscanf(line, "%s", operation) != 1) {
            fprintf(stderr, "Error parsing operation at line %d\n", line_number);
            continue;
        }

        // Process each operation
        if (strncmp(operation, "display", 7) == 0) {
            display(data, *num_entries);
        }
        else if (strncmp(operation, "filter-state:", 13) == 0) {
            char state_abbr[3];
            sscanf(line + 13, "%2s", state_abbr);
            filter_state(data, num_entries, state_abbr);
        }
        else if (strncmp(operation, "filter:", 7) == 0) {
            char field[MAX_FIELD_LEN], operator[MAX_OP_LEN];
            float number;
            sscanf(line + 7, "%[^:]:%[^:]:%f", field, operator, &number);
            filter_field(data, num_entries, field, operator, number);
        }
        else if (strncmp(operation, "population-total", 16) == 0) {
            population_total(data, *num_entries);
        }
        else if (strncmp(operation, "population:", 11) == 0) {
            char field[MAX_FIELD_LEN];
            sscanf(line + 11, "%s", field);
            population_field(data, *num_entries, field);
        }
        else if (strncmp(operation, "percent:", 8) == 0) {
            char field[MAX_FIELD_LEN];
            sscanf(line + 8, "%s", field);
            percent_field(data, *num_entries, field);
        }
        else {
            fprintf(stderr, "Error: Unknown operation at line %d: %s\n", line_number, operation);
        }
    }

    fclose(file);
}

\
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <demographics_file> <operations_file>\n", argv[0]);
        return 1;
    }

    const char *demographics_file = argv[1];
    const char *operations_file = argv[2];

    // Load demographics data
    Demographics *data = NULL;
    int num_entries = parse_demographics_file(demographics_file, &data);
    if (num_entries == -1) {
        return 1; // Error loading demographics file
    }

    printf("%d records loaded.\n", num_entries);

    // Process operations file
    process_operations(operations_file, data, &num_entries);


    free(data);  // Free memory after processing operations
    return 0;
}
