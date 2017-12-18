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
    PUSH = 0, GET = 1
};

int main(){
    int ret, fd, n, result, action;
    char key[50], value[50];
    char buffer[110];
    char stringToSend[BUFFER_LENGTH];

    printf("Starting device test code example...\n");
    fd = open("/dev/ictredis", O_RDWR);             // Open the device with read/write access
    if (fd < 0){
        perror("Failed to open the device...");
        return errno;
    }
    printf("Opened ictRedis device successfully\n");

    // main loop
    while(1) {
        printf("\n\n");
        printf("+==================================================+\n");
        printf("| 1. Write a key-value                             |\n");
        printf("| 2. Read a key                                    |\n");
        printf("| 3. Exit                                          |\n");
        printf("+==================================================+\n");
        printf("Choose an action: ");
        scanf("%d", &action);

        switch(action) {
            case 1:
                printf("Enter key: ");
                scanf("%s", key);
                printf("Enter value: ");
                scanf("%s", value);
                // compose the message to send to the device
                snprintf(buffer, 110, "%d|%s|%s", PUSH, key, value);
                ret = write(fd, buffer, strlen(buffer));
                if (ret < 0){
                    perror("Failed to write the message to the device.");
                    return errno;
                }
                printf("Written successfully\n");
            break;

            case 2:
                printf("Enter the key you want to read: ");
                scanf("%s", key);
                snprintf(buffer, 110, "%d|%s", GET, key);
                ret = write(fd, buffer, strlen(buffer));
                if (ret < 0){
                    perror("Failed to write the message to the device.");
                    return errno;
                }
                // now read
                read(fd, value, 50);
                printf("Read: %s\n", value);
            break;

            case 3:
                printf("Exit\n");
                close(fd);
                exit(1);
            break;

            default:
                printf("Invalid action\n");
        }
    }
    
    printf("Type in a short string to send to the kernel module:\n");
    scanf("%d", &n);                // Read in a string (with spaces)
    printf("Writing number to the device [%d].\n", n);
    ret = write(fd, (char *) &n, sizeof(n)); // Send the string to the LKM
    if (ret < 0){
        perror("Failed to write the message to the device.");
        return errno;
    }

    printf("Press ENTER to read back from the device...\n");
    getchar();

    printf("Reading from the device...\n");
    ret = read(fd,(char *) &result, sizeof(int));        // Read the response from the LKM
    if (ret < 0){
        perror("Failed to read the message from the device.");
        return errno;
    }
    printf("The received message is: [%d]\n", result);
    printf("End of the program\n");
    return 0;
}