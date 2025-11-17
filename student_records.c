/*
 Student Record System (C) - 
 This software features are:
 - Dynamic, resizable array better memory management
 - Error handling and input validation
 - File I/O with better parsing
 - Robust duplicate detection
 - Good code organization and documentation
 - Optimized user experience with clear messages

 Compile(for me):
   gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -o student_records student_records.c

 Author: our Group
*/
/*Here, we include all needed libraries*/
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

/* We created a function that stores induvidual student data*/
/*The way it works, is that, it uses dynamic memory names with flexible sizes*/
typedef struct {
    int roll;
    char *name;
    int marks;
} Student;

/*Then this part is the function "studentList" structure */
typedef struct {
    Student **items;
    size_t size;
    size_t capacity;
    int modified;  // This property tracks unsaved changes
    char *last_filename;  // This property helps to remember the last used filename
} StudentList;

/* ---------- Function Prototypes ---------- */

static char *safe_strdup(const char *s);
/*Here deals with User Input & validation*/
static char *read_line(const char *prompt);
static void trim_inplace(char *s);
static ErrorCode init_student_list(StudentList *list);
static void free_student(Student *s);
static void free_student_list(StudentList *list);
static ErrorCode ensure_capacity(StudentList *list);
static Student *create_student(int roll, const char *name, int marks);
static long find_index_by_roll(const StudentList *list, int roll);
/*The core operations of the code*/
/*Topics we learnt from school were added here: creare, read, update, delete*/
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
/*File I/O*/
static ErrorCode save_to_file(StudentList *list, const char *filename);
static ErrorCode load_from_file(StudentList *list, const char *filename); /*/*Here was edited to work in a way that displays students directly from the file*/
static ErrorCode display_from_file(const char *filename);
static ErrorCode search_in_file(const char *filename, int roll);
static ErrorCode statistics_from_file(const char *filename);
static Student *search_by_roll(const StudentList *list, int roll);

/*Sorting and Display*/
/*Here, we have Multiple sorting options, and Clean display formating*/
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

/* This function was added here so that we can safely duplicate strings without worrying about null pointers */
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

/* This part of the code reads input from the user dynamically, 
   so we can handle any length of input without fixed buffer sizes */
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
    
    /* This case was added here so that if EOF is encountered and nothing was read, 
       we free the memory and return NULL instead of an empty string */
    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
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

/* This block does: it automatically expands the array when it gets full 
   so we don't have to manually manage the size every time */
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

/* This part checks for duplicates before adding, so we don't have two students with the same roll number */
static ErrorCode add_student(StudentList *list, Student *s) {
    if (!list || !s) {
        return ERR_INVALID_INPUT;
    }

    if (find_index_by_roll(list, s->roll) >= 0) {
        return ERR_DUPLICATE;
    }
    
    ErrorCode err = ensure_capacity(list);

    if (err != SUCCESS) {
        return err;
    }

    list->items[list->size++] = s;
    list->modified = 1;
    return SUCCESS;
}

/* This block does: it removes a student and shifts all the remaining students 
   to fill the gap, so there are no empty spaces in the array */
static ErrorCode remove_student_by_index(StudentList *list, size_t index) {
    if (!list || index >= list->size) {
        return ERR_INVALID_INPUT;
    }

    free_student(list->items[index]);
    
    memmove(&list->items[index], &list->items[index + 1],
            (list->size - index - 1) * sizeof(Student*));
    list->size--;
    list->modified = 1;
    
    return SUCCESS;
}

/* This case was added here so that when modifying a student, we check if the new roll number 
   doesn't conflict with existing students (except the one being modified) */
static ErrorCode modify_student(
    StudentList *list, 
    size_t index,
    int new_roll, 
    const char *new_name, 
    int new_marks) {
        
    if (!list || index >= list->size) {
        return ERR_INVALID_INPUT;
    }

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

/* Display Functions */

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
    printf("----------------------------------------------------------------------------\n");
    
    for (size_t i = 0; i < list->size; i++) {
        printf("[%zu] ", i + 1);
        display_student(list->items[i]);
    }
    printf("------------------------------------------------------------------------------\n");
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
    printf("----------------------------------------------------------------------\n");
    printf("Total Students:    %zu\n", list->size);
    printf("Average Marks:     %.2f\n", avg);
    printf("Highest Marks:     %d\n", max_marks);
    printf("Lowest Marks:      %d\n", min_marks);
    printf("Pass Count:        %d (%.1f%%)\n", pass_count, pass_rate);
    printf("Fail Count:        %d\n", fail_count);
    printf("----------------------------------------------------------------------\n");
}

/* ---------- File Operations section (this area deals with the operations for the file handling, creation and all) ---------- */

/* This part of the code does this: it saves all students to a text file using the format roll|marks|name
   so the data can be stored permanently and loaded later */
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

/* Reads and displays all student records directly from file without loading into memory */
static ErrorCode display_from_file(const char *filename) {
    if (!filename) {
        return ERR_INVALID_INPUT;
    }
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open '%s' for reading: %s\n",
                filename, strerror(errno));
        return ERR_FILE_IO;
    }
    
    char buffer[MAX_LINE_LENGTH];
    size_t line_num = 0;
    size_t count = 0;
    int roll, marks;
    char name[MAX_NAME_LENGTH + 1];
    
    printf("\nReading from file: %s\n", filename);
    printf("------------------------------------------------------------------------------\n");
    while (fgets(buffer, sizeof(buffer), f)) {
        line_num++;
        
        // Skip comments and empty lines
        if (buffer[0] == '#' || buffer[0] == '\n') continue;
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // Parse: roll|marks|name
        char *p1 = strchr(buffer, '|');
        if (!p1) {
            continue;
        }
        *p1 = '\0';
        
        char *p2 = strchr(p1 + 1, '|');
        if (!p2) {
            continue;
        }
        *p2 = '\0';
        
        roll = (int)strtol(buffer, NULL, 10);
        marks = (int)strtol(p1 + 1, NULL, 10);
        char *name_ptr = p2 + 1;
        trim_inplace(name_ptr);
        
        // Validate data
        if (roll <= 0 || marks < 0 || marks > 100) {
            continue;
        }
        
        // Copy name safely
        strncpy(name, name_ptr, MAX_NAME_LENGTH);
        name[MAX_NAME_LENGTH] = '\0';
        
        // Display the student
        count++;
        printf("[%zu] Roll: %-5d Name: %-30s Marks: %3d [%s]\n",
               count, roll, name, marks,
               (marks >= PASS_THRESHOLD) ? "PASS" : "FAIL");
    }
    
    fclose(f);
    
    if (count == 0) {
        printf("No student records found in the file.\n");
    } else {
        printf("----------------------------------------------------------------------------\n");
        printf("Total records in file: %zu\n", count);
    }
    
    return SUCCESS;
}

/* Searches for a specific student by roll number directly in the file */
static ErrorCode search_in_file(const char *filename, int roll) {
    if (!filename) {
        return ERR_INVALID_INPUT;
    }
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open '%s' for reading: %s\n",
                filename, strerror(errno));
        return ERR_FILE_IO;
    }
    
    char buffer[MAX_LINE_LENGTH];
    size_t line_num = 0;
    int found = 0;
    
    printf("\nSearching for roll number %d in file: %s\n", roll, filename);
    printf("---------------------------------------------------------------------------\n");
    
    while (fgets(buffer, sizeof(buffer), f)) {
        line_num++;
        
        // Skip comments and empty lines
        if (buffer[0] == '#' || buffer[0] == '\n') continue;
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // Parse: roll|marks|name
        char *p1 = strchr(buffer, '|');
        if (!p1) continue;
        *p1 = '\0';
        
        char *p2 = strchr(p1 + 1, '|');
        if (!p2) continue;
        *p2 = '\0';
        
        int file_roll = (int)strtol(buffer, NULL, 10);
        
        // This Checks if this is the student we're looking for
        if (file_roll == roll) {
            int marks = (int)strtol(p1 + 1, NULL, 10);
            char *name = p2 + 1;
            trim_inplace(name);
            
            // Validate and display
            if (roll > 0 && marks >= 0 && marks <= 100) {
                found = 1;
                printf("Found at line %zu:\n", line_num);
                printf("Roll: %-5d Name: %-30s Marks: %3d [%s]\n",
                       file_roll, name, marks,
                       (marks >= PASS_THRESHOLD) ? "PASS" : "FAIL");
                break;
            }
        }
    }
    
    fclose(f);
    
    if (!found) {
        printf("Student with roll number %d not found in the file.\n", roll);
    }
    printf("-------------------------------------------------------------------------------\n");
    
    return found ? SUCCESS : ERR_NOT_FOUND;
}

/* Calculates statistics by reading all records from file and aggregating data */
static ErrorCode statistics_from_file(const char *filename) {
    if (!filename) {
        return ERR_INVALID_INPUT;
    }
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open '%s' for reading: %s\n",
                filename, strerror(errno));
        return ERR_FILE_IO;
    }
    
    char buffer[MAX_LINE_LENGTH];
    size_t line_num = 0;
    size_t count = 0;
    int pass_count = 0, fail_count = 0;
    int min_marks = 100, max_marks = 0;
    long total_marks = 0;
    
    printf("\nCalculating statistics from file: %s\n", filename);
    
    while (fgets(buffer, sizeof(buffer), f)) {
        line_num++;
        
        // Skip comments and empty lines
        if (buffer[0] == '#' || buffer[0] == '\n') continue;
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // Parse: roll|marks|name
        char *p1 = strchr(buffer, '|');
        if (!p1) continue;
        *p1 = '\0';
        
        char *p2 = strchr(p1 + 1, '|');
        if (!p2) continue;
        *p2 = '\0';
        
        int roll = (int)strtol(buffer, NULL, 10);
        int marks = (int)strtol(p1 + 1, NULL, 10);
        
        // Validate data
        if (roll <= 0 || marks < 0 || marks > 100) {
            continue;
        }
        
        count++;
        total_marks += marks;
        
        if (marks >= PASS_THRESHOLD) {
            pass_count++;
        } else {
            fail_count++;
        }
        
        if (marks < min_marks) {
            min_marks = marks;
        }
        
        if (marks > max_marks) {
            max_marks = marks;
        }
    }
    
    fclose(f);
    
    if (count == 0) {
        printf("\nNo valid student records found in the file.\n");
        return SUCCESS;
    }
    
    double avg = (double)total_marks / count;
    double pass_rate = (double)pass_count / count * 100;
    
    printf("----------------------------------------------------------------------------\n");
    printf("Statistics Summary (from file)\n");
    printf("-----------------------------------------------------------------------------\n");
    printf("Total Students:    %zu\n", count);
    printf("Average Marks:     %.2f\n", avg);
    printf("Highest Marks:     %d\n", max_marks);
    printf("Lowest Marks:      %d\n", min_marks);
    printf("Pass Count:        %d (%.1f%%)\n", pass_count, pass_rate);
    printf("Fail Count:        %d\n", fail_count);
    printf("-----------------------------------------------------------------------------\n");
    
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

/* ---------- Input Helpers(This code assissts with input cases and the rest) ---------- */

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
    printf("┌────────────────────────────────────────┐\n");
    printf("│     Student Record System Menu         │\n");
    printf("├────────────────────────────────────────┤\n");
    printf("│  1. Add a student                      │\n");
    printf("│  2. Modify a student                   │\n");
    printf("│  3. Remove a student                   │\n");
    printf("│  4. Display all students               │\n");
    printf("│  5. Search by roll number              │\n");
    printf("│  6. Show statistics                    │\n");
    printf("│  7. Sort by marks (ascending)          │\n");
    printf("│  8. Sort by marks (descending)         │\n");
    printf("│  9. Sort by name                       │\n");
    printf("│ 10. Save to file                       │\n");
    printf("│ 11. Load from file                     │\n");
    printf("│ 12. Quick save                         │\n");
    printf("│  0. Exit                               │\n");
    printf("└────────────────────────────────────────┘\n");
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
        
        if (list.modified) {
            printf("Unsaved changes ");
            if (list.last_filename) {
                printf("(last file: %s)", list.last_filename);
            }
            printf("\n");
        }
        
        int choice = prompt_int("Choose an option (0-12): ", 0, 12);
        
        switch (choice) {
            /* These cases were added so that when adding a student, it automatically saves to file 
               right away, so the data is persistent even if the program crashes */
            case 1: {
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
                    
                    const char *filename = list.last_filename ? list.last_filename : FILENAME;
                    if (save_to_file(&list, filename) == SUCCESS) {
                        printf("Student record saved to '%s'\n", filename);
                    } else {
                        printf("Warning: Student added but failed to save to file.\n");
                    }
                } else {
                    printf("Failed to add student.\n");
                    free_student(s);
                }
                break;
            }
            /* This case was added here so that when modifying, we reload from file first to get 
               the latest data, then allow partial updates (pressing Enter keeps old values) */
            case 2: {
                const char *filename = list.last_filename ? list.last_filename : FILENAME;
                FILE *test_file = fopen(filename, "r");
                if (test_file) {
                    fclose(test_file);
                    load_from_file(&list, filename);
                }
                
                int roll = prompt_int("Enter roll number to modify: ", 1, 99999);
                long idx = find_index_by_roll(&list, roll);
                
                if (idx < 0) {
                    printf("Student with roll %d not found.\n", roll);
                    printf("Make sure the student exists in the file.\n");
                    break;
                }
                
                printf("\nCurrent details:\n");
                display_student(list.items[idx]);
                
                printf("\nEnter new details (press Enter to keep current value):\n");
                
                printf("New roll number (current: %d): ", list.items[idx]->roll);
                char *roll_input = read_line("");
                int new_roll = list.items[idx]->roll;
                if (roll_input && strlen(roll_input) > 0) {
                    char *endptr = NULL;
                    long val = strtol(roll_input, &endptr, 10);
                    if (*endptr == '\0' && val >= 1 && val <= 99999) {
                        new_roll = (int)val;
                    } else {
                        printf("Invalid input, keeping current roll number.\n");
                    }
                }
                free(roll_input);
                
                printf("New name (current: %s): ", list.items[idx]->name ? list.items[idx]->name : "Unnamed");
                char *new_name = read_line("");
                if (!new_name || strlen(new_name) == 0) {
                    free(new_name);
                    new_name = safe_strdup(list.items[idx]->name ? list.items[idx]->name : "Unnamed");
                } else {
                    trim_inplace(new_name);
                    if (strlen(new_name) > MAX_NAME_LENGTH) {
                        new_name[MAX_NAME_LENGTH] = '\0';
                        printf("Name truncated to %d characters.\n", MAX_NAME_LENGTH);
                    }
                }
                
                printf("New marks (current: %d): ", list.items[idx]->marks);
                char *marks_input = read_line("");
                int new_marks = list.items[idx]->marks;
                if (marks_input && strlen(marks_input) > 0) {
                    char *endptr = NULL;
                    long val = strtol(marks_input, &endptr, 10);
                    if (*endptr == '\0' && val >= 0 && val <= 100) {
                        new_marks = (int)val;
                    } else {
                        printf("Invalid input, keeping current marks.\n");
                    }
                }
                free(marks_input);
                
                ErrorCode err = modify_student(&list, (size_t)idx, new_roll, new_name, new_marks);
                free(new_name);
                
                if (err == ERR_DUPLICATE) {
                    printf("Roll number %d already exists!\n", new_roll);
                } else if (err == SUCCESS) {
                    printf("Student modified successfully!\n");
                    
                    if (save_to_file(&list, filename) == SUCCESS) {
                        printf("Changes saved to '%s'\n", filename);
                    } else {
                        printf("Warning: Student modified but failed to save to file.\n");
                    }
                } else {
                    printf("Failed to modify student.\n");
                }
                break;
            }
            
            /* This block loads from file first, then asks for confirmation before deleting,
               and saves immediately after removal so the file stays updated */
            case 3: {
                const char *filename = list.last_filename ? list.last_filename : FILENAME;
                FILE *test_file = fopen(filename, "r");
                if (test_file) {
                    fclose(test_file);
                    load_from_file(&list, filename);
                }
                
                int roll = prompt_int("Enter roll number to remove: ", 1, 99999);
                long idx = find_index_by_roll(&list, roll);
                
                if (idx < 0) {
                    printf("Student with roll %d not found.\n", roll);
                    printf("Make sure the student exists in the file.\n");
                } else {
                    printf("\nRemoving: ");
                    display_student(list.items[idx]);
                    
                    if (prompt_yes_no("Are you sure you want to remove this student? (y/n): ")) {
                        if (remove_student_by_index(&list, (size_t)idx) == SUCCESS) {
                            printf("Student removed successfully!\n");
                            
                            if (save_to_file(&list, filename) == SUCCESS) {
                                printf("Changes saved to '%s'\n", filename);
                            } else {
                                printf("Warning: Student removed but failed to save to file.\n");
                            }
                        } else {
                            printf("Failed to remove student.\n");
                        }
                    } else {
                        printf("Removal cancelled.\n");
                    }
                }
                break;
            }
            
            /* This part reads directly from file instead of memory, so we see what's actually 
               saved in the file, even if memory is out of sync */
            case 4: {
                const char *filename = list.last_filename ? list.last_filename : FILENAME;
                ErrorCode err = display_from_file(filename);
                if (err == ERR_FILE_IO) {
                    printf("File '%s' not found or cannot be read.\n", filename);
                    printf("Make sure you have added students first (option 1).\n");
                }
                break;
            }
            
            /* This case was added here so that searching happens directly in the file,
               so we don't need to load everything into memory first */
            case 5: {
                const char *filename = list.last_filename ? list.last_filename : FILENAME;
                
                FILE *test_file = fopen(filename, "r");
                if (!test_file) {
                    printf("Error: File '%s' not found or cannot be read.\n", filename);
                    printf("Make sure you have added students first (option 1).\n");
                    break;
                }
                fclose(test_file);
                
                int roll = prompt_int("Enter roll number to search: ", 1, 99999);
                ErrorCode err = search_in_file(filename, roll);
                
                if (err == ERR_FILE_IO) {
                    printf("Failed to read from file '%s'.\n", filename);
                } else if (err == ERR_NOT_FOUND) {
                    printf("\nSearch completed - student not found.\n");
                }
                break;
            }
            
            /* This block calculates statistics by reading directly from file, so we get 
               accurate stats based on what's actually saved, not what's in memory */
            case 6: {
                const char *filename = list.last_filename ? list.last_filename : FILENAME;
                
                FILE *test_file = fopen(filename, "r");
                if (!test_file) {
                    printf("Error: File '%s' not found or cannot be read.\n", filename);
                    printf("Make sure you have added students first (option 1).\n");
                    break;
                }
                fclose(test_file);
                
                ErrorCode err = statistics_from_file(filename);
                if (err == ERR_FILE_IO) {
                    printf("Failed to read from file '%s'.\n", filename);
                }
                break;
            }
            
            /* These cases were added so that sorting loads from file, sorts, displays, and asks
               if you want to save the sorted order, so you can preview before committing */
            case 7: {
                const char *filename = list.last_filename ? list.last_filename : FILENAME;
                
                FILE *test_file = fopen(filename, "r");
                if (!test_file) {
                    printf("Error: File '%s' not found.\n", filename);
                    printf("Make sure you have added students first (option 1).\n");
                    break;
                }
                fclose(test_file);
                
                if (load_from_file(&list, filename) != SUCCESS) {
                    printf("Failed to load from file.\n");
                    break;
                }
                
                if (list.size == 0) {
                    printf("No students to sort.\n");
                    break;
                }
                
                sort_students(&list, cmp_marks_asc);
                printf("\nSorted by marks (ascending):\n");
                display_all_students(&list);
                
                if (prompt_yes_no("\nSave sorted order to file? (y/n): ")) {
                    if (save_to_file(&list, filename) == SUCCESS) {
                        printf("Sorted data saved to '%s'\n", filename);
                    } else {
                        printf("Failed to save sorted data.\n");
                    }
                } else {
                    printf("Sorting not saved (file unchanged).\n");
                }
                break;
            }
            
            case 8: {
                const char *filename = list.last_filename ? list.last_filename : FILENAME;
                
                FILE *test_file = fopen(filename, "r");
                if (!test_file) {
                    printf("Error: File '%s' not found.\n", filename);
                    printf("Make sure you have added students first (option 1).\n");
                    break;
                }
                fclose(test_file);
                
                if (load_from_file(&list, filename) != SUCCESS) {
                    printf("Failed to load from file.\n");
                    break;
                }
                
                if (list.size == 0) {
                    printf("No students to sort.\n");
                    break;
                }
                
                sort_students(&list, cmp_marks_desc);
                printf("\nSorted by marks (descending):\n");
                display_all_students(&list);
                
                if (prompt_yes_no("\nSave sorted order to file? (y/n): ")) {
                    if (save_to_file(&list, filename) == SUCCESS) {
                        printf("Sorted data saved to '%s'\n", filename);
                    } else {
                        printf("Failed to save sorted data.\n");
                    }
                } else {
                    printf("Sorting not saved (file unchanged).\n");
                }
                break;
            }
            
            case 9: {
                const char *filename = list.last_filename ? list.last_filename : FILENAME;
                
                FILE *test_file = fopen(filename, "r");
                if (!test_file) {
                    printf("Error: File '%s' not found.\n", filename);
                    printf("Make sure you have added students first (option 1).\n");
                    break;
                }
                fclose(test_file);
                
                if (load_from_file(&list, filename) != SUCCESS) {
                    printf("Failed to load from file.\n");
                    break;
                }
                
                if (list.size == 0) {
                    printf("No students to sort.\n");
                    break;
                }
                
                sort_students(&list, cmp_name_asc);
                printf("\nSorted by name (alphabetically):\n");
                display_all_students(&list);
                
                if (prompt_yes_no("\nSave sorted order to file? (y/n): ")) {
                    if (save_to_file(&list, filename) == SUCCESS) {
                        printf("Sorted data saved to '%s'\n", filename);
                    } else {
                        printf("Failed to save sorted data.\n");
                    }
                } else {
                    printf("Sorting not saved (file unchanged).\n");
                }
                break;
            }
            
            /* This case was added here so that saving allows custom filenames, not just the default,
               so users can create backup files or save to different locations */
            case 10: {
                if (list.size == 0) {
                    printf("No students in memory to save.\n");
                    printf("Load students first (option 11) or add new students (option 1).\n");
                    break;
                }
                
                const char *default_filename = list.last_filename ? list.last_filename : FILENAME;
                
                printf("Current file: %s\n", default_filename);
                if (prompt_yes_no("Save to this file? (y/n): ")) {
                    if (save_to_file(&list, default_filename) == SUCCESS) {
                        printf("Successfully saved %zu records to '%s'\n", list.size, default_filename);
                    } else {
                        printf("Failed to save to '%s'\n", default_filename);
                    }
                } else {
                    char *custom_filename = read_line("Enter filename (or press Enter for default): ");
                    if (custom_filename) {
                        trim_inplace(custom_filename);
                        if (strlen(custom_filename) == 0) {
                            free(custom_filename);
                            custom_filename = safe_strdup(FILENAME);
                        }
                        
                        if (custom_filename) {
                            if (save_to_file(&list, custom_filename) == SUCCESS) {
                                printf("Successfully saved %zu records to '%s'\n", list.size, custom_filename);
                            } else {
                                printf("Failed to save to '%s'\n", custom_filename);
                            }
                            free(custom_filename);
                        }
                    }
                }
                break;
            }

            /* This part allows loading from custom filenames, so you can work with 
               different student record files, not just one default file */
            case 11: {
                const char *default_filename = FILENAME;
                
                printf("Default file: %s\n", default_filename);
                if (prompt_yes_no("Load from this file? (y/n): ")) {
                    ErrorCode err = load_from_file(&list, default_filename);
                    if (err == SUCCESS) {
                        printf("Records loaded successfully into memory.\n");
                    } else if (err == ERR_FILE_IO) {
                        printf("Failed to load from '%s' - file not found or cannot be read.\n", default_filename);
                    } else {
                        printf("Failed to load from file.\n");
                    }
                } else {
                    char *custom_filename = read_line("Enter filename to load: ");
                    if (custom_filename) {
                        trim_inplace(custom_filename);
                        if (strlen(custom_filename) > 0) {
                            ErrorCode err = load_from_file(&list, custom_filename);
                            if (err == SUCCESS) {
                                printf("Records loaded successfully into memory.\n");
                            } else if (err == ERR_FILE_IO) {
                                printf("Failed to load from '%s' - file not found or cannot be read.\n", custom_filename);
                            } else {
                                printf("Failed to load from file.\n");
                            }
                        } else {
                            printf("Invalid filename.\n");
                        }
                        free(custom_filename);
                    }
                }
                break;
            }
              
            /* This case was added here so that quick save remembers the last file used,
               making it faster to save without having to type the filename every time */
            case 12: {
                if (list.size == 0) {
                    printf("No students in memory to save.\n");
                    break;
                }
                
                if (!list.last_filename) {
                    printf("No previous file loaded. Using default: " FILENAME "\n");
                    if (save_to_file(&list, FILENAME) == SUCCESS) {
                        printf("Quick saved %zu records to '" FILENAME "'\n", list.size);
                    } else {
                        printf("Failed to quick save.\n");
                    }
                } else {
                    if (save_to_file(&list, list.last_filename) == SUCCESS) {
                        printf("Quick saved %zu records to '%s'\n", list.size, list.last_filename);
                    } else {
                        printf("Failed to quick save to '%s'\n", list.last_filename);
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
