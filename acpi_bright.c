#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

void print_help() {
    printf(
        "acpi_bright <options> \
         Program options:\n \
            -h                      Print this help message\n \
            -d <ACPI Device Name>   Modify brightness for specific device\n \
            -a                      Modify brightness for all devices\n \
            -s <brightness>         Set brightness level \
            -c <brightness delta>   Change brightness by delta");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Must provide value to alter brightness.\n");
        return 1;
    }

    // Argument booleans and data
    int help = 0;               // -h
    int all_selected = 0;       // -a
    int set_value = 0;          // 0 if no arg, 1 for s, 2 for c
    int level = 0;              // parsed level to set or change 
    char *device = NULL;        // -d 

    char *endptr;               // strtol

    long opt;                   // getopt

    while ((opt = getopt(argc, argv, "had:s:c:"))) {
        switch (opt) {
            case 'h':
                help = 1;
                break;
            case 'a':
                all_selected = 1;
                break;
            case 'd':
                device = strdup(optarg);
                break;
            case 's':

                if (set_value != 0) {
                    fprintf(stderr, "Cannot use -s and -c! acpi_bright -h for more info.\n");
                    return 1;
                }

                set_value = 1;
                
                level = strtol(optarg, &endptr, 10);

                if (endptr == optarg || *endptr != ' ') {
                    fprintf(stderr, "Could not parse %c, as int.\n", *endptr);
                    return 1;
                }

                break;
            case 'c':

                if (set_value != 0) {
                    fprintf(stderr, "Cannot use -s and -c! acpi_bright -h for more info.\n");
                    return 1;
                }

                set_value = 2;

                level = strtol(optarg, &endptr, 10);

                if (endptr == optarg || *endptr != ' ') {
                    fprintf(stderr, "Could not parse %c, as int.\n", *endptr);
                    return 1;
                }

                break;
            default: /* bad */
                fprintf(stderr, "Bad argument provided!\n");
                return 1;
        }
    }

    if (help) {
        print_help();
        return 0;
    }

    if (set_value == 1) {
        
    } else if (set_value == 2) {

    } else { 
        fprintf(stderr, "Must set or change brightness.\n");
        return 1;
    }
}