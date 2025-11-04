/*
 Student Record System (C) - Improved Version
 - Dynamic, resizable array with better memory management
 - Enhanced error handling and input validation
 - Improved file I/O with better parsing
 - More robust duplicate detection
 - Better code organization and documentation
 - Enhanced user experience with clearer messages

 Compile:
   gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -o student_records student_records.c

 Author: Improved version
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define INITIAL_CAPACITY 8
#define PASS_THRESHOLD 40
#define MAX_NAME_LENGTH 100
#define MAX_LINE_LENGTH 1024
#define FILENAME "students.txt"

typedef enum {
    SUCCESS = 0,
    ERR_MEMORY,
    ERR_NOT_FOUND,
    ERR_DUPLICATE,
    ERR_FILE_IO,
    ERR_INVALID_INPUT
} ErrorCode;

typedef struct {
    int roll;
    char *name;
    int marks;
} Student;

typedef struct {
    Student **items;
    size_t size;
    size_t capacity;
    int modified;  // Track unsaved changes
    char *last_filename;  // Remember last used filename
} StudentList;

/* ---------- Function Prototypes ---------- */

static char *safe_strdup(const char *s);
static char *read_line(const char *prompt);
static void trim_inplace(char *s);
static ErrorCode init_student_list(StudentList *list);
static void free_student(Student *s);
static void free_student_list(StudentList *list);
static ErrorCode ensure_capacity(StudentList *list);
static Student *create_student(int roll, const char *name, int marks);
static long find_index_by_roll(const StudentList *list, int roll);
static ErrorCode add_student(StudentList *list, Student *s);
static ErrorCode remove_student_by_index(StudentList *list, size_t index);
static ErrorCode modify_student(
    StudentList *list, 
    size_t index,
    int new_roll, 
    const char *new_name, 
    int new_marks);
static void display_student(const Student *s);
static void display_all_students(const StudentList *list);
static void display_statistics(const StudentList *list);
static ErrorCode save_to_file(StudentList *list, const char *filename);
static ErrorCode load_from_file(StudentList *list, const char *filename);
static Student *search_by_roll(const StudentList *list, int roll);
static int cmp_marks_asc(const void *a, const void *b);
static int cmp_marks_desc(const void *a, const void *b);
static int cmp_name_asc(const void *a, const void *b);
static void sort_students(StudentList *list, int (*cmp)(const void*, const void*));
static int prompt_yes_no(const char *prompt);
static void auto_save_prompt(StudentList *list);
static int prompt_int(const char *prompt, int min, int max);
static ErrorCode prompt_student_input(int *out_roll, char **out_name, int *out_marks);
static void show_menu(void);

/* ---------- Memory Management ---------- */

static char *safe_strdup(const char *s) {
    if (!s) {
        return NULL;
    }

    size_t len = strlen(s) + 1;
    char *copy = malloc(len);

    if (copy) {
        memcpy(copy, s, len);
    }

    return copy;
}

static char *read_line(const char *prompt) {
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    
    size_t capacity = 128;
    size_t len = 0;
    char *buf = malloc(capacity);

    if (!buf) {
        return NULL;
    }

    int c;
    while ((c = getchar()) != EOF && c != '\n') {
        if (len + 1 >= capacity) {
            capacity *= 2;
            char *tmp = realloc(buf, capacity);
            if (!tmp) {
                free(buf);
                return NULL;
            }
            buf = tmp;
        }
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    return buf;
}

static void trim_inplace(char *s) {
    if (!s) {
        return;
    }

    // Left trim
    char *start = s;

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }

    // Right trim
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) {
        s[--n] = '\0';
    }
}

/* ---------- StudentList Management ---------- */

static ErrorCode init_student_list(StudentList *list) {
    if (!list) {
        return ERR_INVALID_INPUT;
    }

    list->capacity = INITIAL_CAPACITY;
    list->size = 0;
    list->modified = 0;
    list->last_filename = NULL;
    list->items = calloc(list->capacity, sizeof(Student*));
    
    return list->items ? SUCCESS : ERR_MEMORY;
}

static void free_student(Student *s) {
    if (!s) {
        return;
    }

    free(s->name);
    free(s);
}

static void free_student_list(StudentList *list) {
    if (!list) {
        return;
    }

    for (size_t i = 0; i < list->size; i++) {
        free_student(list->items[i]);
    }

    free(list->items);
    free(list->last_filename);
    list->items = NULL;
    list->last_filename = NULL;
    list->size = 0;
    list->capacity = 0;
    list->modified = 0;
}

static ErrorCode ensure_capacity(StudentList *list) {
    if (!list) {
        return ERR_INVALID_INPUT;
    }

    if (list->size < list->capacity) {
        return SUCCESS;
    }

    size_t new_capacity = list->capacity * 2;
    Student **tmp = realloc(list->items, new_capacity * sizeof(Student*));

    if (!tmp) {
        return ERR_MEMORY;
    }

    list->items = tmp;
    list->capacity = new_capacity;
    return SUCCESS;
}

/* ---------- Student Operations ---------- */

static Student *create_student(int roll, const char *name, int marks) {
    Student *student = malloc(sizeof(Student));

    if (!student) {
        return NULL;
    }
    
    student->roll = roll;
    student->name = safe_strdup(name ? name : "Unnamed");
    student->marks = marks;

    if (!student->name) {
        free(student);
        return NULL;
    }

    return student;
}

static long find_index_by_roll(const StudentList *list, int roll) {
    if (!list) {
        return -1;
    }

    for (size_t i = 0; i < list->size; i++) {
        if (list->items[i]->roll == roll) {
            return (long)i;
        }
    }

    return -1;
}

static ErrorCode add_student(StudentList *list, Student *s) {
    if (!list || !s) {
        return ERR_INVALID_INPUT;
    }

    // Check for duplicate roll number
    if (find_index_by_roll(list, s->roll) >= 0) {
        return ERR_DUPLICATE;
    }
    
    ErrorCode err = ensure_capacity(list);

    if (err != SUCCESS) {
        return err;
    }

    list->items[list->size++] = s;
    list->modified = 1;  // Mark as modified
    return SUCCESS;
}

static ErrorCode remove_student_by_index(StudentList *list, size_t index) {
    if (!list || index >= list->size) {
        return ERR_INVALID_INPUT;
    }

    free_student(list->items[index]);
    
    // Shift remaining elements
    memmove(&list->items[index], &list->items[index + 1],
            (list->size - index - 1) * sizeof(Student*));
    list->size--;
    list->modified = 1;  // Mark as modified
    
    return SUCCESS;
}

static ErrorCode modify_student(
    StudentList *list, 
    size_t index,
    int new_roll, 
    const char *new_name, 
    int new_marks) {
        
    if (!list || index >= list->size) {
        return ERR_INVALID_INPUT;
    }

    // Check if new roll number conflicts with another student
    if (new_roll != list->items[index]->roll) {
        long existing = find_index_by_roll(list, new_roll);
        if (existing >= 0 && (size_t)existing != index) {
            return ERR_DUPLICATE;
        }
    }
    
    Student *s = list->items[index];
    s->roll = new_roll;
    s->marks = new_marks;
    
    free(s->name);
    s->name = safe_strdup(new_name ? new_name : "Unnamed");
    if (!s->name) {
        return ERR_MEMORY;
    }

    list->modified = 1;  // Mark as modified
    return SUCCESS;
}

/* ---------- Display Functions ---------- */

static void display_student(const Student *s) {
    if (!s) {
        return;
    }

    printf("Roll: %-5d Name: %-30s Marks: %3d [%s]\n",
           s->roll, s->name ? s->name : "(no name)", s->marks,
           (s->marks >= PASS_THRESHOLD) ? "PASS" : "FAIL");
}

static void display_all_students(const StudentList *list) {
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

static void display_statistics(const StudentList *list) {
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

/* ---------- File Operations ---------- */

static ErrorCode save_to_file(StudentList *list, const char *filename) {
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

static ErrorCode load_from_file(StudentList *list, const char *filename) {
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

/* ---------- Search & Sort ---------- */

static Student *search_by_roll(const StudentList *list, int roll) {
    long idx = find_index_by_roll(list, roll);
    return (idx >= 0) ? list->items[idx] : NULL;
}

static int cmp_marks_asc(const void *a, const void *b) {
    const Student *sa = *(const Student**)a;
    const Student *sb = *(const Student**)b;
    return sa->marks - sb->marks;
}

static int cmp_marks_desc(const void *a, const void *b) {
    return -cmp_marks_asc(a, b);
}

static int cmp_name_asc(const void *a, const void *b) {
    const Student *sa = *(const Student**)a;
    const Student *sb = *(const Student**)b;
    return strcmp(sa->name, sb->name);
}

static void sort_students(StudentList *list, int (*cmp)(const void*, const void*)) {
    if (!list || list->size < 2) {
        return;
    }

    qsort(list->items, list->size, sizeof(Student*), cmp);
    list->modified = 1;  // Mark as modified since order changed
}

/* ---------- Input Helpers ---------- */

static int prompt_yes_no(const char *prompt) {
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

static void auto_save_prompt(StudentList *list) {
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

static int prompt_int(const char *prompt, int min, int max) {
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

static ErrorCode prompt_student_input(int *out_roll, char **out_name, int *out_marks) {
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

static void show_menu(void) {
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

/* ---------- Main Program ---------- */

int main(void) {
    printf("Welcome to Student Record System v2.0!\n\n");
    
    char *user = read_line("Please enter your name: ");

    if (user) {
        trim_inplace(user);
        if (strlen(user) == 0) {
            free(user);
            user = safe_strdup("User");
        }
    } else {
        user = safe_strdup("User");
    }
    
    printf("\nHello, %s! Let's manage some student records.\n", user);
    
    StudentList list;

    if (init_student_list(&list) != SUCCESS) {
        fprintf(stderr, "Fatal: Memory allocation failed\n");
        free(user);
        return EXIT_FAILURE;
    }
    
    int running = 1;
    
    while (running) {
        show_menu();
        
        // Show modification status
        if (list.modified) {
            printf("Unsaved changes ");
            if (list.last_filename) {
                printf("(last file: %s)", list.last_filename);
            }
            printf("\n");
        }
        
        int choice = prompt_int("Choose an option (0-12): ", 0, 12);
        
        switch (choice) {
            case 1: { // Add
                int roll, marks;
                char *name;
                if (prompt_student_input(&roll, &name, &marks) != SUCCESS) {
                    printf("Failed to get input.\n");
                    break;
                }
                
                Student *s = create_student(roll, name, marks);
                free(name);
                
                if (!s) {
                    printf("Memory allocation failed.\n");
                    break;
                }
                
                ErrorCode err = add_student(&list, s);

                if (err == ERR_DUPLICATE) {
                    printf("Student with roll %d already exists!\n", roll);
                    free_student(s);
                } else if (err == SUCCESS) {
                    printf("Student added successfully! [%s]\n",
                           (marks >= PASS_THRESHOLD) ? "PASS" : "FAIL");
                } else {
                    printf("Failed to add student.\n");
                    free_student(s);
                }
                break;
            }
            
            case 2: { // Modify
                int roll = prompt_int("Enter roll number to modify: ", 1, 99999);
                long idx = find_index_by_roll(&list, roll);
                
                if (idx < 0) {
                    printf("Student with roll %d not found.\n", roll);
                    break;
                }
                
                printf("\nCurrent details:\n");
                display_student(list.items[idx]);
                
                printf("\nEnter new details:\n");
                int new_roll, new_marks;
                char *new_name;
                
                if (prompt_student_input(&new_roll, &new_name, &new_marks) != SUCCESS) {
                    printf("Failed to get input.\n");
                    break;
                }
                
                ErrorCode err = modify_student(&list, (size_t)idx, new_roll, new_name, new_marks);
                free(new_name);
                
                if (err == ERR_DUPLICATE) {
                    printf("Roll number %d already exists!\n", new_roll);
                } else if (err == SUCCESS) {
                    printf("Student modified successfully!\n");
                } else {
                    printf("Failed to modify student.\n");
                }
                break;
            }
            
            case 3: { // Remove
                int roll = prompt_int("Enter roll number to remove: ", 1, 99999);
                long idx = find_index_by_roll(&list, roll);
                
                if (idx < 0) {
                    printf("Student with roll %d not found.\n", roll);
                } else {
                    printf("\nRemoving: ");
                    display_student(list.items[idx]);
                    
                    if (remove_student_by_index(&list, (size_t)idx) == SUCCESS) {
                        printf("Student removed successfully!\n");
                    } else {
                        printf("Failed to remove student.\n");
                    }
                }
                break;
            }
            
            case 4: // Display all
                display_all_students(&list);
                break;
            
            case 5: { // Search
                int roll = prompt_int("Enter roll number to search: ", 1, 99999);
                Student *s = search_by_roll(&list, roll);
                
                if (!s) {
                    printf("\nStudent with roll %d not found.\n", roll);
                } else {
                    printf("\nFound:\n");
                    display_student(s);
                }
                break;
            }
            
            case 6: // Statistics
                display_statistics(&list);
                break;
            
            case 7:
                sort_students(&list, cmp_marks_asc);
                printf("Sorted by marks (ascending).\n");
                break;
            
            case 8:
                sort_students(&list, cmp_marks_desc);
                printf("Sorted by marks (descending).\n");
                break;
            
            case 9:
                sort_students(&list, cmp_name_asc);
                printf("Sorted by name (alphabetically).\n");
                break;
            
            case 10: { // Save to file
                if (save_to_file(&list, FILENAME) == SUCCESS) {
                    printf("Saved %zu records to '" FILENAME "'\n", list.size);
                } else {
                    printf("Failed to save to '" FILENAME "'\n");
                }
                break;
            }

            case 11: { // Load from file
                if (load_from_file(&list, FILENAME) == SUCCESS) {
                    printf("Records loaded from '" FILENAME "'\n");
                } else {
                    printf("Failed to load from '" FILENAME "'\n");
                }
                break;
            }
              
            case 12: { // Quick save
                if (!list.last_filename) {
                    printf("No file loaded yet. Use option 10 to save to a new file.\n");
                } else {
                    if (save_to_file(&list, list.last_filename) == SUCCESS) {
                        printf("Quick saved %zu records to '%s'\n", list.size, list.last_filename);
                    }
                }
                break;
            }

            case 0:
                running = 0;
                printf("\nExiting...\n");
                auto_save_prompt(&list);
                break;
        }
    }
    
    // Cleanup
    free_student_list(&list);
    free(user);
    
    printf("\nThank you for using Student Record System! Goodbye!\n");
    return EXIT_SUCCESS;
}
