# Arch Linux Installer
`ali` is an installer for [Arch Linux](https://archlinux.org/), offering a Qt6 UI as an alternative to the existing [archinstall](https://wiki.archlinux.org/title/Archinstall).

> [!WARNING]
> This is early beta, not ready for public use.

TODO screenshot

## Features
- `/boot` must be EFI (FAT32) 

## Limitations
There are limitations which will be addressed:

- Does not manage partitions or create filesystems, so you must do this manually with `fdisk` or `cfdisk` prior to running `ali`
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
- Everything else: pipe to command (`popen()`) or parsing files (i.e. `/etc/locales.gen`)


## Usage
- There is a custom ISO, created with [archiso](https://wiki.archlinux.org/title/Archiso)
- Run the ISO in Virtual Box
- The display server is not started on boot, this is to allow creating partitions/filesystem
- Run `startx` to start the display server
- Uses `openbox` window manager and to run ali
- During install, there is minimal log entries in the UI, so during `pacstrap` and `pacman` shows no work, but they are running
- Right-click on the desktop then select "Log Out" to return to the terminal (all other options do nothing, and will be removed)
- Log file in `/var/log/ali/install.log` with full logs


## Development
Keep it simple, avoid offering a million options, but do add:

- Bootloader: add `systemd-boot`
- Profiles:
  - Minimal: No desktop
  - Desktop: Not an exhaustive selection
  - Server: Only if significantly different from minimal (i.e. not just minimal + packages + config)
- Configs:
  - Save config to file
  - Open with config:
    - Populating fields
    - Auto install
  
  