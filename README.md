# Arch Linux Installer
`ali` is an installer for [Arch Linux](https://archlinux.org/), offering a Qt6 UI as an alternative to the existing [archinstall](https://wiki.archlinux.org/title/Archinstall).

> [!WARNING]
> This is early beta, not ready for public use.


<!-- ![start_screen_small](https://github.com/user-attachments/assets/442793c1-7874-49ef-8b44-964fcbbd0643) -->

<img src="https://github.com/user-attachments/assets/442793c1-7874-49ef-8b44-964fcbbd0643" alt="Alt Text" width="500">

[start.webm](https://github.com/user-attachments/assets/1a7deb2e-c4d3-4a1b-b053-5fadae5f32c7)



## Features
- `/boot` must be EFI (FAT32) 

## Limitations
There are limitations which will be addressed:

- Does not manage partitions or create filesystems. You must do this manually with `fdisk` or `cfdisk` prior to running `ali`
- Install process assumes `/` is `ext4`
- Only tested with a GPT partition table
- Bootloader: only GRUB
- Accounts: only sets password for root
- Packages: only required packages installed, and kernel is only `linux` - no additional packages yet
- Tested only with English language and `uk`/`us` keyboard
- Tested only within a VM
- Tested only one machine (AMD based laptop without a dedicated GPU)

Some UI options are ignored:
- Mounts:
  - Does not mount `/home`, so it always uses `/`
- Accounts:
  - User account name/password ignored (user not added)
- Network:
  - "NTP" option ignored
  - "Copy Config" ignored

## Design
- Qt6 for the UI
- Partitions, filesystems and mounting: `libblkid` and `libmount`
- Everything else: open pipe (`popen()`) or parsing files (i.e. `/etc/locales.gen`)


## Usage
- There is a custom ISO, created with [archiso](https://wiki.archlinux.org/title/Archiso)
- Run the ISO in Virtual Box
- The display server is not started on boot, this is to allow creating partitions/filesystem
- Run `startx`
- An `openbox` session is started
- The UI has a log view. It is separate from the main log, displaying minimal information. So some operations appear dead (i.e. `pacstrap` and `pacman`), but they are running
- You can right-click on the desktop, then select "Log Out" to return to the terminal (all other options do nothing, and will be removed)
- Full log file: `/var/log/ali/install.log`


## Development
Keep it simple, avoid offering a million options, but do add:

- Bootloader: add `systemd-boot`
- Profiles:
  - Minimal: No desktop
  - Desktop: Not an exhaustive selection
  - Server: Only if significantly different from minimal (i.e. not just minimal + packages + config)
- Configs:
  - Save install config to file
  - Open install config, to either:
    - Start UI, populate fields but don't install
    - No UI, check mounts are valid then installs without prompt
  
  
## Build
TODO
