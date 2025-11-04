#include "student.h"

/* ---------- File Operations ---------- */

ErrorCode save_to_file(StudentList *list, const char *filename) {
    if (!list || !filename) {
        return ERR_INVALID_INPUT;
    }
    
    FILE *f = fopen(filename, "w");
    
    if (!f) {
        fprintf(stderr, "Error: Cannot open '%s' for writing: %s\n",
                filename, strerror(errno));
        return ERR_FILE_IO;
    }
    
    fprintf(f, "# Student Record System Data File\n");
    fprintf(f, "# Format: roll|marks|name\n");
    fprintf(f, "# Total records: %zu\n", list->size);
    
    for (size_t i = 0; i < list->size; i++) {
        Student *s = list->items[i];
        fprintf(f, "%d|%d|%s\n", s->roll, s->marks, s->name ? s->name : "");
    }
    
    fclose(f);
    
    // Update last filename and clear modified flag
    char *new_filename = safe_strdup(filename);
    if (!new_filename) {
        return ERR_MEMORY;
    }
    free(list->last_filename);
    list->last_filename = new_filename;
    list->modified = 0;
    return SUCCESS;
}

ErrorCode load_from_file(StudentList *list, const char *filename) {
    if (!list || !filename) {
        return ERR_INVALID_INPUT;
    }
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open '%s' for reading: %s\n",
                filename, strerror(errno));
        return ERR_FILE_IO;
    }
    
    // Clear existing list
    for (size_t i = 0; i < list->size; i++) {
        free_student(list->items[i]);
    }
    list->size = 0;
    
    char buffer[MAX_LINE_LENGTH];
    size_t line_num = 0;
    size_t loaded = 0;
    
    while (fgets(buffer, sizeof(buffer), f)) {
        line_num++;
        
        // Skip comments and empty lines
        if (buffer[0] == '#' || buffer[0] == '\n') continue;
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // Parse: roll|marks|name
        char *p1 = strchr(buffer, '|');
        if (!p1) {
            fprintf(stderr, "Warning: Invalid format at line %zu\n", line_num);
            continue;
        }
        *p1 = '\0';
        
        char *p2 = strchr(p1 + 1, '|');
        if (!p2) {
            fprintf(stderr, "Warning: Invalid format at line %zu\n", line_num);
            continue;
        }
        *p2 = '\0';
        
        int roll = (int)strtol(buffer, NULL, 10);
        int marks = (int)strtol(p1 + 1, NULL, 10);
        char *name = p2 + 1;
        trim_inplace(name);
        
        // Validate data
        if (roll <= 0 || marks < 0 || marks > 100) {
            fprintf(stderr, "Warning: Invalid data at line %zu (skipped)\n", line_num);
            continue;
        }
        
        Student *s = create_student(roll, name, marks);
        if (s && add_student(list, s) == SUCCESS) {
            loaded++;
        } else {
            free_student(s);
            fprintf(stderr, "Warning: Duplicate roll %d at line %zu (skipped)\n",
                    roll, line_num);
        }
    }
    
    fclose(f);
    
    // Update last filename and clear modified flag
    char *new_filename = safe_strdup(filename);
    if (!new_filename) {
        return ERR_MEMORY;
    }
    free(list->last_filename);
    list->last_filename = new_filename;
    list->modified = 0;
    
    printf("Loaded %zu records from '%s'\n", loaded, filename);
    return SUCCESS;
}
