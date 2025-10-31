/*
//The I/O subsystem allows a process to communicate with peripheral devices such
as disks, tape drives, terminals, printers, and networks, and the kernel modules that
control devices are known as device drivers.
//There is usually a one-to-one
correspondence between device drivers and device types: Systems may contain one
disk driver to control all disk drives, one terminal driver to control all terminals,
and one tape driver to control all tape drives.
//The system supports "software devices," which have no associated physical
device. For example, it treats physical memory as a device to allow a process
access to physical memory outside its address space, even though memory is not a
peripheral device.
//
*/