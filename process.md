# Startup
- Open/truncate log file: `/var/log/ali/ali.log`
- Confirm required commands
- Confirm 64bit platform 
- Confirm network activity
- Sync lock: `timedatectl`


# Prepare

## Partitions and Mounts
- Show partitions
  - Highlight EFI/FAT32 for `/boot`
    - Warn if size < 256mb
- Users sets `/boot`, `/`, `/home`
- Ask if `/home` uses `/`, otherwise set explicitly
- Confirm `/` and `/home` have suitable filesystem (i.e. `ext4`)


## Locale
- User selects mapping from `localectl list-keymaps`
- Terminal font: `setfont ter-132b`


## Timezone
1. `timedatectl list-timezones`
2. Then `timedatectl set-timezone <ZONE>`


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
1. Base package: `pacstrap -K /mnt base linux linux-firmware`
    - See guide for different kernels and omitting firmware pacakge when on VM
2. CPU Microcode: `amd-ucode` or `intel-ucode`


## Firmware
- Prompt to install `linux-firmware` 


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
  