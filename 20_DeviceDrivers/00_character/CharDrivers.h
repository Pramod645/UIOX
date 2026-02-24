#define __CHARDRIVERS__H
#ifdef __CHARDRIVERS__H

#define Character Device Drivers
#ifdef Character Device Drivers
/*
Handling a character device is relatively easy, because usually sophisticated buffering
strategies are not needed and disk caches are not involved. Of course, character
devices differ in their requirements: some of them must implement a sophisticated
communication protocol to drive the hardware device, while others just have to read
a few values from a couple of I/O ports of the hardware devices. For instance, the
device driver of a multiport serial card device (a hardware device offering many serial
ports) is much more complicated than the device driver of a bus mouse.

A character device driver is described by a cdev structure, whose fields are listed in
Table 13-8.
Table 13-8. The fields of the cdev structure
Type Field Description
struct kobject kobj Embedded kobject
struct module * owner Pointer to the module implementing the driver, if any
struct file_operations * ops Pointer to the file operations table of the device driver
struct list_head list Head of the list of inodes relative to device files for this character
device
dev_t dev Initial major and minor numbers assigned to the device driver
unsigned int count Size of the range of device numbers assigned to the device driver
The cdev_add() function registers a cdev descriptor in the device driver model. The
function initializes the dev and count fields of the cdev descriptor, then invokes the
kobj_map() function. This function, in turn, sets up the device driver model’s data
structures that glue the interval of device numbers to the device driver descriptor.
The device driver model defines a kobject mapping domain for the character devices,
which is represented by a descriptor of type kobj_map and is referenced by the cdev_
map global variable. The kobj_map descriptor includes a hash table of 255 entries
indexed by the major number of the intervals. The hash table stores objects of type
probe, one for each registered range of major and minor numbers, whose fields are
listed in Table 13-9.
Table 13-9. The fields of the probe object
Type Field Description
struct probe * next Next element in hash collision list
dev_t dev Initial device number (major and minor) of the interval
unsigned long range Size of the interval
struct module * owner Pointer to the module that implements the device driver, if any
struct kobject *(*)
(dev_t, int *, void *)
get Method for probing the owner of the interval
int (*)(dev_t, void *) lock Method for increasing the reference counter of the owner of the
interval
void * data Private data for the owner of the interval
*/

#define Assigning Device Numbers
/*
To keep track of which character device numbers are currently assigned, the kernel
uses a hash table chrdevs, which contains intervals of device numbers. Two intervals
may share the same major number, but they cannot overlap, thus their minor numbers
should be all different. The table includes 255 entries, and the hash function
masks out the four higher-order bits of the major number—therefore, major numbers
less than 255 are hashed in different entries. Each entry points to the first element
of a collision list ordered by increasing major and minor numbers.
Each list element is a char_device_struct structure, whose fields are shown in
Table 13-10.

Table 13-10. The fields of the char_device_struct descriptor
Type Field Description
unsigned char_device_struct * next The pointer to next element in hash collision list
unsigned int major The major number of the interval
unsigned int baseminor The initial minor number of the interval
int minorct The interval size
const char * name The name of the device driver that handles the interval
struct file_operations * fops Not used
struct cdev * cdev Pointer to the character device driver descriptor
*/
register_chrdev( );

#define Accessing a Character Device Driver

#define Buffering Strategies for Character Devices
#endif // end of Character Device Drivers
/* ------------------------------------- */


#endif // end of __CHARDRIVERS__H