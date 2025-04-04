# Init

`-v`: verbose
`-w`: working dir. note this uses tmpfs, requiring: 
`-r`: delete working directory when complete

`sudo mkarchiso -v -r -w /tmp/archiso-tmp -o archiso/iso archiso/profile/`

`export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/qt6/plugins`

# Startup
- Open/truncate log file: `/var/log/ali/ali.log`
- Confirm required commands
- Confirm 64bit platform 
- Confirm network activity
- Sync lock: `timedatectl`

# Prepare

## Locale
- User selects mapping from `localectl list-keymaps`
- Terminal font: `setfont ter-132b` : WARNING - this is only suitable for high DPI and fullscreen, unusable otherwise
  - Can be reset with `setfont` (`setfont -R` failed)


## Timezone
1. `timedatectl list-timezones`
2. Then `timedatectl set-timezone <ZONE>`


## Partitions and Mounts
- Show partitions
  - Highlight EFI/FAT32 for `/boot`
    - Warn if size < 256mb
- Users sets `/boot`, `/`, `/home`
- Ask if `/home` uses `/`, otherwise set explicitly
- Confirm `/` and `/home` have suitable filesystem (i.e. `ext4`)


## Mount
1. Root: `mount <root_partition> /mnt`
2. Boot on UEFI: `mount --mkdir <efi_partition> /mnt/boot`
3. Swap: `swapon <swap_partition>`


## Video
Could eventually use [map](https://github.com/lfreist/hwinfo/blob/main/include/hwinfo/utils/pci.ids.h) of vendor to devices.

- Offer nvidia or AMD drivers
  - Nvidia open: `nvidia-open` (Turing (GTX16 and RTX20) onwards) -- recommended
  - Nvidia closed: `nvidia` (Maxwell to Lovelace (GT740 to GTX750 Ti, then GTX900 to RTX40))
  - Open source: `mesa` then `vulkan-nouveau` (don't think we need `xf86-video-nouveau`)

## Audio
- pulseaudio
  - `pulseaudio`
  - Offer `pulseaudio-bluetooth`
- pipewire
  - `pipewire`


# Install

## Prepare Pacman
1. Prompt to install `linux-firmware` 
2. Base package: `pacstrap -K /mnt base linux linux-firmware`
    - See guide for different kernels and omitting firmware pacakge when on VM
3. CPU Microcode: `amd-ucode` or `intel-ucode`


## Additional
- User can type additional package names
- Suggest common:
  - VS Code
    - pacman: `code` (`visual-studio-code-bin` & `vscodium` are AUR)
  - Browser:
    - `firefox`
    - `chromium`
    - `vivaldi`, `vivaldi-ffmpeg-codecs`
  - `git`
  - `discord`
  