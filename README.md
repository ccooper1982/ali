# Arch Linux Installer
`ali` is an installer for [Arch Linux](https://archlinux.org/), offering a Qt6 UI as an alternative to the existing [archinstall](https://wiki.archlinux.org/title/Archinstall).

> [!WARNING]
> This is in early development and not ready for general use.

<br/>

[ali arch install video](https://github.com/user-attachments/assets/d4bdbd10-80d9-49b7-873f-8b0cd51b9626)

The focus is reliability before adding features, such as desktop environments or additional bootloaders. 

## Current Features
- Create filesystems (`vfat` for `/boot`, and just `ext4` for `/` at the moment)
- Ensures `/boot` is FAT32
- Create user account, adding to sudoers if enabled
- Locale: keyboard, language, timezone
- Network: copies live ISO network config (`iwd` and `systemd-networkd`)
- GRUB: Probe for other OSes (beta, early testing)
- Validation, prevent install if required


## Limitations
There are limitations which will be addressed over the coming weeks:

- Does not manage partitions. You must create partitions first
- Only tested with `ext4` on `/`
- No dedicated swap partition
- Bootloader
  - Only GRUB
- Packages
  - Kernel limited to `linux`
  - Adding packages by name not implemented yet
- Network management assumed to be `iwd` (WiFi) and `systemd-networkd`


## Design
- Qt6 for the UI
- Window manager is openbox
- Piping to commands or using libraries (`libblkid` and `libmount`)


## Development / Testing
- Limited hardware:
  - Laptop: AMD with integrated GPU within VirtualBox
  - Desktop: Intel i7 with an GTX 1060

To reflect the real live evironment, a custom Arch live ISO is used, created with [archiso](https://wiki.archlinux.org/title/Archiso):
- It is the same as the [actual](https://wiki.archlinux.org/title/Archiso#Prepare_a_custom_profile) ISO from Arch with additional packages
- Additional packages are listed at the bottom of the definition [file](https://github.com/ccooper1982/ali/blob/main/archiso/default/packages.x86_64)

The ISO is 1.3GB, compared to 1.2GB for the official ISO. The extra are mostly libraries/modules required for a minimal
window manager environment. The `ali` executable itself is ~500KB.

The plan is to add an `ali-bin` to the AUR.


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

## AUR
TODO when ready for general use.
