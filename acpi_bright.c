#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

static const char *ACPI_DIR = "/sys/class/backlight/";
static const int ACPI_DIR_LEN = 22;

static const char *MAX_FILE = "max_brightness";
static const int MAX_FILE_LEN = 15;

void print_help() {
    printf(
        "acpi_bright <options> \
         Program options:\n \
            -h                      Print this help message\n \
            -d <ACPI Device Name>   Modify brightness for specific device (required)\n \
            -s <brightness>         Set brightness level\n \
            -c <brightness delta>   Change brightness by delta");
}

long get_max_brightness(char *device) {
    int device_len = strlen(device);
    int path_len = device_len+ACPI_DIR_LEN+MAX_FILE_LEN+2;
    char *path = calloc(path_len, sizeof(char));
    
    snprintf(path, path_len, "%s%s/%s", ACPI_DIR, device, MAX_FILE);

    int fd = open(path, O_RDONLY);

    free(path);

    if(fd == -1) {
        fprintf(stderr, "Error occurred while opening %s. Check if file exists.\n");
        return -1L;
    } 

    char *max_str = calloc(9, sizeof(char));
    int bytes = read(fd, max_str, 9);

    if(bytes == 0 || bytes == -1) {
        fprintf(stderr, "Max brightness could not be read.\n");
        return -1L;
    }
    
    long max = strtol(max_str, NULL, 10);    
    free(max_str);

    return max;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Must provide value to alter brightness.\n");
        return 1;
    }

    // Argument booleans and data
    int help = 0;               // -h
    int set_value = 0;          // 0 if no arg, 1 for s, 2 for c
    int level = 0;              // parsed level to set or change 
    char *device = NULL;        // -d 

    char *endptr;               // strtol

    int opt;                   // getopt

    while ((opt = getopt(argc, argv, "had:s:c:")) != -1) {
        switch (opt) {
            case 'h':
                help = 1;
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

                if (endptr == optarg || (*endptr != ' ' && *endptr != '\0') ) {
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

    if (set_value == 0) {
        fprintf(stderr, "Must set or change brightness.\n");
        return 1;
    }

    if (!device) {
        fprintf(stderr, "ACPI device not specified.\n");
        return 1;
    }

    long max_brightness = get_max_brightness(device);

    if(max_brightness == -1) {
        return 1;
    } else if (max_brightness < 0) {
        fprintf(stderr, "Parse error with max brightness\n");
        return 1;
    }


    if (set_value == 1) {
        
    } else {

    }
}
