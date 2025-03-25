# Arch Linux Installer
`ali` is an installer for [Arch Linux](https://archlinux.org/), offering a Qt6 UI as an alternative to the existing [archinstall](https://wiki.archlinux.org/title/Archinstall).

> [!WARNING]
> This is in early development and not ready for general use.


<!-- ![start_screen_small](https://github.com/user-attachments/assets/442793c1-7874-49ef-8b44-964fcbbd0643) -->

<img src="https://github.com/user-attachments/assets/442793c1-7874-49ef-8b44-964fcbbd0643" alt="Alt Text" width="500">

[start.webm](https://github.com/user-attachments/assets/1a7deb2e-c4d3-4a1b-b053-5fadae5f32c7)



## Features
- Clear UI, no unnecessary fluff
- Adds only 200MB to the official ISO
- Ensures `/boot` is EFI (FAT32) 
  - Later version will check the size is suitable
- Validation, preventing install if required (limited at the moment, will expand in the future)


## Limitations
There are limitations which will be addressed over the coming weeks:

- Does not manage partitions or create filesystems. You must do this manually with `fdisk` or `cfdisk` before starting `ali`
- Doesn't use the root filesystem type, assumes `/` is `ext4`
- Only tested with a GPT partition table (with root as `ext4`)
- Bootloader: only GRUB
- Locale: Ignored / unavailable
- Accounts: only sets password for root, and no user account created
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
- Piping to commands or using sys calls or libraries (`libblkid` and `libmount`)


## Development / Testing
- Testing is within VirtualBox
- To reflect the real live evironment, a custom Arch live ISO is used, created with [archiso](https://wiki.archlinux.org/title/Archiso)
- It is the same as the [actual](https://wiki.archlinux.org/title/Archiso#Prepare_a_custom_profile) ISO from Arch with 
additional packages
- Additional packages are listed at the bottom of the definition [file](https://github.com/ccooper1982/ali/blob/main/archiso/default/packages.x86_64)

The ISO is 1.4GB, compared to 1.2GB for the official ISO. The extra 200MB is mostly libraries/modules required for a minimal
window manager environment. The `ali` executable itself is ~450KB.


## Usage
- Run the ISO in Virtual Box
- The display server is not started at boot, this is to allow creating partitions/filesystem
- Run `startx`
- An `openbox` session is started
- The UI has a log view in the "Install" section. It is separate from the main log, displaying only minimal information. So some operations appear dead (i.e. `pacstrap` and `pacman`) but they are running
- You can right-click on the desktop, then select "Log Out" to return to the terminal (all other options do nothing, and will be removed)
- Full log file: `/var/log/ali/install.log`


## Development
Keep it simple, avoid offering a million options, but do add:

- Bootloader: add `systemd-boot`
- Profiles:
  - Minimal: No desktop
  - Desktop: Not an exhaustive selection
- Configs:
  - Save install config to file
  - Open install config, to either:
    - Start UI, populate fields but don't install
    - Start without UI, check mounts are valid then install without prompt
  
  
## Build
TODO when ready for general use.
