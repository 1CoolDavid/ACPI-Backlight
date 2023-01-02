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
            -c <brightness delta>   Change brightness by delta\n \
            -p <percent>            Set brightness percentage\n");
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

void set_brightness(int new_brightness, int max_brightness, int brightness_fd) {
    // Ensure no negative or over 100% level brightness
    new_brightness = MIN(MAX(new_brightness, 0), max_brightness);

    char *brightness_buffer = calloc(10, sizeof(char));
    sprintf(brightness_buffer, "%d", new_brightness);
    write(brightness_fd, brightness_buffer, strlen(brightness_buffer));
    free(brightness_buffer);
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

    int level_change = 0;       // amount to change level by

    int percentage = -1;         // -p

    char *device = NULL;        // -d 

    char *endptr;               // strtol

    int opt;                    // getopt

    while ((opt = getopt(argc, argv, "had:s:c:p:")) != -1) {
        switch (opt) {
            case 'h':
                help = 1;
                break;
            case 'd':
                device = strdup(optarg);
                break;
            case 'p':

                if (level_buffer != NULL || level_change != 0) {
                    fprintf(stderr, "Only use 1: -s, -c, -p! acpi_bright -h for more info.\n");
                    return 1;
                }

                percentage = (int)strtol(optarg, &endptr, 10);

                if (endptr == optarg || (*endptr != ' ' && *endptr != '\0')) {
                    fprintf(stderr, "Could not parse %c, as int.\n", *endptr);
                    return 1;
                }

                if (percentage < 0 || percentage > 100) {
                    fprintf(stderr, "%d\% is not a valid brightness!\n", percentage);
                    return 1;
                }

                break;
            case 's':

                if (level_change != 0 || percentage != -1) {
                    fprintf(stderr, "Only use 1: -s, -c, -p! acpi_bright -h for more info.\n");
                    return 1;
                }

                level_length = strlen(optarg);
                level_buffer = strdup(optarg);

                break;
            case 'c':

                if (level_buffer != NULL || percentage != -1) {
                    fprintf(stderr, "Only use 1: -s, -c, -p! acpi_bright -h for more info.\n");
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
    if (percentage != -1) { // setting percentage
        int new_brightness = (percentage/100.0)*max_brightness;
        set_brightness(new_brightness, max_brightness, brightness_fd);
    } else if (level_buffer != NULL) { // setting value
        write(brightness_fd, level_buffer, level_length);
        free(level_buffer);
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
        
        set_brightness(brightness, max_brightness, brightness_fd);
    }

    close(brightness_fd);
}
