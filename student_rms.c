#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
    #define CLEAR_CMD "cls"
#else
    #define CLEAR_CMD "clear"
#endif

#define MAX_STUDENTS  100
#define MAX_NAME       60
#define MAX_REG        25
#define MAX_COURSE     50
#define MAX_EMAIL      60
#define MAX_PHONE      20
#define MAX_YEAR        5
#define MAX_FILENAME   80
#define WIDTH          60
#define DEFAULT_FILE   "students.dat"

typedef struct {
    int   id;
    char  reg_number[MAX_REG];
    char  name[MAX_NAME];
    char  course[MAX_COURSE];
    char  year[MAX_YEAR];
    char  email[MAX_EMAIL];
    char  phone[MAX_PHONE];
    float gpa;
} Student;

static Student records[MAX_STUDENTS];
static int     record_count = 0;
static int     next_id      = 1;

// --- UTILITY FUNCTIONS ---

void clear_screen(void) { system(CLEAR_CMD); }

void print_line(void) {
    for (int i = 0; i < WIDTH; i++) putchar('-');
    putchar('\n');
}

void print_double_line(void) {
    for (int i = 0; i < WIDTH; i++) putchar('=');
    putchar('\n');
}

void print_ok(const char *msg) { printf("\n  [OK]  %s\n", msg); }
void print_err(const char *msg) { printf("\n  [ERR] %s\n", msg); }

void pause_screen(void) {
    printf("\n  Press Enter to continue...");
    getchar();
}

void read_line(char *buf, int size) {
    if (fgets(buf, size, stdin)) {
        buf[strcspn(buf, "\n")] = '\0';
        int start = 0;
        while (buf[start] && isspace((unsigned char)buf[start])) start++;
        memmove(buf, buf + start, strlen(buf + start) + 1);
        int len = (int)strlen(buf);
        while (len > 0 && isspace((unsigned char)buf[len-1])) buf[--len] = '\0';
    } else {
        buf[0] = '\0';
    }
}

void str_upper(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

void get_non_empty(const char *prompt, char *buf, int size) {
    while (1) {
        printf("%s", prompt);
        read_line(buf, size);
        if (strlen(buf) > 0) return;
        printf("  Warning: This field cannot be empty.\n");
    }
}

float get_float(const char *prompt, float lo, float hi) {
    char buf[20];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (strlen(buf) > 0) {
            float v = (float)atof(buf);
            if (v >= lo && v <= hi) return v;
        }
        printf("  Warning: Enter a number between %.1f and %.1f\n", lo, hi);
    }
}

int get_int_range(const char *prompt, int lo, int hi) {
    char buf[10];
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (strlen(buf) > 0) {
            int valid = 1;
            for (int i = 0; buf[i]; i++)
                if (!isdigit((unsigned char)buf[i])) { valid = 0; break; }
            if (valid) {
                int v = atoi(buf);
                if (v >= lo && v <= hi) return v;
            }
        }
        printf("  Warning: Enter a whole number between %d and %d\n", lo, hi);
    }
}

int validate_reg(const char *reg) {
    int i = 0, len = (int)strlen(reg), letters = 0, d1 = 0, d2 = 0;
    while (i < len && isalpha((unsigned char)reg[i])) { i++; letters++; }
    if (letters < 2 || letters > 6) return 0;
    if (i >= len || reg[i] != '/') return 0;
    i++;
    while (i < len && isdigit((unsigned char)reg[i])) { i++; d1++; }
    if (d1 != 4) return 0;
    if (i >= len || reg[i] != '/') return 0;
    i++;
    while (i < len && isdigit((unsigned char)reg[i])) { i++; d2++; }
    if (d2 < 3 || d2 > 6) return 0;
    return (i == len);
}

// Checks if an email exists. Ignores the current student's index during updates.
int is_email_taken(const char *email, int ignore_index) {
    for (int i = 0; i < record_count; i++) {
        if (i != ignore_index && strcmp(records[i].email, email) == 0) return 1;
    }
    return 0;
}

// Checks if a phone exists. Ignores the current student's index during updates.
int is_phone_taken(const char *phone, int ignore_index) {
    for (int i = 0; i < record_count; i++) {
        if (i != ignore_index && strcmp(records[i].phone, phone) == 0) return 1;
    }
    return 0;
}

void print_banner(void) {
    clear_screen();
    print_double_line();
    printf("   MOUNT KIGALI UNIVERSITY\n");
    printf("   STUDENT RECORD MANAGEMENT SYSTEM\n");
    print_double_line();
}

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

// --- CORE FEATURES ---

void add_student(void) {
    char reg[MAX_REG];
    print_banner();
    printf("\n   ADD NEW STUDENT RECORD\n\n");
    print_line();

    if (record_count >= MAX_STUDENTS) {
        print_err("Database is full. Cannot add more records.");
        pause_screen();
        return;
    }

    while (1) {
        printf("   Registration Number (e.g. BSCSR/2025/61955): ");
        read_line(reg, sizeof(reg));
        str_upper(reg);

        if (strlen(reg) == 0) { print_err("Cannot be empty."); continue; }
        if (!validate_reg(reg)) { print_err("Invalid format. Use: LETTERS/YEAR/DIGITS"); continue; }

        int duplicate = 0;
        for (int i = 0; i < record_count; i++) {
            if (strcmp(records[i].reg_number, reg) == 0) { duplicate = 1; break; }
        }
        if (duplicate) { print_err("That registration number already exists."); continue; }
        break; 
    }

    Student *s = &records[record_count];
    s->id = next_id++;
    strcpy(s->reg_number, reg);

    get_non_empty("   Full Name         : ", s->name, sizeof(s->name));
    get_non_empty("   Course            : ", s->course, sizeof(s->course));

    int yr = get_int_range("   Year of Study (1-4): ", 1, 4);
    s->year[0] = '0' + yr;
    s->year[1] = '\0';

    while (1) {
        get_non_empty("   Email Address     : ", s->email, sizeof(s->email));
        if (strchr(s->email, '@') == NULL) {
            print_err("Invalid email -- must contain @");
        } else if (is_email_taken(s->email, -1)) {
            print_err("Email already in use.");
        } else {
            break;
        }
    }

    while (1) {
        get_non_empty("   Phone Number      : ", s->phone, sizeof(s->phone));
        if (is_phone_taken(s->phone, -1)) {
            print_err("Phone number already in use.");
        } else {
            break;
        }
    }

    s->gpa = get_float("   GPA (0.0 - 4.0)   : ", 0.0f, 4.0f);
    record_count++;

    print_ok("Student record added successfully!");
    pause_screen();
}

void display_all(void) {
    print_banner();
    printf("\n   ALL STUDENT RECORDS\n\n");

    if (record_count == 0) {
        print_err("No records found.");
        pause_screen();
        return;
    }

    // Table Display Format
    printf("%-4s | %-18s | %-20s | %-15s | %-3s | %-25s | %-15s | %-4s\n", 
           "ID", "Reg Number", "Name", "Course", "Yr", "Email", "Phone", "GPA");
    for (int i = 0; i < 115; i++) putchar('-');
    putchar('\n');

    for (int i = 0; i < record_count; i++) {
        Student *s = &records[i];
        // .20s truncates strings that are too long to maintain table structure
        printf("%-4d | %-18.18s | %-20.20s | %-15.15s | %-3.3s | %-25.25s | %-15.15s | %-4.2f\n",
               s->id, s->reg_number, s->name, s->course, s->year, s->email, s->phone, s->gpa);
    }
    printf("\n   Total Records: %d\n\n", record_count);
    pause_screen();
}

void search_student(void) {
    char search_reg[MAX_REG];
    int  found = 0;

    print_banner();
    printf("\n   SEARCH STUDENT\n\n");
    print_line();

    printf("   Enter Registration Number: ");
    read_line(search_reg, sizeof(search_reg));
    str_upper(search_reg);

    for (int i = 0; i < record_count; i++) {
        if (strcmp(records[i].reg_number, search_reg) == 0) {
            print_ok("Student found:");
            print_student(&records[i]);
            found = 1;
            break;
        }
    }
    if (!found) print_err("No student found with that registration number.");
    pause_screen();
}



void update_student(void) {
    char search_reg[MAX_REG];
    int  found = 0;

    print_banner();
    printf("\n   UPDATE STUDENT INFORMATION\n\n");
    print_line();

    printf("   Enter Registration Number to update: ");
    read_line(search_reg, sizeof(search_reg));
    str_upper(search_reg);

    for (int i = 0; i < record_count; i++) {
        if (strcmp(records[i].reg_number, search_reg) == 0) {
            found = 1;
            Student *s = &records[i];
            char choice[4];
            int editing = 1;

            // Loop continues until user explicitly chooses 0 to exit
            while (editing) {
                print_banner();
                printf("\n   UPDATING RECORD: %s\n\n", s->name);
                print_student(s);
                
                printf("\n   What would you like to update?\n");
                printf("   1. Name\n   2. Course\n   3. Year of Study\n");
                printf("   4. Email\n   5. Phone Number\n   6. GPA\n");
                printf("   0. Return to Main Menu\n\n   Choice: ");
                read_line(choice, sizeof(choice));

                if (strcmp(choice, "1") == 0) {
                    get_non_empty("   New Name     : ", s->name, sizeof(s->name));
                    print_ok("Name updated.");
                } else if (strcmp(choice, "2") == 0) {
                    get_non_empty("   New Course   : ", s->course, sizeof(s->course));
                    print_ok("Course updated.");
                } else if (strcmp(choice, "3") == 0) {
                    int yr = get_int_range("   New Year (1-4): ", 1, 4);
                    s->year[0] = '0' + yr;
                    s->year[1] = '\0';
                    print_ok("Year updated.");
                } else if (strcmp(choice, "4") == 0) {
                    char new_email[MAX_EMAIL];
                    while (1) {
                        get_non_empty("   New Email    : ", new_email, sizeof(new_email));
                        if (strchr(new_email, '@') == NULL) {
                            print_err("Must contain @");
                        } else if (is_email_taken(new_email, i)) {
                            print_err("Email already in use by another student.");
                        } else {
                            strcpy(s->email, new_email);
                            print_ok("Email updated.");
                            break;
                        }
                    }
                } else if (strcmp(choice, "5") == 0) {
                    char new_phone[MAX_PHONE];
                    while (1) {
                        get_non_empty("   New Phone    : ", new_phone, sizeof(new_phone));
                        if (is_phone_taken(new_phone, i)) {
                            print_err("Phone number already in use by another student.");
                        } else {
                            strcpy(s->phone, new_phone);
                            print_ok("Phone updated.");
                            break;
                        }
                    }
                } else if (strcmp(choice, "6") == 0) {
                    s->gpa = get_float("   New GPA (0.0-4.0): ", 0.0f, 4.0f);
                    print_ok("GPA updated.");
                } else if (strcmp(choice, "0") == 0) {
                    editing = 0;
                } else {
                    print_err("Invalid choice.");
                }
                
                if(editing) pause_screen();
            }
            break; 
        }
    }

    if (!found) {
        print_err("No student found with that registration number.");
        pause_screen();
    }
}

void delete_student(void) {
    char search_reg[MAX_REG];
    int  found = 0;

    print_banner();
    printf("\n   DELETE STUDENT RECORD\n\n");
    print_line();

    printf("   Enter Registration Number to delete: ");
    read_line(search_reg, sizeof(search_reg));
    str_upper(search_reg);

    for (int i = 0; i < record_count; i++) {
        if (strcmp(records[i].reg_number, search_reg) == 0) {
            found = 1;
            char confirm[4];
            printf("\n   Record to delete:\n");
            print_student(&records[i]);
            printf("   Are you sure you want to delete this record? (y/n): ");
            read_line(confirm, sizeof(confirm));

            if (confirm[0] == 'y' || confirm[0] == 'Y') {
                for (int j = i; j < record_count - 1; j++) {
                    records[j] = records[j + 1];
                }
                record_count--;
                print_ok("Student record deleted successfully.");
            } else {
                printf("\n   Deletion cancelled.\n");
            }
            break;
        }
    }
    if (!found) print_err("No student found with that registration number.");
    pause_screen();
}

void save_to_file(void) {
    char filename[MAX_FILENAME];
    print_banner();
    printf("\n   SAVE RECORDS TO FILE\n\n");
    print_line();

    if (record_count == 0) {
        print_err("No records to save.");
        pause_screen();
        return;
    }

    printf("   Filename [%s]: ", DEFAULT_FILE);
    read_line(filename, sizeof(filename));
    if (strlen(filename) == 0) strcpy(filename, DEFAULT_FILE);

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        print_err("Could not open file for writing.");
        pause_screen();
        return;
    }

    fprintf(fp, "%d\n", record_count);
    for (int i = 0; i < record_count; i++) {
        Student *s = &records[i];
        fprintf(fp, "%d\n%s\n%s\n%s\n%s\n%s\n%s\n%.2f\n", 
                s->id, s->reg_number, s->name, s->course, s->year, s->email, s->phone, s->gpa);
    }
    fclose(fp);
    printf("\n   [OK]  %d record(s) saved to '%s'.\n", record_count, filename);
    pause_screen();
}

void load_from_file(void) {
    char filename[MAX_FILENAME];
    int count;
    print_banner();
    printf("\n   LOAD RECORDS FROM FILE\n\n");
    print_line();

    printf("   Filename [%s]: ", DEFAULT_FILE);
    read_line(filename, sizeof(filename));
    if (strlen(filename) == 0) strcpy(filename, DEFAULT_FILE);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        print_err("File not found.");
        pause_screen();
        return;
    }

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

    if (fscanf(fp, "%d\n", &count) != 1) {
        print_err("File format is invalid.");
        fclose(fp);
        pause_screen();
        return;
    }

    if (count > MAX_STUDENTS) count = MAX_STUDENTS;
    record_count = 0;
    next_id = 1;

    for (int i = 0; i < count; i++) {
        Student *s = &records[i];
        char line[100];

        if (fscanf(fp, "%d\n", &s->id) != 1) break;
        if (!fgets(s->reg_number, sizeof(s->reg_number), fp)) break; s->reg_number[strcspn(s->reg_number, "\n")] = '\0';
        if (!fgets(s->name, sizeof(s->name), fp)) break;             s->name[strcspn(s->name, "\n")] = '\0';
        if (!fgets(s->course, sizeof(s->course), fp)) break;         s->course[strcspn(s->course, "\n")] = '\0';
        if (!fgets(s->year, sizeof(s->year), fp)) break;             s->year[strcspn(s->year, "\n")] = '\0';
        if (!fgets(s->email, sizeof(s->email), fp)) break;           s->email[strcspn(s->email, "\n")] = '\0';
        if (!fgets(s->phone, sizeof(s->phone), fp)) break;           s->phone[strcspn(s->phone, "\n")] = '\0';
        if (!fgets(line, sizeof(line), fp)) break;                   s->gpa = (float)atof(line);

        record_count++;
        if (s->id >= next_id) next_id = s->id + 1;
    }
    fclose(fp);
    printf("\n   [OK]  %d record(s) loaded from '%s'.\n", record_count, filename);
    pause_screen();
}

int main(void) {
    char choice[4];
    while (1) {
        print_menu();
        read_line(choice, sizeof(choice));

        if      (strcmp(choice, "1") == 0) add_student();
        else if (strcmp(choice, "2") == 0) display_all();
        else if (strcmp(choice, "3") == 0) search_student();
        else if (strcmp(choice, "4") == 0) update_student();
        else if (strcmp(choice, "5") == 0) delete_student();
        else if (strcmp(choice, "6") == 0) save_to_file();
        else if (strcmp(choice, "7") == 0) load_from_file();
        else if (strcmp(choice, "0") == 0) {
            print_banner();
            printf("\n   Thank you for using the System. Goodbye!\n\n");
            print_double_line();
            break;
        } else {
            print_err("Invalid option. Please enter a number from 0 to 7.");
            pause_screen();
        }
    }
    return 0;
}
