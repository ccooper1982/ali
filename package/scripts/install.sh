
# call_path=$(dirname "$0")

# echo "Creating user ali"
# useradd -m -G wheel ali
# echo "ali:ali" | chpasswd


# ## TODO count files or don't use 00_, 
# echo "Add ali user to sudoers"
# echo "ali ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/00_ali
# chmod 440 /etc/sudoers.d/00_ali


# echo "Increasing cowspace"
# mount -o remount,size=900M /run/archiso/cowspace

# echo "Updating packages"
# pacman -Syu --noconfirm

# echo "Clearing cache"
# pacman -Scc --noconfirm

# echo "Init pacman keys"
# su -l -c "sudo pacman-key --init" ali

# echo "Installing packages for makepkg"
# su -l -c "sudo pacman -Sy --noconfirm devtools debugedit" ali

# ## TEMP testing only, this will be cloned from AUR
# cp /package/PKGBUILD /home/ali
# cp $call_path/do_install.sh /home/ali
# chown ali:ali /home/ali/PKGBUILD /home/ali/do_install.sh

# su -l -c /home/ali/do_install.sh ali
