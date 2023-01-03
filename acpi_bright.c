#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

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
            -s <brightness>         Set brightness level, prepend with +/- to make change incremental (supports \%)\n \
            -r                      Print current (raw) brightness \n \
            -p                      Print current brightness percent \n");
}

int get_brightness_fd(char *path, int path_len) {
    char *brightness_path = calloc(path_len+BRIGHTNESS_FILE_LEN, sizeof(char));
    snprintf(brightness_path, path_len+BRIGHTNESS_FILE_LEN, "%s%s", path, BRIGHTNESS_FILE);
    
    int fd = open(brightness_path, O_RDWR);

    if(fd < 0) {
        fprintf(stderr, "Error occurred while opening %s. Check if file exists.\n", brightness_path);
    }

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
    int bytes = read(fd, max_str, 9);

    close(fd);

    if(bytes == 0 || bytes == -1) {
        free(max_str);
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

int get_brightness(char *path, int path_len) {
    int brightness_fd = get_brightness_fd(path, path_len);

    if(brightness_fd < 0) {
        return -1;
    }

    char *curr_str = calloc(10, sizeof(char));
    int bytes = read(brightness_fd, curr_str, 10);

    if(bytes == 0 || bytes == -1) {
        fprintf(stderr, "Current brightness could not be read.\n");
        return -1L;
    }
    
    int brightness = atoi(curr_str);    
    free(curr_str);

    return brightness;
}

int percent_to_raw(int percent, int max) {
    float one_percent = max/100.0;
    float exact_brightness = percent*one_percent;

    return (int)exact_brightness < exact_brightness ? (int)exact_brightness+1 : (int)exact_brightness;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Must provide value to alter brightness.\n");
        return 1;
    }

    // Argument booleans and data
    bool help = 0;                  // -h

    int level = 0;                  // level to set/change  
    bool is_setting = false;        // -s

    bool is_incremental = false;    // is +/- parsed 

    bool is_percentage = false;     // is % parsed

    bool get_raw = 0;               // -r
    bool get_percent = 0;           // -p

    char *device = NULL;            // -d 

    char *endptr;               // strtol

    int opt;                    // getopt

    while ((opt = getopt(argc, argv, "hprs:d:")) != -1) {
        switch (opt) {
            case 'h':
                help = 1;
                break;
            case 'd':
                device = strdup(optarg);
                break;
            case 'p':
                if(get_raw) {
                    fprintf(stderr, "Can't get raw and get percent. acpi_bright -h for more info.");
                    if(device) {free(device);}
                    return 1;
                } else if (is_setting) {
                    fprintf(stderr, "Can't set and get. acpi_bright -h for more info.\n");
                    if(device) {free(device);}
                    return 1;
                }

                get_percent = true;
                break;
            case 'r':
                if(get_percent) {
                    fprintf(stderr, "Can't get raw and get percent. acpi_bright -h for more info.");
                    if(device) {free(device);}
                    return 1;
                } else if (is_setting) {
                    fprintf(stderr, "Can't set and get. acpi_bright -h for more info.\n");
                    if(device) {free(device);}
                    return 1;
                }

                get_raw = true;
                break;
            case 's':
                is_setting = true;

                if (*optarg == '+' || *optarg == '-') {
                    is_incremental = true;
                }

                if (get_percent || get_raw) {
                    fprintf(stderr, "Can't set and get. acpi_bright -h for more info.\n");
                    if(device) {free(device);}
                    return 1;
                }

                level = (int)strtol(optarg, &endptr, 10);

                if(*endptr == '%') {
                    is_percentage = true;
                    endptr++;
                }

                if (endptr == optarg || (*endptr != ' ' && *endptr != '\0')) {
                    fprintf(stderr, "Could not parse %c, as int or percent.\n", *endptr);
                    if(device) {free(device);}
                    return 1;
                }

                break;
            default: /* bad */
                fprintf(stderr, "Bad argument provided!\n");
                if(device) {free(device);}
                print_help();
                return 1;
        }
    }

    if (help) {
        print_help();
        if(device) {free(device);}
        return 0;
    }

    if (!device) {
        fprintf(stderr, "ACPI device not specified.\n");
        return 1;
    }

    if (!get_raw && level == 0 && !get_percent) {
        fprintf(stderr, "Too few arguments supplied. acpi_bright -h for more info.\n");
        free(device);
        return 1;
    }

    int device_len = strlen(device);
    int path_len = device_len+ACPI_DIR_LEN+2;
    char *path = calloc(path_len, sizeof(char));
    
    snprintf(path, path_len, "%s%s/", ACPI_DIR, device);
    free(device);

    if(get_raw) {
        int brightness = get_brightness(path, path_len);
        if(brightness) {
            printf("%d\n", brightness);
            free(path);
            return 0;
        }
        free(path);
        return 1;
    }

    int max_brightness = get_max_brightness(path, path_len);
    
    if(max_brightness == -1) {
        free(path);
        return 1;
    } else if (max_brightness < 0) {
        free(path);
        fprintf(stderr, "Parsing error of max brightness\n");
        return 1;
    }

    if(get_percent) {
        int brightness = get_brightness(path, path_len);
        float percentage = ((brightness*100.0))/max_brightness;
        if(brightness) {
            printf("%d\%\n", (int)percentage);
            free(path);
            return 0;
        }
        free(path);
        return 1;
    }

    int brightness_fd = get_brightness_fd(path, path_len);
    free(path);

    if(brightness_fd < 0) {
        return 1;
    }

    if (!is_incremental) { // setting value
        int new_brightness = is_percentage ? percent_to_raw(level, max_brightness) : level;
        set_brightness(new_brightness, max_brightness, brightness_fd);
    } else { // changing value
        char *curr_str = calloc(10, sizeof(char));
        int bytes = read(brightness_fd, curr_str, 10);

        if(bytes == 0 || bytes == -1) {
            fprintf(stderr, "Current brightness could not be read.\n");
            return -1L;
        }
        
        int brightness = atoi(curr_str);    
        free(curr_str);
        
        if (is_percentage) {
            // Adding 5% can lead to rounding errors so this is more accurate
            int current_percent = ((brightness*100.0))/max_brightness;
            int new_percent = MAX(MIN(current_percent + level, 100),0) ;
            int new_brightness = percent_to_raw(current_percent+level, max_brightness);
            set_brightness(new_brightness, max_brightness, brightness_fd);
        } else {
            set_brightness(brightness+level, max_brightness, brightness_fd);
        }  
    }

    close(brightness_fd);
}
