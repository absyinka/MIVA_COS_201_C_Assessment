#include "student.h"

/* ---------- StudentList Management ---------- */

ErrorCode init_student_list(StudentList *list) {
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

void free_student(Student *s) {
    if (!s) {
        return;
    }

    free(s->name);
    free(s);
}

void free_student_list(StudentList *list) {
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

ErrorCode ensure_capacity(StudentList *list) {
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

Student *create_student(int roll, const char *name, int marks) {
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

long find_index_by_roll(const StudentList *list, int roll) {
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

ErrorCode add_student(StudentList *list, Student *s) {
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

ErrorCode remove_student_by_index(StudentList *list, size_t index) {
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

ErrorCode modify_student(
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

/* ---------- Search & Sort ---------- */
Student *search_by_roll(const StudentList *list, int roll) {
    long idx = find_index_by_roll(list, roll);
    return (idx >= 0) ? list->items[idx] : NULL;
}

int cmp_marks_asc(const void *a, const void *b) {
    const Student *sa = *(const Student**)a;
    const Student *sb = *(const Student**)b;
    return sa->marks - sb->marks;
}

int cmp_marks_desc(const void *a, const void *b) {
    return -cmp_marks_asc(a, b);
}

int cmp_name_asc(const void *a, const void *b) {
    const Student *sa = *(const Student**)a;
    const Student *sb = *(const Student**)b;
    return strcmp(sa->name, sb->name);
}

void sort_students(StudentList *list, int (*cmp)(const void*, const void*)) {
    if (!list || list->size < 2) {
        return;
    }

    qsort(list->items, list->size, sizeof(Student*), cmp);
    list->modified = 1;  // Mark as modified since order changed
}
