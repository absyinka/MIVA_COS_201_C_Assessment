#include "student.h"

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