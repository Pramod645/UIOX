Key difference between the two builds:



                        Static	                                    Dynamic
BSP binary	            Linked into uiox_kernel.elf	                Standalone uiox_bsp.elf / .bin
Primary BL target	    Jumps directly to uiox_kernel_main	        Jumps to bsp_entry.S stub
Kernel loading	        Already in flash/RAM at link address	    BSP reads kernel ELF, copies segments to DRAM
BSP entry               point	uiox_bsp_init() called by kernel	uiox_bsp_entry_c() runs before kernel exists
Use case	            Tightly integrated SoC, small flash	        Separate bootloader partition, OTA updates
=================================================================================================================
