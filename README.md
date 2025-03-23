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
- Options are limited
  - Boot: only tested with a GPT partition table
  - Bootloader: only GRUB
  - Accounts: only sets password for root, does not add user account yet
  - Packages: only required and kernel (and only `linux`), no additional packages yet
- Tested with English language and `uk` keyboard
- Tested only within a VM

Some UI options are ignored:
- Does not mount `/home`

## Usage
TODO
