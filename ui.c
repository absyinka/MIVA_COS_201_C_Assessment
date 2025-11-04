#include "student.h"

/* ---------- Display Functions ---------- */

void display_student(const Student *s) {
    if (!s) {
        return;
    }

    printf("Roll: %-5d Name: %-30s Marks: %3d [%s]\n",
           s->roll, s->name ? s->name : "(no name)", s->marks,
           (s->marks >= PASS_THRESHOLD) ? "PASS" : "FAIL");
}

void display_all_students(const StudentList *list) {
    if (!list || list->size == 0) {
        printf("\nNo students in the system.\n");
        return;
    }
    
    printf("\nStudent Records (Total: %zu)\n", list->size);
    printf("===========================================================================\n");
    
    for (size_t i = 0; i < list->size; i++) {
        printf("[%zu] ", i + 1);
        display_student(list->items[i]);
    }
    printf("===========================================================================\n");
}

void display_statistics(const StudentList *list) {
    if (!list || list->size == 0) {
        printf("\nNo data available for statistics.\n");
        return;
    }
    
    int pass_count = 0, fail_count = 0;
    int min_marks = 100, max_marks = 0;
    long total_marks = 0;
    
    for (size_t i = 0; i < list->size; i++) {
        int marks = list->items[i]->marks;
        total_marks += marks;
        
        if (marks >= PASS_THRESHOLD) {
            pass_count++;
        }else {
            fail_count++;
        }

        if (marks < min_marks) {
            min_marks = marks;
        }

        if (marks > max_marks) {
            max_marks = marks;
        }
    }
    
    double avg = (double)total_marks / list->size;
    double pass_rate = (double)pass_count / list->size * 100;
    
    printf("\nStatistics Summary\n");
    printf("======================================================================\n");
    printf("Total Students:    %zu\n", list->size);
    printf("Average Marks:     %.2f\n", avg);
    printf("Highest Marks:     %d\n", max_marks);
    printf("Lowest Marks:      %d\n", min_marks);
    printf("Pass Count:        %d (%.1f%%)\n", pass_count, pass_rate);
    printf("Fail Count:        %d\n", fail_count);
    printf("======================================================================\n");
}

/* ---------- Input Helpers ---------- */
int prompt_yes_no(const char *prompt) {
    while (1) {
        char *line = read_line(prompt);

        if (!line) {
            continue;
        }

        trim_inplace(line);

        if (strlen(line) > 0) {
            char c = tolower((unsigned char)line[0]);
            free(line);

            if (c == 'y') {
                return 1;
            }

            if (c == 'n') {
                return 0;
            }
        } else {
            free(line);
        }
        printf("Please enter 'y' or 'n'.\n");
    }
}

void auto_save_prompt(StudentList *list) {
    if (!list->modified) {
        return;
    }
    
    printf("\nYou have unsaved changes!\n");
    
    if (list->last_filename) {
        printf("Last file: %s\n", list->last_filename);

        if (prompt_yes_no("Save to this file? (y/n): ")) {
            if (save_to_file(list, list->last_filename) == SUCCESS) {
                printf("Saved %zu records to '%s'\n", list->size, list->last_filename);
            }
        }
    } else {
        if (prompt_yes_no("Would you like to save to " FILENAME "? (y/n): ")) {
            if (save_to_file(list, FILENAME) == SUCCESS) {
                printf("Saved %zu records to '" FILENAME "'\n", list->size);
            }
        }
    }
}

int prompt_int(const char *prompt, int min, int max) {
    while (1) {
        char *line = read_line(prompt);

        if (!line) {
            printf("Memory error. Try again.\n");
            continue;
        }
        
        trim_inplace(line);

        if (strlen(line) == 0) {
            free(line);
            printf("Input cannot be empty. Try again.\n");
            continue;
        }
        
        char *endptr = NULL;
        errno = 0;
        long val = strtol(line, &endptr, 10);
        
        if (*endptr != '\0' || errno == ERANGE) {
            free(line);
            printf("Invalid number. Try again.\n");
            continue;
        }
        
        free(line);
        
        if (val < min || val > max) {
            printf("Number must be between %d and %d. Try again.\n", min, max);
            continue;
        }
        
        return (int)val;
    }
}

ErrorCode prompt_student_input(int *out_roll, char **out_name, int *out_marks) {
    int roll = prompt_int("Enter roll number (1-99999): ", 1, 99999);
    
    char *line = read_line("Enter student name: ");

    if (!line) {
        return ERR_MEMORY;
    }

    trim_inplace(line);

    if (strlen(line) == 0) {
        free(line);
        line = safe_strdup("Unnamed");
    } else if (strlen(line) > MAX_NAME_LENGTH) {
        line[MAX_NAME_LENGTH] = '\0';
        printf("Name truncated to %d characters.\n", MAX_NAME_LENGTH);
    }
    
    int marks = prompt_int("Enter marks (0-100): ", 0, 100);
    
    *out_roll = roll;
    *out_name = line;
    *out_marks = marks;
    
    return SUCCESS;
}

/* ---------- Menu System ---------- */

void show_menu(void) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     Student Record System Menu         ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║  1. Add a student                      ║\n");
    printf("║  2. Modify a student                   ║\n");
    printf("║  3. Remove a student                   ║\n");
    printf("║  4. Display all students               ║\n");
    printf("║  5. Search by roll number              ║\n");
    printf("║  6. Show statistics                    ║\n");
    printf("║  7. Sort by marks (ascending)          ║\n");
    printf("║  8. Sort by marks (descending)         ║\n");
    printf("║  9. Sort by name                       ║\n");
    printf("║ 10. Save to file                       ║\n");
    printf("║ 11. Load from file                     ║\n");
    printf("║ 12. Quick save                         ║\n");
    printf("║ 0.  Exit                               ║\n");
    printf("╚════════════════════════════════════════╝\n");
}