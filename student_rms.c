/*
=================================================================
  MOUNT KENYA UNIVERSITY
  STUDENT RECORD MANAGEMENT SYSTEM  (C Language)
  Course  : Bachelor of Science in Computer Science
  Lecturer: [Your Lecturer Name]
  Date    : March 2026

  HOW TO COMPILE:
    gcc student_rms.c -o student_rms

  HOW TO RUN:
    Linux/Mac : ./student_rms
    Windows   : student_rms.exe

  MENU:
    1. Add new student record
    2. Display all student records
    3. Search student by registration number
    4. Update student information
    5. Delete student record
    6. Save records to file
    7. Load records from file
    0. Exit
=================================================================
*/

/* ── Include standard libraries ── */
#include <stdio.h>    /* printf, scanf, fgets, fopen, fclose, fprintf, fscanf */
#include <stdlib.h>   /* atoi, exit                                           */
#include <string.h>   /* strcpy, strcmp, strlen, strcspn                      */
#include <ctype.h>    /* toupper, isdigit, isalpha, isspace                   */

/* ── Platform detection: different command to clear the screen ── */
#ifdef _WIN32
    #define CLEAR_CMD "cls"    /* Windows uses "cls"        */
#else
    #define CLEAR_CMD "clear"  /* Linux / Mac uses "clear"  */
#endif

/* ──────────────────────────────────────────
   CONSTANTS  (fixed limits for our arrays)
────────────────────────────────────────── */
#define MAX_STUDENTS  100   /* Maximum number of students the system can hold */
#define MAX_NAME       60   /* Maximum characters in a student's name          */
#define MAX_REG        25   /* Maximum characters in a registration number     */
#define MAX_COURSE     50   /* Maximum characters in a course name             */
#define MAX_EMAIL      60   /* Maximum characters in an email address          */
#define MAX_PHONE      20   /* Maximum characters in a phone number            */
#define MAX_YEAR        5   /* Maximum characters in year of study (e.g. "2") */
#define MAX_FILENAME   80   /* Maximum characters in a filename                */
#define WIDTH          60   /* Width of the display border on screen           */
#define DEFAULT_FILE   "students.dat"  /* Default filename for saving/loading */

/* ──────────────────────────────────────────
   DATA STRUCTURE: Student record blueprint
   A struct groups related fields together
   like a paper form with labelled boxes.
────────────────────────────────────────── */
typedef struct {
    int  id;                  /* Auto-generated unique ID number (1, 2, 3...) */
    char reg_number[MAX_REG]; /* Registration number: e.g. BSCSR/2025/61955   */
    char name[MAX_NAME];      /* Full name: e.g. MUKIZA JEAN                  */
    char course[MAX_COURSE];  /* Course name: e.g. BSc Computer Science        */
    char year[MAX_YEAR];      /* Year of study: "1", "2", "3", or "4"          */
    char email[MAX_EMAIL];    /* Email: e.g. mukiza@students.mku.ac.ke         */
    char phone[MAX_PHONE];    /* Phone: e.g. +250 700 000 000                  */
    float gpa;                /* Grade Point Average: e.g. 3.75                */
} Student;

/* ──────────────────────────────────────────
   GLOBAL VARIABLES
   These are accessible by ALL functions.
────────────────────────────────────────── */
static Student records[MAX_STUDENTS]; /* The database: array of Student structs */
static int     record_count = 0;      /* How many students are currently stored  */
static int     next_id      = 1;      /* Next ID to assign when adding a student */

/* ──────────────────────────────────────────
   UTILITY / HELPER FUNCTIONS
   Small reusable tools used throughout.
────────────────────────────────────────── */

/* Clear the terminal screen */
void clear_screen(void) {
    system(CLEAR_CMD);
}

/* Print a line of dashes across the screen */
void print_line(void) {
    int i;
    for (i = 0; i < WIDTH; i++) putchar('-');
    putchar('\n');
}

/* Print a line of equals signs (for headers) */
void print_double_line(void) {
    int i;
    for (i = 0; i < WIDTH; i++) putchar('=');
    putchar('\n');
}

/* Print a success message */
void print_ok(const char *msg) {
    printf("\n  [OK]  %s\n", msg);
}

/* Print an error message */
void print_err(const char *msg) {
    printf("\n  [ERR] %s\n", msg);
}

/* Wait for user to press Enter before continuing */
void pause_screen(void) {
    printf("\n  Press Enter to continue...");
    getchar(); /* Wait for Enter key */
}

/* Read a line from keyboard, remove trailing newline */
void read_line(char *buf, int size) {
    if (fgets(buf, size, stdin)) {
        /* Replace the newline character '\n' with end-of-string '\0' */
        buf[strcspn(buf, "\n")] = '\0';
        /* Remove leading and trailing spaces */
        int start = 0;
        while (buf[start] && isspace((unsigned char)buf[start])) start++;
        memmove(buf, buf + start, strlen(buf + start) + 1);
        int len = (int)strlen(buf);
        while (len > 0 && isspace((unsigned char)buf[len-1])) buf[--len] = '\0';
    } else {
        buf[0] = '\0'; /* If reading failed, return empty string */
    }
}

/* Convert a string to uppercase (modifies in place) */
void str_upper(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

/* Keep asking until user types something non-empty */
void get_non_empty(const char *prompt, char *buf, int size) {
    while (1) {
        printf("%s", prompt);
        read_line(buf, size);
        if (strlen(buf) > 0) return; /* Got something -- exit loop */
        printf("  Warning: This field cannot be empty.\n");
    }
}

/* Keep asking until user types a valid float between lo and hi */
float get_float(const char *prompt, float lo, float hi) {
    char buf[20];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (strlen(buf) > 0) {
            float v = (float)atof(buf); /* atof converts "3.75" to 3.75 */
            if (v >= lo && v <= hi) return v;
        }
        printf("  Warning: Enter a number between %.1f and %.1f\n", lo, hi);
    }
}

/* Keep asking until user types a valid integer between lo and hi */
int get_int_range(const char *prompt, int lo, int hi) {
    char buf[10];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (strlen(buf) > 0) {
            /* Check every character is a digit */
            int valid = 1, i;
            for (i = 0; buf[i]; i++)
                if (!isdigit((unsigned char)buf[i])) { valid = 0; break; }
            if (valid) {
                int v = atoi(buf);
                if (v >= lo && v <= hi) return v;
            }
        }
        printf("  Warning: Enter a whole number between %d and %d\n", lo, hi);
    }
}

/*
   Validate registration number format: LETTERS/YEAR/DIGITS
   e.g. BSCSR/2025/61955  or  CS/2024/001
   Returns 1 if valid, 0 if invalid.
*/
int validate_reg(const char *reg) {
    int i = 0, len = (int)strlen(reg);
    int letters = 0, d1 = 0, d2 = 0;

    /* Count letters at start (2 to 6) */
    while (i < len && isalpha((unsigned char)reg[i])) { i++; letters++; }
    if (letters < 2 || letters > 6) return 0;

    /* First slash */
    if (i >= len || reg[i] != '/') return 0;
    i++;

    /* Exactly 4 digits (year) */
    while (i < len && isdigit((unsigned char)reg[i])) { i++; d1++; }
    if (d1 != 4) return 0;

    /* Second slash */
    if (i >= len || reg[i] != '/') return 0;
    i++;

    /* 3 to 6 digits (student number) */
    while (i < len && isdigit((unsigned char)reg[i])) { i++; d2++; }
    if (d2 < 3 || d2 > 6) return 0;

    return (i == len); /* Must have consumed whole string */
}

/* Print the MKU header banner */
void print_banner(void) {
    clear_screen();
    print_double_line();
    printf("   MOUNT KENYA UNIVERSITY\n");
    printf("   STUDENT RECORD MANAGEMENT SYSTEM\n");
    printf("   Main Campus, Kigali  |  2026\n");
    print_double_line();
}

/* Print the main menu options */
void print_menu(void) {
    print_banner();
    printf("\n   Total Records: %d / %d\n\n", record_count, MAX_STUDENTS);
    print_line();
    printf("   1. Add New Student Record\n");
    printf("   2. Display All Student Records\n");
    printf("   3. Search Student by Registration Number\n");
    printf("   4. Update Student Information\n");
    printf("   5. Delete Student Record\n");
    printf("   6. Save Records to File\n");
    printf("   7. Load Records from File\n");
    printf("   0. Exit\n");
    print_line();
    printf("\n   Enter your choice: ");
}

/* Print one student record in a formatted box */
void print_student(const Student *s) {
    print_line();
    printf("   ID          : %d\n",   s->id);
    printf("   Reg Number  : %s\n",   s->reg_number);
    printf("   Name        : %s\n",   s->name);
    printf("   Course      : %s\n",   s->course);
    printf("   Year        : %s\n",   s->year);
    printf("   Email       : %s\n",   s->email);
    printf("   Phone       : %s\n",   s->phone);
    printf("   GPA         : %.2f\n", s->gpa);
    print_line();
}

/* ──────────────────────────────────────────
   FEATURE 1: ADD NEW STUDENT RECORD
   Asks the user for all student details,
   validates them, then saves to our array.
────────────────────────────────────────── */
void add_student(void) {
    char reg[MAX_REG];  /* Temporary storage for reg number input */
    int  i;             /* Loop counter for duplicate check        */

    print_banner();
    printf("\n   ADD NEW STUDENT RECORD\n\n");
    print_line();

    /* Check if the database is full */
    if (record_count >= MAX_STUDENTS) {
        print_err("Database is full. Cannot add more records.");
        pause_screen();
        return;
    }

    /* ── Get and validate registration number ── */
    while (1) {
        printf("   Registration Number (e.g. BSCSR/2025/61955): ");
        read_line(reg, sizeof(reg));
        str_upper(reg); /* Convert to uppercase */

        if (strlen(reg) == 0) {
            print_err("Registration number cannot be empty.");
            continue;
        }
        if (!validate_reg(reg)) {
            print_err("Invalid format. Use: LETTERS/YEAR/DIGITS (e.g. CS/2025/001)");
            continue;
        }

        /* Check for duplicate: no two students can share a reg number */
        int duplicate = 0;
        for (i = 0; i < record_count; i++) {
            if (strcmp(records[i].reg_number, reg) == 0) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate) {
            print_err("That registration number already exists.");
            continue;
        }

        break; /* Valid and unique -- exit loop */
    }

    /* ── Get remaining fields ── */
    /* We get a pointer to the next empty slot in the array */
    Student *s = &records[record_count];

    /* Assign auto-generated ID */
    s->id = next_id++;

    /* Copy validated reg number into the student struct */
    strcpy(s->reg_number, reg);

    /* Get full name */
    get_non_empty("   Full Name         : ", s->name, sizeof(s->name));

    /* Get course */
    get_non_empty("   Course            : ", s->course, sizeof(s->course));

    /* Get year of study (1-4 only) */
    printf("   Year of Study (1-4): ");
    {
        int yr = get_int_range("   Year of Study (1-4): ", 1, 4);
        /* Convert the integer year back to a string like "2" */
        /* We use sprintf to convert int to string             */
        /* But actually we already called get_int_range which  */
        /* returned an int. Let's store it as string.          */
        /* sprintf writes a formatted string into a buffer.    */
        s->year[0] = '0' + yr; /* Simple way: '0'+2 = '2'    */
        s->year[1] = '\0';     /* End of string marker        */
    }

    /* Get email */
    while (1) {
        get_non_empty("   Email Address     : ", s->email, sizeof(s->email));
        /* Check email has an @ symbol */
        if (strchr(s->email, '@') != NULL) break;
        print_err("Invalid email -- must contain @");
    }

    /* Get phone */
    get_non_empty("   Phone Number      : ", s->phone, sizeof(s->phone));

    /* Get GPA (Grade Point Average, 0.0 to 4.0) */
    s->gpa = get_float("   GPA (0.0 - 4.0)   : ", 0.0f, 4.0f);

    /* Increase the record count to "save" this new student */
    record_count++;

    print_ok("Student record added successfully!");
    printf("\n");
    print_student(s); /* Show what was just saved */
    pause_screen();
}

/* ──────────────────────────────────────────
   FEATURE 2: DISPLAY ALL STUDENT RECORDS
   Loops through the array and prints every
   student record stored in the system.
────────────────────────────────────────── */
void display_all(void) {
    int i; /* Loop counter */

    print_banner();
    printf("\n   ALL STUDENT RECORDS\n\n");

    /* Check if there is anything to show */
    if (record_count == 0) {
        print_err("No records found. Add some students first.");
        pause_screen();
        return;
    }

    printf("   Total Records: %d\n\n", record_count);

    /* Loop through every record and print it */
    for (i = 0; i < record_count; i++) {
        print_student(&records[i]); /* & gives the address of records[i] */
    }

    pause_screen();
}

/* ──────────────────────────────────────────
   FEATURE 3: SEARCH BY REGISTRATION NUMBER
   Asks for a reg number and finds the
   matching student if they exist.
────────────────────────────────────────── */
void search_student(void) {
    char search_reg[MAX_REG]; /* Stores the reg number to search for */
    int  i;                   /* Loop counter                        */
    int  found = 0;           /* Flag: 1 = found, 0 = not found      */

    print_banner();
    printf("\n   SEARCH STUDENT BY REGISTRATION NUMBER\n\n");
    print_line();

    printf("   Enter Registration Number: ");
    read_line(search_reg, sizeof(search_reg));
    str_upper(search_reg); /* Convert to uppercase for comparison */

    if (strlen(search_reg) == 0) {
        print_err("Registration number cannot be empty.");
        pause_screen();
        return;
    }

    /* Search through all records */
    for (i = 0; i < record_count; i++) {
        /* strcmp returns 0 if the two strings are equal */
        if (strcmp(records[i].reg_number, search_reg) == 0) {
            print_ok("Student found:");
            print_student(&records[i]);
            found = 1;
            break; /* Stop searching once found */
        }
    }

    if (!found) {
        print_err("No student found with that registration number.");
    }

    pause_screen();
}

/* ──────────────────────────────────────────
   FEATURE 4: UPDATE STUDENT INFORMATION
   Finds a student by reg number and lets
   the user change any of their details.
────────────────────────────────────────── */
void update_student(void) {
    char search_reg[MAX_REG]; /* Reg number to search for         */
    int  i;                   /* Loop counter                     */
    int  found = 0;           /* Flag: found or not               */
    char choice[4];           /* Menu choice for what to update   */

    print_banner();
    printf("\n   UPDATE STUDENT INFORMATION\n\n");
    print_line();

    printf("   Enter Registration Number to update: ");
    read_line(search_reg, sizeof(search_reg));
    str_upper(search_reg);

    /* Find the student */
    for (i = 0; i < record_count; i++) {
        if (strcmp(records[i].reg_number, search_reg) == 0) {
            found = 1;

            /* Show current record before updating */
            printf("\n   Current Record:\n");
            print_student(&records[i]);

            /* Give the user a sub-menu to choose what to update */
            printf("\n   What would you like to update?\n\n");
            printf("   1. Name\n");
            printf("   2. Course\n");
            printf("   3. Year of Study\n");
            printf("   4. Email\n");
            printf("   5. Phone Number\n");
            printf("   6. GPA\n");
            printf("   0. Cancel\n\n");
            printf("   Choice: ");
            read_line(choice, sizeof(choice));

            /* Get a pointer to this student for shorter typing */
            Student *s = &records[i];

            if (strcmp(choice, "1") == 0) {
                /* Update name */
                get_non_empty("   New Name     : ", s->name, sizeof(s->name));
                print_ok("Name updated.");

            } else if (strcmp(choice, "2") == 0) {
                /* Update course */
                get_non_empty("   New Course   : ", s->course, sizeof(s->course));
                print_ok("Course updated.");

            } else if (strcmp(choice, "3") == 0) {
                /* Update year */
                int yr = get_int_range("   New Year (1-4): ", 1, 4);
                s->year[0] = '0' + yr;
                s->year[1] = '\0';
                print_ok("Year updated.");

            } else if (strcmp(choice, "4") == 0) {
                /* Update email -- validate it has @ */
                char email[MAX_EMAIL];
                while (1) {
                    get_non_empty("   New Email    : ", email, sizeof(email));
                    if (strchr(email, '@') != NULL) {
                        strcpy(s->email, email);
                        break;
                    }
                    print_err("Invalid email -- must contain @");
                }
                print_ok("Email updated.");

            } else if (strcmp(choice, "5") == 0) {
                /* Update phone */
                get_non_empty("   New Phone    : ", s->phone, sizeof(s->phone));
                print_ok("Phone updated.");

            } else if (strcmp(choice, "6") == 0) {
                /* Update GPA */
                s->gpa = get_float("   New GPA (0.0-4.0): ", 0.0f, 4.0f);
                print_ok("GPA updated.");

            } else if (strcmp(choice, "0") == 0) {
                printf("   Update cancelled.\n");
            } else {
                print_err("Invalid choice.");
            }

            /* Show the updated record */
            if (strcmp(choice, "0") != 0) {
                printf("\n   Updated Record:\n");
                print_student(s);
            }

            break; /* Found and processed -- stop looping */
        }
    }

    if (!found) {
        print_err("No student found with that registration number.");
    }

    pause_screen();
}

/* ──────────────────────────────────────────
   FEATURE 5: DELETE STUDENT RECORD
   Finds a student and removes them by
   shifting all later records one step left.
────────────────────────────────────────── */
void delete_student(void) {
    char search_reg[MAX_REG]; /* Reg number to search for       */
    int  i;                   /* Loop counter                   */
    int  found = 0;           /* Flag                           */
    char confirm[4];          /* Stores user's yes/no answer    */

    print_banner();
    printf("\n   DELETE STUDENT RECORD\n\n");
    print_line();

    printf("   Enter Registration Number to delete: ");
    read_line(search_reg, sizeof(search_reg));
    str_upper(search_reg);

    /* Find the student */
    for (i = 0; i < record_count; i++) {
        if (strcmp(records[i].reg_number, search_reg) == 0) {
            found = 1;

            /* Show the record before asking for confirmation */
            printf("\n   Record to delete:\n");
            print_student(&records[i]);

            /* Ask for confirmation before deleting */
            printf("   Are you sure you want to delete this record? (y/n): ");
            read_line(confirm, sizeof(confirm));

            if (confirm[0] == 'y' || confirm[0] == 'Y') {
                /* Delete by shifting all records after position i
                   one slot to the left, overwriting the deleted one.
                   Example: if we delete index 2:
                   Before: [0] [1] [2] [3] [4]
                   After:  [0] [1] [3] [4] (3 moved to slot 2, 4 to slot 3)
                */
                int j;
                for (j = i; j < record_count - 1; j++) {
                    records[j] = records[j + 1]; /* Copy next into current */
                }
                record_count--; /* One fewer record now */
                print_ok("Student record deleted successfully.");
            } else {
                printf("\n   Deletion cancelled.\n");
            }

            break; /* Stop searching */
        }
    }

    if (!found) {
        print_err("No student found with that registration number.");
    }

    pause_screen();
}

/* ──────────────────────────────────────────
   FEATURE 6: SAVE RECORDS TO FILE
   Writes all student data to a text file
   on disk so data is not lost when program
   closes.
────────────────────────────────────────── */
void save_to_file(void) {
    char filename[MAX_FILENAME]; /* Name of the file to save to */
    FILE *fp;                    /* FILE pointer for file operations */
    int  i;                      /* Loop counter */

    print_banner();
    printf("\n   SAVE RECORDS TO FILE\n\n");
    print_line();

    if (record_count == 0) {
        print_err("No records to save.");
        pause_screen();
        return;
    }

    /* Ask for filename, or use default */
    printf("   Filename [%s]: ", DEFAULT_FILE);
    read_line(filename, sizeof(filename));
    if (strlen(filename) == 0) strcpy(filename, DEFAULT_FILE);

    /* Open file for writing ("w" = create or overwrite) */
    fp = fopen(filename, "w");
    if (!fp) {
        print_err("Could not open file for writing. Check permissions.");
        pause_screen();
        return;
    }

    /* Write the count as the first line so we know how many to load */
    fprintf(fp, "%d\n", record_count);

    /* Write each student's data, one field per line */
    for (i = 0; i < record_count; i++) {
        Student *s = &records[i];
        fprintf(fp, "%d\n",   s->id);
        fprintf(fp, "%s\n",   s->reg_number);
        fprintf(fp, "%s\n",   s->name);
        fprintf(fp, "%s\n",   s->course);
        fprintf(fp, "%s\n",   s->year);
        fprintf(fp, "%s\n",   s->email);
        fprintf(fp, "%s\n",   s->phone);
        fprintf(fp, "%.2f\n", s->gpa);
    }

    /* Always close the file when done to save changes to disk */
    fclose(fp);

    printf("\n  [OK]  %d record(s) saved to '%s'.\n", record_count, filename);
    pause_screen();
}

/* ──────────────────────────────────────────
   FEATURE 7: LOAD RECORDS FROM FILE
   Reads student data from a saved file
   back into the program's memory.
────────────────────────────────────────── */
void load_from_file(void) {
    char filename[MAX_FILENAME]; /* Name of the file to load from */
    FILE *fp;                    /* FILE pointer                   */
    int  count;                  /* Number of records in the file  */
    int  i;                      /* Loop counter                   */

    print_banner();
    printf("\n   LOAD RECORDS FROM FILE\n\n");
    print_line();

    /* Ask for filename, or use default */
    printf("   Filename [%s]: ", DEFAULT_FILE);
    read_line(filename, sizeof(filename));
    if (strlen(filename) == 0) strcpy(filename, DEFAULT_FILE);

    /* Open file for reading ("r" = read only) */
    fp = fopen(filename, "r");
    if (!fp) {
        print_err("File not found. Make sure the file exists.");
        pause_screen();
        return;
    }

    /* Warn if loading will replace existing data */
    if (record_count > 0) {
        char confirm[4];
        printf("\n   Warning: Loading will replace %d existing record(s).\n", record_count);
        printf("   Continue? (y/n): ");
        read_line(confirm, sizeof(confirm));
        if (confirm[0] != 'y' && confirm[0] != 'Y') {
            fclose(fp);
            printf("\n   Load cancelled.\n");
            pause_screen();
            return;
        }
    }

    /* Read the number of records stored in the file */
    if (fscanf(fp, "%d\n", &count) != 1) {
        print_err("File format is invalid.");
        fclose(fp);
        pause_screen();
        return;
    }

    /* Limit to our maximum capacity */
    if (count > MAX_STUDENTS) count = MAX_STUDENTS;

    /* Read each student's data from the file */
    record_count = 0; /* Reset before loading */
    next_id = 1;

    for (i = 0; i < count; i++) {
        Student *s = &records[i];
        char line[100];

        /* fscanf reads formatted data from file (like scanf but from file) */
        if (fscanf(fp, "%d\n", &s->id) != 1) break;

        /* fgets reads one full line including spaces */
        if (!fgets(s->reg_number, sizeof(s->reg_number), fp)) break;
        s->reg_number[strcspn(s->reg_number, "\n")] = '\0'; /* Remove newline */

        if (!fgets(s->name, sizeof(s->name), fp)) break;
        s->name[strcspn(s->name, "\n")] = '\0';

        if (!fgets(s->course, sizeof(s->course), fp)) break;
        s->course[strcspn(s->course, "\n")] = '\0';

        if (!fgets(s->year, sizeof(s->year), fp)) break;
        s->year[strcspn(s->year, "\n")] = '\0';

        if (!fgets(s->email, sizeof(s->email), fp)) break;
        s->email[strcspn(s->email, "\n")] = '\0';

        if (!fgets(s->phone, sizeof(s->phone), fp)) break;
        s->phone[strcspn(s->phone, "\n")] = '\0';

        if (!fgets(line, sizeof(line), fp)) break;
        s->gpa = (float)atof(line); /* Convert text "3.75" to float 3.75 */

        record_count++; /* Count this successfully loaded record */

        /* Keep next_id above highest ID loaded */
        if (s->id >= next_id) next_id = s->id + 1;
    }

    fclose(fp); /* Close the file when done */

    printf("\n  [OK]  %d record(s) loaded from '%s'.\n", record_count, filename);
    pause_screen();
}

/* ──────────────────────────────────────────
   ENTRY POINT: main()
   This is where the program starts.
   Shows the menu in a loop until user exits.
────────────────────────────────────────── */
int main(void) {
    char choice[4]; /* Stores the user's menu choice */

    /* Main loop -- keeps running until user chooses 0 (Exit) */
    while (1) {
        print_menu(); /* Show the menu                              */
        read_line(choice, sizeof(choice)); /* Get the user's choice */

        if      (strcmp(choice, "1") == 0) add_student();
        else if (strcmp(choice, "2") == 0) display_all();
        else if (strcmp(choice, "3") == 0) search_student();
        else if (strcmp(choice, "4") == 0) update_student();
        else if (strcmp(choice, "5") == 0) delete_student();
        else if (strcmp(choice, "6") == 0) save_to_file();
        else if (strcmp(choice, "7") == 0) load_from_file();
        else if (strcmp(choice, "0") == 0) {
            /* Exit the program */
            print_banner();
            printf("\n   Thank you for using MKU Student Record System.\n");
            printf("   Goodbye!\n\n");
            print_double_line();
            break; /* Exit the while loop -- program ends */
        } else {
            print_err("Invalid option. Please enter a number from 0 to 7.");
            pause_screen();
        }
    }

    return 0; /* Return 0 = program completed successfully */
}

/*
=================================================================
  END OF PROGRAM
  Compile: gcc student_rms.c -o student_rms
  Run    : ./student_rms
=================================================================
*/
