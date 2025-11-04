#ifndef STUDENT_H
#define STUDENT_H

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

/* ---------- Memory Management ---------- */
char *safe_strdup(const char *s);
char *read_line(const char *prompt);
void trim_inplace(char *s);

/* ---------- StudentList Management ---------- */
ErrorCode init_student_list(StudentList *list);
void free_student(Student *s);
void free_student_list(StudentList *list);
ErrorCode ensure_capacity(StudentList *list);

/* ---------- Student Operations ---------- */
Student *create_student(int roll, const char *name, int marks);
long find_index_by_roll(const StudentList *list, int roll);
ErrorCode add_student(StudentList *list, Student *s);
ErrorCode remove_student_by_index(StudentList *list, size_t index);
ErrorCode modify_student(StudentList *list, size_t index, int new_roll, const char *new_name, int new_marks);

/* ---------- Display Functions ---------- */
void display_student(const Student *s);
void display_all_students(const StudentList *list);
void display_statistics(const StudentList *list);

/* ---------- File Operations ---------- */
ErrorCode save_to_file(StudentList *list, const char *filename);
ErrorCode load_from_file(StudentList *list, const char *filename);

/* ---------- Search & Sort ---------- */
Student *search_by_roll(const StudentList *list, int roll);
int cmp_marks_asc(const void *a, const void *b);
int cmp_marks_desc(const void *a, const void *b);
int cmp_name_asc(const void *a, const void *b);
void sort_students(StudentList *list, int (*cmp)(const void*, const void*));

/* ---------- Input Helpers ---------- */
int prompt_yes_no(const char *prompt);
void auto_save_prompt(StudentList *list);
int prompt_int(const char *prompt, int min, int max);
ErrorCode prompt_student_input(int *out_roll, char **out_name, int *out_marks);

/* ---------- Menu System ---------- */
void show_menu(void);

#endif // STUDENT_H
