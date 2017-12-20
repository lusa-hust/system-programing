/*
* @file   testebbchar.c
* @author Derek Molloy
* @date   7 April 2015
* @version 0.1
* @brief  A Linux user space program that communicates with the ebbchar.c LKM. It passes a
* string to the LKM and reads the response from the LKM. For this example to work the device
* must be called /dev/ebbchar.
* @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM
enum mode_write_e {
    PUSH = 0, GET = 1, EDIT = 2, DELETE = 3
};

int main() {
    int ret, fd, n, result, action;
    char key[50], value[50];
    char buffer[110];
    char stringToSend[BUFFER_LENGTH];

    printf("Starting device test code example...\n");

    // main loop
    while (1) {
        printf("\n\n");
        printf("+==================================================+\n");
        printf("| 1. Push a key-value                              |\n");
        printf("| 2. Read a key                                    |\n");
        printf("| 3. Edit a key-value                              |\n");
        printf("| 4. Delete key-value                              |\n");
        printf("| 5. Exit                                          |\n");
        printf("+==================================================+\n");
        printf("Choose an action: ");
        scanf("%d%*c", &action);
        switch (action) {
            case 1: {
                // Push
                fd = open("/dev/ictredis", O_RDWR);             // Open the device with read/write access
                if (fd < 0) {
                    perror("Failed to open the device...");
                    close(fd);
                    break;
                }

                printf("Enter key: ");
                scanf ("%[^\n]%*c", key);
                printf("Enter value: ");
                scanf ("%[^\n]%*c", value);
                // compose the message to send to the device
                snprintf(buffer, 110, "%d|%s|%s", PUSH, key, value);
                ret = write(fd, buffer, strlen(buffer));
                if (ret < 0) {
                    perror("Failed to write the message to the device.");
                    close(fd);
                    break;
                }

                if(ret > 0) {
                    printf("Push key: %s | value: %s successfully\n", key, value);
                } else {
                    printf("key exist ! Fail to push key %s | value: %s ", key, value);
                }


                close(fd);
            }
                break;


            case 2: {
                // read
                fd = open("/dev/ictredis", O_RDWR);             // Open the device with read/write access
                if (fd < 0) {
                    perror("Failed to open the device...");
                    break;
                }


                printf("Enter the key you want to read: ");
                scanf ("%[^\n]%*c", key);
                snprintf(buffer, 110, "%d|%s", GET, key);
                ret = write(fd, buffer, strlen(buffer));
                if (ret < 0) {
                    perror("Failed to write the message to the device.");
                    close(fd);
                    break;
                }
                // now read
                if (ret == 0) {
                    printf("Key %s not found\n", key);
                } else {
                    int rret = read(fd, value, 50);
                    if (rret <= 0) {
                        printf("Key %s not found\n", key);
                    } else {
                        printf("Key %s found with value: %s\n", key, value);
                    }
                }

                close(fd);
            }
                break;

            case 3: {
                // edit
                fd = open("/dev/ictredis", O_RDWR);             // Open the device with read/write access
                if (fd < 0) {
                    perror("Failed to open the device...");
                    close(fd);
                    break;
                }

                printf("Enter key need to edit: ");
                scanf ("%[^\n]%*c", key);
                printf("Enter value: ");
                scanf ("%[^\n]%*c", value);
                // compose the message to send to the device
                snprintf(buffer, 110, "%d|%s|%s", EDIT, key, value);
                ret = write(fd, buffer, strlen(buffer));
                if (ret < 0) {
                    perror("Failed to write the message to the device.");
                    close(fd);
                    break;
                }

                if(ret > 0) {
                    printf("Edit key: %s | value: %s successfully\n", key, value);
                } else {
                    printf("key not found ! Fail to edit key %s | value: %s ", key, value);
                }

                close(fd);
            }

                break;
            case 4: {
                // delete
                fd = open("/dev/ictredis", O_RDWR);             // Open the device with read/write access
                if (fd < 0) {
                    perror("Failed to open the device...");
                    break;
                }


                printf("Enter the key you want to Delete: ");
                scanf ("%[^\n]%*c", key);
                snprintf(buffer, 110, "%d|%s", DELETE, key);
                ret = write(fd, buffer, strlen(buffer));
                if (ret < 0) {
                    perror("Failed to write the message to the device.");
                    close(fd);
                    break;
                }

                if(ret > 0) {
                    printf("Delete key: %s successfully\n", key);
                } else {
                    printf("key not found ! Fail to delete key %s ", key);
                }

                close(fd);
            }

                break;

            case 5: {
                printf("Exit\n");
                exit(1);
            }

            default:
                printf("Invalid action\n");
        }
    }


}