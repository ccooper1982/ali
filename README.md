# Arch Linux Installer
`ali` is an installer for [Arch Linux](https://archlinux.org/), offering a Qt6 UI as an alternative to the existing [archinstall](https://wiki.archlinux.org/title/Archinstall).

> [!WARNING]
> This is in early development and not ready for general use.

<br/>

[ali arch install video](https://github.com/user-attachments/assets/d4bdbd10-80d9-49b7-873f-8b0cd51b9626)


## Features
- Clear UI, no unnecessary fluff
- Adds only 200MB to the official ISO
- Ensures `/boot` is FAT32
  - Later version will check the size is suitable
- Validation, preventing install if required (limited at the moment, will expand in the future)
- Create `ext4` and `vfat (FAT32)` filesystems (beta)


## Limitations
There are limitations which will be addressed over the coming weeks:

- Does not create/resize partitions, etc. You must do this manually with `fdisk` or `cfdisk` first
- Only tested with a GPT partition table, likely it will only support GPT 
- Only tested with `ext4` on `/`
- Unable to mount `/home` to a dedicated partition
- Bootloader: only GRUB
- Assumes arch is the only OS (doesn't probe for another OS)
- Packages: kernel limited to `linux`; adding packages by name not implemented yet
- Tested only with English language and `uk`/`us` keyboard
- Tested only within a VM
- Tested only one machine (AMD based laptop without a dedicated GPU)

Some UI options are ignored:
- Network:
  - "NTP" option ignored
  - "Copy Config" ignored


## Design
- Qt6 for the UI
- Window manager is openbox
- Piping to commands or libraries (`libblkid` and `libmount`)


## Development / Testing
- Testing is within VirtualBox
- To reflect the real live evironment, a custom Arch live ISO is used, created with [archiso](https://wiki.archlinux.org/title/Archiso)
- It is the same as the [actual](https://wiki.archlinux.org/title/Archiso#Prepare_a_custom_profile) ISO from Arch with 
additional packages
- Additional packages are listed at the bottom of the definition [file](https://github.com/ccooper1982/ali/blob/main/archiso/default/packages.x86_64)

The ISO is 1.3GB, compared to 1.2GB for the official ISO. The extra are mostly libraries/modules required for a minimal
window manager environment. The `ali` executable itself is ~500KB.


## Usage
- Run the ISO in Virtual Box
- The display server is not started at boot, this is to allow creating partitions/filesystems
- Run `startx`
- An `openbox` session is started
- The UI has a log view in the "Install" section. It is separate from the main log
- You can right-click on the desktop, then select "Return to terminal" to `chroot` if required
- Full log file: `/var/log/ali/install.log`


## Development
Aside from removing the limitations listed above:

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
