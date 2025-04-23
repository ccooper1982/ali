# Arch Linux Installer
`ali` is an installer for [Arch Linux](https://archlinux.org/), offering a Qt6 UI as an alternative to the existing [archinstall](https://wiki.archlinux.org/title/Archinstall).

> [!WARNING]
> This is in early development and not ready for general use.

<br/>

Most recent version:

[ali preview](https://youtu.be/oiti9-pb-IM)

The focus is reliability before adding features:

## Current Features
- Create filesystems
  - `vfat` (FAT32) for `/efi`
  - `ext4` or `btrfs` for `/` and `/home`
  - When using `btrfs`, subvolumes for `@` and `@home` are created
- Ensures `/efi` is FAT32
- Create user account, adding to sudoers if enabled
- Profiles:
  - Minimal: no extra packages installed (but can be added manually)
  - Desktop: Cinnamon, Hyprland
  - Greeters: GDM, SDDM, LightDM GTK and Slick
- Locale: keyboard, language, timezone
- Network: copies live ISO network config for `iwd` and `systemd-networkd`
- GRUB: Probe for other OSes (beta, early testing)
- Validation: prevent install if required


## Limitations

- Does not manage partitions. You must create partitions first

Other limitations which will be addressed over the coming weeks:

- Filesystems: tested only with `ext4` and `btrfs`
- Swap: only option is to enable/disable swap on `zram`
- Bootloader
  - Only GRUB
- Packages
  - Kernel limited to `linux`, will add more soon


## Design
- Qt6 for the UI
- Window manager is openbox
- Piping to commands or using libraries (`libblkid` and `libmount`)


## Development / Testing
- Limited hardware:
  - Laptop: AMD with integrated GPU within VirtualBox
  - Desktop: Intel i7 with an GTX 1060
  - Mini PC: Peland4 (Intel CPU and GPU)

To reflect the real live evironment, a custom Arch live ISO is used, created with [archiso](https://wiki.archlinux.org/title/Archiso):
- It is the same as the [actual](https://wiki.archlinux.org/title/Archiso#Prepare_a_custom_profile) ISO from Arch with additional packages
- Additional packages are listed at the bottom of the definition [file](https://github.com/ccooper1982/ali/blob/main/archiso/default/packages.x86_64)

The ISO is 1.3GB, compared to 1.2GB for the official ISO. The extra are mostly libraries/modules required for a minimal
window manager environment. The `ali` executable itself is ~700KB.

The plan is to add an `ali-bin` to the AUR.


## Development
Aside from removing the limitations listed above:

- Bootloader: add `systemd-boot`
- Configs:
  - Save install config to file
  - Open install config, to either:
    - Start UI, populate fields but don't install
    - Start without UI, check mounts are valid then install without prompt
  
  
## Build
TODO when ready for general use.

## AUR
TODO when ready for general use.
