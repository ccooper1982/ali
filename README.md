# Arch Linux Installer
`ali` is an installer for [Arch Linux](https://archlinux.org/), offering a Qt6 UI as an alternative to the existing [archinstall](https://wiki.archlinux.org/title/Archinstall).

> [!WARNING]
> This is early beta, not ready for public use.

TODO screenshot

## Features
- `/boot` must be EFI (FAT32) 

## Limitations
There are limitations which will be addressed:

- It yet doesn't manage partitions or create filesystems, so you must do this manually with `fdisk` or `cfdisk` prior to running `ali`
- Install process assumes `/` is `ext4`
- Only tested with a GPT partition table
- Bootloader: only GRUB
- Accounts: only sets password for root
- Packages: only required packages installed, and kernel is only `linux` - no additional packages yet
- Tested only with English language and `uk`/`us` keyboard
- Tested only within a VM

Some UI options are ignored:
- Does not mount `/home`
- User account name/password ignored (user not added)

## Usage
TODO
