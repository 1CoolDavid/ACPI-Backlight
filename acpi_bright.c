#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

static const char *ACPI_DIR = "/sys/class/backlight/";
static const int ACPI_DIR_LEN = 22;

static const char *MAX_FILE = "max_brightness";
static const int MAX_FILE_LEN = 15;

static const char *BRIGHTNESS_FILE = "brightness";
static const int BRIGHTNESS_FILE_LEN = 11;

void print_help() {
    printf(
        "acpi_bright <options> \
         Program options:\n \
            -h                      Print this help message\n \
            -d <ACPI Device Name>   Modify brightness for specific device (required)\n \
            -s <brightness>         Set brightness level\n \
            -c <brightness delta>   Change brightness by delta");
}

int get_brightness_fd(char *path, int path_len) {
    char *brightness_path = calloc(path_len+BRIGHTNESS_FILE_LEN, sizeof(char));
    snprintf(brightness_path, path_len+BRIGHTNESS_FILE_LEN, "%s%s", path, BRIGHTNESS_FILE);
    
    int fd = open(brightness_path, O_RDWR);

    free(brightness_path);

    return fd;
}

int get_max_brightness(char *path, int path_len) {
    char *max_path = calloc(path_len+MAX_FILE_LEN, sizeof(char));
    
    snprintf(max_path, path_len+MAX_FILE_LEN, "%s%s", path, MAX_FILE);

    int fd = open(max_path, O_RDONLY);

    free(max_path);

    if(fd == -1) {
        fprintf(stderr, "Error occurred while opening %s. Check if file exists.\n", max_path);
        return -1L;
    } 

    char *max_str = calloc(10, sizeof(char));
    int bytes = read(fd, max_str, 10);

    close(fd);

    if(bytes == 0 || bytes == -1) {
        fprintf(stderr, "Max brightness could not be read.\n");
        return -1L;
    }
    
    int max = atoi(max_str);    
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

    char *level_buffer = NULL;  // level to set  
    int level_length = 0;       // parsed level str length

    int level_change = 0;      // amount to change level by

    char *device = NULL;        // -d 

    char *endptr;               // strtol

    int opt;                    // getopt

    while ((opt = getopt(argc, argv, "had:s:c:")) != -1) {
        switch (opt) {
            case 'h':
                help = 1;
                break;
            case 'd':
                device = strdup(optarg);
                break;
            case 's':

                if (level_change != 0) {
                    fprintf(stderr, "Cannot use -s and -c! acpi_bright -h for more info.\n");
                    return 1;
                }

                level_length = strlen(optarg);
                level_buffer = strdup(optarg);

                break;
            case 'c':

                if (level_buffer != NULL) {
                    fprintf(stderr, "Cannot use -s and -c! acpi_bright -h for more info.\n");
                    return 1;
                }

                level_change = (int)strtol(optarg, &endptr, 10);

                if (endptr == optarg || (*endptr != ' ' && *endptr != '\0')) {
                    fprintf(stderr, "Could not parse %c, as int.\n", *endptr);
                    return 1;
                }

                if (!level_change) {
                    fprintf(stderr, "Can't change brightness by 0!\n");
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

    if (!device) {
        fprintf(stderr, "ACPI device not specified.\n");
        return 1;
    }

    int device_len = strlen(device);
    int path_len = device_len+ACPI_DIR_LEN+2;
    char *path = calloc(path_len, sizeof(char));
    
    snprintf(path, path_len, "%s%s/", ACPI_DIR, device, MAX_FILE);
    free(device);

    int max_brightness = get_max_brightness(path, path_len);

    if(max_brightness == -1) {
        free(path);
        return 1;
    } else if (max_brightness < 0) {
        free(path);
        fprintf(stderr, "Parse error with max brightness\n");
        return 1;
    }

    int brightness_fd = get_brightness_fd(path, path_len);

    free(path);

    if (level_buffer != NULL) { // setting value
        write(brightness_fd, level_buffer, level_length);
    } else { // changing value
        char *curr_str = calloc(10, sizeof(char));
        int bytes = read(brightness_fd, curr_str, 10);

        if(bytes == 0 || bytes == -1) {
            fprintf(stderr, "Current brightness could not be read.\n");
            return -1L;
        }
        
        int brightness = atoi(curr_str);    
        free(curr_str);

        brightness += level_change;
        
        // Ensure no negative or over 100% level brightness
        brightness = MIN(MAX(brightness, 0), max_brightness);

        level_buffer = calloc(10, sizeof(char));
        sprintf(level_buffer, "%d", brightness);
        write(brightness_fd, level_buffer, strlen(level_buffer));
    }
    
    free(level_buffer);
    close(brightness_fd);
}
