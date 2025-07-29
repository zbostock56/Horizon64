```
                  _                 __   _  _   
  /\  /\___  _ __(_)_______  _ __  / /_ | || |  
 / /_/ / _ \| '__| |_  / _ \| '_ \| '_ \| || |_ 
/ __  / (_) | |  | |/ / (_) | | | | (_) |__   _|
\/ /_/ \___/|_|  |_/___\___/|_| |_|\___/   |_|  
                                                
```

Horizon64 is a hobbyist operating system designed for the x86_64 architecture. It was originally booted with a custom bootloader called [sunrise](https://github.com/zbostock56/sunrise), which was developed for the i686 architecture. Horizon64 has since been refactored to operate in 64-bit mode and now is booted with the [limine](https://github.com/limine-bootloader/limine) bootloader.

## Features

- 64-bit kernel
- Basic system initialization
  - ACPI
  - APIC
  - HPET
  - PCI Device Scanning
  - CMOS
  - Keyboard
  - Serial
  - PSF Fonts
  - Symmetric Multi Processing (SMP)
- Physicial and virtual memory management
- Bitmap and slab allocator
- Round-robin Scheduler
- FAT32, ttyfs, pipefs, initrd, ramfs, vfs
- System Calls
- Userspace
    - Basic Terminal
        - cat
        - ls
        - echo
    - libc (h64libc)
    - ELF loader

## Future Plans

- Booting on bare metal
- Switching to Multi-Level Feedback Queue Scheduler
    - Multicore friendly
- Signals
- Text Editors: [bim](https://github.com/klange/bim)
