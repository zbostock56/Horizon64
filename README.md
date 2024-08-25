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
  - HPET
  - CMOS
  - Keyboard
  - Serial
  - PSF Fonts
- Physicial and virtual memory management
- Bitmap allocator

## Future Plans

- Process management
- Booting on bare metal
