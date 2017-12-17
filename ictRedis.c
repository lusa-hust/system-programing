#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kern_levels.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <asm-generic/current.h>


#include <stdlib.h>

#define  DEVICE_NAME "ictredis"
#define  CLASS_NAME  "ict"
#define MAX_ELEMENT 50

enum mode_write_e {
    PUSH = 0, GET = 1
};

typedef enum mode_write_e ModeWrite;

struct element_t {
    char key[50];
    char value[50];
};


typedef struct element_t Element;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lusa");
MODULE_DESCRIPTION("A simple redis implement using char device");
MODULE_VERSION("0.1");


static int majorNumber;                  ///< Stores the device number -- determined automatically

static Element array[MAX_ELEMENT];
static int max_index;
static char requestKey[100];

static int numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class *ictredisClass = NULL; ///< The device-driver class struct pointer
static struct device *ictredisDevice = NULL; ///< The device-driver device struct pointer
static ModeWrite modeWrite;

// The prototype functions for the character driver -- must come before the struct definition
static int dev_open(struct inode *, struct file *);

static int dev_release(struct inode *, struct file *);

static ssize_t dev_read(struct file *, char *, size_t, loff_t *);

static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static Element create_elemet(char *string);


/// A macro that is used to declare a new mutex that is visible in this file
/// results in a semaphore variable ICTRedis_mutex with value 1 (unlocked)
/// DEFINE_MUTEX_LOCKED() results in a variable with value 0 (locked)
static DEFINE_MUTEX(ICTRedis_mutex);


/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
        {
                .open = dev_open,
                .read = dev_read,
                .write = dev_write,
                .release = dev_release,
        };


/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init

ICTRedis_init(void) {
    mutex_init(&ICTRedis_mutex);       /// Initialize the mutex lock dynamically at runtime
    printk(KERN_INFO "ICTRedis: Initializing the ICTRedis LKM\n");

    // Try to dynamically allocate a major number for the device -- more difficult but worth it
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "ICTRedis failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "ICTRedis: registered correctly with major number %d\n", majorNumber);

    // Register the device class
    ictredisClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(ictredisClass)) {                // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(ictredisClass);          // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "ICTRedis: device class registered correctly\n");

    // Register the device driver
    ictredisDevice = device_create(ictredisClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(ictredisDevice)) {               // Clean up if there is an error
        class_destroy(ictredisClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(ictredisDevice);
    }
    max_index = -1;
    printk(KERN_INFO "ICTRedis: device class created correctly\n"); // Made it! device was initialized
    return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit

ICTRedis_exit(void) {
    mutex_destroy(&ICTRedis_mutex);        /// destroy the dynamically-allocated mutex
    device_destroy(ictredisClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(ictredisClass);                          // unregister the device class
    class_destroy(ictredisClass);                             // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
    printk(KERN_INFO "ICTRedis: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep) {
    if (!mutex_trylock(&ICTRedis_mutex)) {    /// Try to acquire the mutex (i.e., put the lock on/down)
        /// returns 1 if successful and 0 if there is contention
        printk(KERN_ALERT "ICTRedis: Device in use by another process");
        return -EBUSY;
    }
    numberOpens++;
    printk(KERN_INFO "ICTRedis: Device has been opened %d time(s)\n", numberOpens);
    printk(KERN_INFO "Device is opened by process id %d\n", get_current()->pid);
    return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;


    if (max_index < 0 || modeWrite != GET) {
        // copy_to_user has the format ( * to, *from, size) and returns 0 on success
        int number = 0;
        error_count = copy_to_user((int *) buffer, &number, sizeof(int));
        if(modeWrite == GET) {
            printk(KERN_ALERT "ICTRedis: request key : %s not found \n", requestKey);
        } else {
            printk(KERN_ALERT "ICTRedis: need request key first\n");
        }

        return 0;  // clear the position to the start and return 0
    }

    int index;

    for (index = 0; index <= max_index; index++) {
        if (strcmp(array[index].key, requestKey) == 0)

            error_count = copy_to_user((int *) buffer, array[index].value, strlen(array[index].value));

        if (error_count == 0) {            // if true then have success
            printk(KERN_INFO "ICTRedis: request key : %s found with value \n", array[index].key, array[index].value);
            return 1;  // clear the position to the start and return 0
        } else {
            printk(KERN_ALERT "ICTRedis: Failed to send %d characters to the user\n", error_count);
            return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
        }
    }


    printk(KERN_ALERT "ICTRedis: request key : %s not found \n", requestKey);
    return 0;  // clear the position to the start and return 0
}


/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    // sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
    // size_of_message = strlen(message);                 // store the length of the stored message

    char *pt;
    pt = strtok(buffer, "|");
    int count = 0;
    while (pt != NULL) {
        if (count == 0) {
            modeWrite = (ModeWrite) atoi(pt);
            count++;
        } else {
            if (modeWrite == PUSH) {
                Element e = create_elemet(pt);
                array[max_index + 1] = e;
                max_index++;
                printk(KERN_INFO "ICTRedis: Received key: %s |value: %s from the user\n", e.key, e.value);
                return len;
            } else if (modeWrite == GET) {
                strcpy(requestKey, pt);
                printk(KERN_INFO "ICTRedis: Received request key: %s from the user\n", requestKey);
                return len;
            }
        }
        pt = strtok(NULL, ",");
    }

    printk(KERN_ALERT "ICTRedis: Write error \n", requestKey);
    return 0;  // clear the position to the start and return 0
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep) {
    mutex_unlock(&ICTRedis_mutex);          /// Releases the mutex (i.e., the lock goes up)
    printk(KERN_INFO "ICTRedis: Device successfully closed\n");
    return 0;
}


// take string "key|value" to create element
static Element create_elemet(char *string) {
    Element ret;
    char *pt;
    pt = strtok(string, "|");
    int count = 0;
    while (pt != NULL) {
        if (count == 0) {
            strcpy(ret.key, pt);
            count++;
        } else {
            strcpy(ret.value, pt);
        }
        pt = strtok(NULL, ",");
    }
    return ret;
}


/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(ICTRedis_init);
module_exit(ICTRedis_exit);
