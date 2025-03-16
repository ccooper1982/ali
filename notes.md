Start at 1.5 of the install guide.

# Locale (1.5)

- Get available layouts: `localectl list-keymaps`
- Load: `loadkeys <name>`
- Font: `setfont ter-132b`
    - May not required since we're in GTK app

# Boot Mode (1.6)
- Read `/sys/firmware/efi/fw_platform_size`
- Only `64`: can't test on 32bit
- If file not exist, we're not in UEFI mode, terminate install

# Network Connection (1.7)
1. Use `ip link` to list the interfaces
    - Could use `ip` [tool](https://web.git.kernel.org/pub/scm/network/iproute2/iproute2.git/tree/ip/iplink.c)
    - But may be easier using `<net/if.h>` and calling [if_nameindex()](https://www.man7.org/linux/man-pages/man3/if_nameindex.3.html)
2. User selects interface
3. Ethernet: go to step 5
4. Wireless: initially use `popen()` with `iwctl`:
    - `iwctl --passphrase <pass> station <device> connect <ssid>`
5. Open TCP socket to remote server to confirm connection

# System Clock (1.8)
- `timedatectl`

# Partitioning (1.9)
1. Show devices/partitions
    - Easy route, use `fdisk -l` or `lsblk -J -f` (JSON output with filesystem info)
    - Future: could attempt to use lsblk or fdisk libraries
        - Example: https://github.com/util-linux/util-linux/blob/master/libblkid/samples/partitions.c
2. User selects partitions for: `/`, `/boot`, `/home`, and `/swap`

# Filesystem (1.10)
1. Check filesystem type of mounts points

# Mount (1.11)
1. Root: `mount <root_partition> /mnt`
2. Boot on UEFI: `mount --mkdir <efi_partition> /mnt/boot`
3. Swap: `swapon <swap_partition>`

# Install (2.0)

## Essential
1. Base package: `pacstrap -K /mnt base linux linux-firmware`
    - See guide for different kernels and omitting firmware pacakge when on VM
2. CPU Microcode: `amd-ucode` or `intel-ucode`

## Firmware
1. `linux-firmware`: See [guide](https://wiki.archlinux.org/title/Mkinitcpio#Possibly_missing_firmware_for_module_XXXX)

## Additional
1. Allow user to add further package names (code, discord, git, etc)

# Configure (3.0)

## fstab
`genfstab -U /mnt >> /mnt/etc/fstab`

## chroot
`arch-chroot /mnt`

## Time
1. Need to get and store timezone in a previous step
2. `ln -sf /usr/share/zoneinfo/<region>/<city> /etc/localtime`
3. `hwclock` to generate `/etc/adjtime`

## Localisation
1. For `/etc/locale.gen`, either:
    a. Uncomment appropriate locale entries, OR
    b. Read file contents, then truncate and write only required entries. File contents read so that we don't lose original entries
     which are required if user changes selection later, OR
    c. As (b), but append the original file contents (with everything commented) so it is there for user's future use

  Note: guide says to uncomment `en_US.UTF-8 UTF-8`, and then uncomment additional. Suggests `en_US` is required?
2. Run `locale-gen` to generate `/etc/locale.conf`
3. Keyboard: write to `/etc/vconsole.conf`, perhaps using:
  a. `localectl set-locale`
  b. `localectl set-x11-keymap`
  See [docs](https://man.archlinux.org/man/localectl.1.en)
  
## Network Config (3.5)
1. Write hostname to `/etc/hostname`

## Initramfs (3.6)
Don't think it's required.

## Root password (3.7)
Set password: Use either:
- [`passwd -q`](https://man.archlinux.org/man/passwd.1.en)
- [`chpasswd`](https://man.archlinux.org/man/chpasswd.8.en)
  
Can validate with `passwd -S`, then confirm 1st field is the username, and 2nd field is 'P'

## Boot Loader
- Use [GRUB](https://wiki.archlinux.org/title/GRUB)
- Confirm boot partition is FAT32 (partition selection stage can force it)

1. Install `grub` and `efibootmgr`
2. `grub-install --target=x86_64-efi --efi-directory=/mnt/boot --bootloader-id=GRUB`





