#define __IOARCH__H
#ifdef __IOARCH__H
/*
The Virtual File System in __VIRTUAL_FILESYSTEM__H depends on lower-level functions to carry
out each read, write, or other operation in a manner suited to each device. The __VIRTUAL_FILESYSTEM__H
included how operations are handled by different
filesystems. here look at how the kernel invokes the operations on
actual devices.
Typically, the same computer includes several buses of different types, linked
*/

#define I/O Architecture
#ifdef I/O Architecture
/*
To make a computer work properly, data paths must be provided that let information
flow between CPU(s), RAM, and the score of I/O devices that can be connected
to a personal computer. These data paths, which are denoted as the buses, act as the
primary communication channels inside the computer.
together by hardware devices called bridges. Two high-speed buses are dedicated to
the data transfers to and from the memory chips: the frontside bus connects the CPUs
to the RAM controller, while the backside bus connects the CPUs directly to the
external hardware cache. The host bridge links together the system bus and the
frontside bus.
The data path that connects a CPU to an I/O device is generically called an I/O bus.

The I/O bus, in turn, is connected to
each I/O device by means of a hierarchy of hardware components including up to
three elements: I/O ports, interfaces, and device controllers
*/

#define I/O Ports
#ifdef I/O Ports
/*
Each device connected to the I/O bus has its own set of I/O addresses, which are
usually called I/O ports.
two consecutive 16-
bit ports may be regarded as a single 32-bit port, which must start on an address that
is a multiple of 4.
Four special assembly language instructions called in, ins, out, and
outs allow the CPU to read from and write into an I/O port. While executing one of
these instructions, the CPU selects the required I/O port and transfers the data
between a CPU register and the port.
I/O ports may also be mapped into addresses of the physical address space. The processor
is then able to communicate with an I/O device by issuing assembly language
instructions that operate directly on memory (for instance, mov, and, or, and so on).
Modern hardware devices are more suited to mapped I/O, because it is faster and
can be combined with DMA.

An important objective for system designers is to offer a unified approach to I/O programming
without sacrificing performance. Toward that end, the I/O ports of each
device are structured into a set of specialized registers called Control register, Status register, Input register and Output register.

CPU writes the commands to be sent to the device into the device control register and
reads a value that represents the internal state of the device from the device status
register. The CPU also fetches data from the device by reading bytes from the device
input register and pushes data to the device by writing bytes into the device output
register.
*/

#define Accessing I/O ports
/*
inb(), inw(), inl()
Read 1, 2, or 4 consecutive bytes, respectively, from an I/O port. The suffix “b,”
“w,” or “l” refers, respectively, to a byte (8 bits), a word (16 bits), and a long (32
bits).
inb_p(), inw_p(), inl_p()
Read 1, 2, or 4 consecutive bytes, respectively, from an I/O port, and then execute
a “dummy” instruction to introduce a pause.
outb(), outw(), outl()
Write 1, 2, or 4 consecutive bytes, respectively, to an I/O port.
outb_p(), outw_p(), outl_p()
Write 1, 2, and 4 consecutive bytes, respectively, to an I/O port, and then execute
a “dummy” instruction to introduce a pause.
insb(), insw(), insl()
Read sequences of consecutive bytes in groups of 1, 2, or 4, respectively, from an
I/O port. The length of the sequence is specified as a parameter of the functions.
outsb(), outsw(), outsl()
Write sequences of consecutive bytes, in groups of 1, 2, or 4, respectively, to an
I/O port.
While accessing I/O ports is simple, detecting which I/O ports have been assigned to
I/O devices may not be easy, in particular, for systems based on an ISA bus.

A resource represents a portion of some entity that can be exclusively assigned to a
device driver. In our case, a resource represents a range of I/O port addresses. The
information relative to each resource is stored in a resource data structure, whose
fields are shown in Table 13-1. All resources of the same kind are inserted in a treelike
data structure; for instance, all resources representing I/O port address ranges
are included in a tree rooted at the node ioport_resource.

Table 13-1. The fields of the resource data structure
Type Field Description
const char * name Description of owner of the resource
unsigned long start Start of the resource range
unsigned long end End of the resource range
unsigned long flags Various flags
struct resource * parent Pointer to parent in the resource tree
struct resource * sibling Pointer to a sibling in the resource tree
struct resource * child Pointer to first child in the resource tree

Each device driver may use the following three functions, passing to them the root
node of the resource tree and the address of a resource data structure of interest:
request_resource()
Assigns a given range to an I/O device.
allocate_resource( )
Finds an available range having a given size and alignment in the resource tree; if
it exists, assigns the range to an I/O device (mainly used by drivers of PCI
devices, which can be configured to use arbitrary port numbers and on-board
memory addresses).
release_resource( )
Releases a given range previously assigned to an I/O device.
*/


#endif //end of I/O Ports



#define I/O Interfaces
/*
An I/O interface is a hardware circuit inserted between a group of I/O ports and the
corresponding device controller. It acts as an interpreter that translates the values in
the I/O ports into commands and data for the device.

In the opposite direction, it
detects changes in the device state and correspondingly updates the I/O port that
plays the role of status register. This circuit can also be connected through an IRQ
line to a Programmable Interrupt Controller, so that it issues interrupt requests on
behalf of the device.

//There are two types of interfaces:
//Custom I/O interfaces
Devoted to one specific hardware device. In some cases, the device controller is
located in the same card * that contains the I/O interface. The devices attached to
a custom I/O interface can be either internal devices (devices located inside the
PC’s cabinet) or external devices (devices located outside the PC’s cabinet).
//General-purpose I/O interfaces
Used to connect several different hardware devices. Devices attached to a general-
purpose I/O interface are usually external devices.

//Custom I/O interfaces: most commonly found:
Keyboard interface
Graphic interface
Disk interface
Bus mouse interface
Network interface


//General-purpose I/O interfaces: The most common interfaces are:
Parallel port
Serial port
PCMCIA interface
SCSI (Small Computer System Interface) interface
Universal serial bus (USB)
*/


#define Device Controllers
/*
A complex device may require a device controller to drive it. Essentially, the controller
plays two important roles:
• It interprets the high-level commands received from the I/O interface and forces
the device to execute specific actions by sending proper sequences of electrical
signals to it.
• It converts and properly interprets the electrical signals received from the device
and modifies (through the I/O interface) the value of the status register.

Simpler devices do not have a device controller.
Several hardware devices include their own memory, which is often called I/O shared
memory. For instance, all recent graphic cards include tens of megabytes of RAM in
the frame buffer, which is used to store the screen image to be displayed on the
monitor.

*/

#endif // end of I/O Architecture
/* ------------------------------------- */

#define The Device Driver Model
#ifdef The Device Driver Model
/*
Earlier versions of the Linux kernel offered few basic functionalities to the device
driver developers: allocating dynamic memory, reserving a range of I/O addresses or
an IRQ line, activating an interrupt service routine in response to a device’s interrupt.

Things are different now. Bus types such as PCI put strong demands on the internal
design of the hardware devices; as a consequence, recent hardware devices, even of
different classes, support similar functionalities. Drivers for such devices should typically
take care of:
• Power management (handling of different voltage levels on the device’s power
line)
• Plug and play (transparent allocation of resources when configuring the device)
• Hot-plugging (support for insertion and removal of the device while the system
is running)

Power management is performed globally by the kernel on every hardware device in
the system.
For instance, when a battery-powered computer enters the “standby”
state, the kernel must force every hardware device (hard disks, graphics card, sound
card, network card, bus controllers, and so on) in a low-power state.

Thus, each
driver of a device that can be put in the “standby” state must include a callback function
that puts the hardware device in the low-power state. Moreover, the hardware
devices must be put in the “standby” state in a precise order, otherwise some devices
could be left in the wrong power state.

To implement these kinds of operations, Linux 2.6 provides some data structures
and helper functions that offer a unifying view of all buses, devices, and device drivers
in the system; this framework is called the device driver model.
*/

#define The sysfs Filesystem
/*
A goal of the sysfs filesystem is to expose the hierarchical relationships among the
components of the device driver model. The related top-level directories of this filesystem
are:
block
The block devices, independently from the bus to which they are connected.
devices
All hardware devices recognized by the kernel, organized according to the bus in
which they are connected.
bus
The buses in the system, which host the devices.
drivers
The device drivers registered in the kernel.
class
The types of devices in the system (audio cards, network cards, graphics cards,
and so on); the same class may include devices hosted by different buses and
driven by different drivers.
power
Files to handle the power states of some hardware devices.
firmware
Files to handle the firmware of some hardware devices.

Relationships between components of the device driver models are expressed in the
sysfs filesystem as symbolic links between directories and files.
The main role of regular files in the sysfs filesystem is to represent attributes of drivers
and devices.
*/

#define Kobjects
/*
The core data structure of the device driver model is a generic data structure named
kobject, which is inherently tied to the sysfs filesystem: each kobject corresponds to a
directory in that filesystem.

Embedding a kobject inside a container allows the kernel to:
• Keep a reference counter for the container
• Maintain hierarchical lists or sets of containers (for instance, a sysfs directory
associated with a block device includes a different subdirectory for each disk
partition)
• Provide a User Mode view for the attributes of the container

Kobjects, ksets, and subsystems
A kobject is represented by a kobject data structure, whose fields are listed in
Table 13-2.

Table 13-2. The fields of the kobject data structure
Type Field Description
char * k_name Pointer to a string holding the name of the container
char [] name String holding the name of the container, if it fits in 20 bytes
struct k_ref kref The reference counter for the container
struct list_head entry Pointers for the list in which the kobject is inserted
struct kobject * parent Pointer to the parent kobject, if any
struct kset * kset Pointer to the containing kset
struct kobj_type * ktype Pointer to the kobject type descriptor
struct dentry * dentry Pointer to the dentry of the sysfs file associated with the kobject

The kobjects can be organized in a hierarchical tree by means of ksets. A kset is a collection
of kobjects of the same type—that is, included in the same type of container.
The fields of the kset data structure are listed in Table 13-3.

Table 13-3. The fields of the kset data structure
Type Field Description
struct subsystem * subsys Pointer to the subsystem descriptor
struct kobj_type * ktype Pointer to the kobject type descriptor of the kset
struct list_head list Head of the list of kobjects included in the kset
struct kobject kobj Embedded kobject (see text)
struct kset_hotplug_ops * hotplug_ops Pointer to a table of callback functions for kobject filtering and
hot-plugging

Collections of ksets called subsystems also exist. A subsystem may include ksets of
different types, and it is represented by a subsystem data structure having just two
fields:
kset
An embedded kset that stores the ksets included in the subsystem
rwsem
A read-write semaphore that protects all ksets and kobjects recursively included
in the subsystem

//Components of the Device Driver Model
The device driver model is built upon a handful of basic data structures, which represent
buses, devices, device drivers, etc. Let us examine them.

//Devices
Each device in the device driver model is represented by a device object, whose fields
are shown in Table 13-4.

Table 13-4. The fields of the device object
Type Field Description
struct list_head node Pointers for the list of sibling devices
struct list_head bus_list Pointers for the list of devices on the same bus
type
struct list_head driver_list Pointers for the driver’s list of devices
struct list_head children Head of the list of children devices
struct device * parent Pointer to the parent device
struct kobject kobj Embedded kobject
char [] bus_id Device position on the hosting bus
struct bus_type * bus Pointer to the hosting bus
struct device_driver
*
driver Pointer to the controlling device driver
void * driver_data Pointer to private data for the driver
void * platform_data Pointer to private data for legacy device drivers
struct dev_pm_info power Power management information
unsigned long detach_state Power state to be entered when unloading the
device driver
unsigned long long * dma_mask Pointer to the DMA mask of the device (see the
later section “Direct Memory Access (DMA)”)
unsigned long long coherent_dma_mask Mask for coherent DMA of the device
struct list_head dma_pools Head of a list of aggregate DMA buffers
struct
dma_coherent_mem *
dma_mem Pointer to a descriptor of the coherent DMA
memory used by the device (see the later section
“Direct Memory Access (DMA)”)
void (*)(struct
device *)
release Callback function for releasing the device
descriptor

//Drivers
Each driver in the device driver model is described by a device_driver object, whose
fields are listed in Table 13-5.

Table 13-5. The fields of the device_driver object
Type Field Description
char * name Name of the device driver
struct bus_type * bus Pointer to descriptor of the bus that hosts the supported
devices
struct semaphore unload_sem Semaphore to forbid device driver unloading; it is
released when the reference counter reaches zero
struct kobject kobj Embedded kobject
struct list_head devices Head of the list including all devices supported by
the driver
struct module * owner Identifies the module that implements the device
driver, if any (see Appendix B)
int (*)(struct device *) probe Method for probing a device (checking that it can be
handled by the device driver)
int (*)(struct device *) remove Method invoked on a device when it is removed
void (*)(struct device *) shutdown Method invoked on a device when it is powered off
(shut down)
int (*)(struct device *,
unsigned long, unsigned long)
suspend Method invoked on a device when it is put in lowpower
state
int (*)(struct device *,
unsigned long)
resume Method invoked on a device when it is put back in
the normal state (full power)

The device_driver object includes four methods for handling hot-plugging, plug and
play, and power management.

//Buses
Each bus type supported by the kernel is described by a bus_type object, whose fields
are listed in Table 13-6.

Table 13-6. The fields of the bus_type object
Type Field Description
char * name Name of the bus type
struct subsystem subsys Kobject subsystem associated with this bus type
struct kset drivers The set of kobjects of the drivers
struct kset devices The set of kobjects of the devices
struct bus_attribute * bus_attrs Pointer to the object including the bus attributes
and the methods for exporting them to the sysfs
filesystem
struct device_attribute * dev_attrs Pointer to the object including the device attributes
and the methods for exporting them to the sysfs
filesystem
struct driver_attribute * drv_attrs Pointer to the object including the device driver
attributes and the methods for exporting them to
the sysfs filesystem
int (*)(struct device *, struct
device_driver *)
match Method for checking whether a given driver supports
a given device
int (*)(struct device *, char **,
int, char *, int)
hotplug Method invoked when a device is being registered
int (*)(struct device *,
unsigned long)
suspend Method for saving the hardware context state and
changing the power level of a device
int (*)(struct device *) resume Method for changing the power level and restoring
the hardware context of a device

//Classes
Each class is described by a class object. All class objects belong to the class_subsys
subsystem associated with the /sys/class directory. Each class object, moreover,
includes an embedded subsystem

Each class object includes a list of class_device descriptors, each of which represents
a single logical device belonging to the class. The class_device structure
includes a dev field that points to a device descriptor, thus a logical device always
refers to a given device in the device driver model. However, there can be several
class_device descriptors that refer to the same device. In fact, a hardware device
might include several different sub-devices, each of which requires a different User
Mode interface. For example, the sound card is a hardware device that usually
includes a DSP, a mixer, a game port interface, and so on; each sub-device requires
its own User Mode interface, thus it is associated with its own directory in the sysfs
filesystem.

Device drivers in the same class are expected to offer the same functionalities to the
User Mode applications; for instance, all device drivers of sound cards should offer a
way to write sound samples to the DSP.

The classes of the device driver model are essentially aimed to provide a standard
method for exporting to User Mode applications the interfaces of the logical devices.
Each class_device descriptor embeds a kobject having an attribute (special file)
named dev. Such attribute stores the major and minor numbers of the device file that
is needed to access to the corresponding logical device
*/


#endif // end of The Device Driver Model
/* ------------------------------------- */

#define Device files
#ifdef Device files
/*
Unix-like operating systems are based on the notion of a
file, which is just an information container structured as a sequence of bytes. According
to this approach, I/O devices are treated as special files called device files; thus,
the same system calls used to interact with regular files on disk can be used to directly
interact with I/O devices. For example, the same write( ) system call may be used to
write data into a regular file or to send it to a printer by writing to the /dev/lp0 device
file.

According to the characteristics of the underlying device drivers, device files can be
of two types: block or character. The difference between the two classes of hardware
devices is not so clear-cut. At least we can assume the following:
• The data of a block device can be addressed randomly, and the time needed to
transfer a data block is small and roughly the same, at least from the point of
view of the human user. Typical examples of block devices are hard disks, floppy
disks, CD-ROM drives, and DVD players.
• The data of a character device either cannot be addressed randomly (consider,
for instance, a sound card), or they can be addressed randomly, but the time
required to access a random datum largely depends on its position inside the
device (consider, for instance, a magnetic tape driver).

Traditionally, this identifier consists of the type of device file (character or block) and
a pair of numbers. The first number, called the major number, identifies the device
type. Traditionally, all device files that have the same major number and the same
type share the same set of file operations, because they are handled by the same
device driver. The second number, called the minor number, identifies a specific
device among a group of devices that share the same major number. For instance, a
group of disks managed by the same disk controller have the same major number
and different minor numbers.
The mknod( ) system call is used to create device files. It receives the name of the
device file, its type, and the major and minor numbers as its parameters. Device files
are usually included in the /dev directory. Table 13-7 illustrates the attributes of
some device files. Notice that character and block devices have independent numbering,
so block device (3,0) is different from character device (3,0).

Table 13-7. Examples of device files
Name Type Major Minor Description
/dev/fd0 block 2 0 Floppy disk
/dev/hda block 3 0 First IDE disk
/dev/hda2 block 3 2 Second primary partition of first IDE disk
/dev/hdb block 3 64 Second IDE disk
/dev/hdb3 block 3 67 Third primary partition of second IDE disk
/dev/ttyp0 char 3 0 Terminal
/dev/console char 5 1 Console
/dev/lp1 char 6 1 Parallel printer
/dev/ttyS0 char 4 64 First serial port
/dev/rtc char 10 135 Real-time clock
/dev/null char 1 3 Null device (black hole)
*/


#define User Mode Handling of Device Files
/*
the major and minor
numbers of the device files are 8 bits long. Thus, there could be at most 65,536 block
device files and 65,536 character device files. You might expect they will suffice, but
unfortunately they don’t.
The real problem is that device files are traditionally allocated once and forever in the
/dev directory; therefore, each logical device in the system should have an associated
device file with a well-defined device number. The official registry of allocated device
numbers and /dev directory nodes is stored in the Documentation/devices.txt file; the
macros corresponding to the major numbers of the devices may also be found in the
include/linux/major.h file.
*/

#define VFS Handling of Device Files
/*
Device files live in the system directory tree but are intrinsically different from regular
files and directories. When a process accesses a regular file, it is accessing some
data blocks in a disk partition through a filesystem; when a process accesses a device
file, it is just driving a hardware device. For instance, a process might access a device
file to read the room temperature from a digital thermometer connected to the computer.
It is the VFS’s responsibility to hide the differences between device files and
regular files from application programs.
To do this, the VFS changes the default file operations of a device file when it is
opened; as a result, each system call on the device file is translated to an invocation
of a device-related function instead of the corresponding function of the hosting filesystem.
The device-related function acts on the hardware device to perform the operation
requested by the process.†
*/
#endif // end of Device files
/* ------------------------------------- */

#define Device Drivers
#ifdef Device Drivers
/*
A device driver is the set of kernel routines that makes a hardware device respond to the
programming interface defined by the canonical set of VFS functions (open, read,
lseek, ioctl, and so forth) that control a device. The actual implementation of all these
functions is delegated to the device driver. Because each device has a different I/O controller,
and thus different commands and different state information, most I/O devices
have their own drivers.
There are many types of device drivers. They mainly differ in the level of support that
they offer to the User Mode applications, as well as in their buffering strategies for
the data collected from the hardware devices. Because these choices greatly influence
the internal structure of a device driver, we discuss them in the sections “Direct
Memory Access (DMA)” and “Buffering Strategies for Character Devices.”
A device driver does not consist only of the functions that implement the device file
operations. Before using a device driver, several activities must have taken place.
*/

#define Device Driver Registration

#define Device Driver Initialization

#define Monitoring I/O Operations
//Polling mode
//Interrupt mode

#define Accessing the I/O Shared Memory


#define Direct Memory Access (DMA)
//Synchronous and asynchronous DMA
//Helper functions for DMA transfers
//Bus addresses
//Cache coherency
//Helper functions for coherent DMA mappings
//Helper functions for streaming DMA mappings


#define Levels of Kernel Support

#endif // end of Device Drivers
/* ------------------------------------- */


#endif // end of __IOARCH__H