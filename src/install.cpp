#include <ali/install.hpp>
#include <ali/disk_utils.hpp>
#include <ali/locale_utils.hpp>
#include <ali/packages.hpp>
#include <ali/profiles.hpp>
#include <ali/widgets/widgets.hpp>
#include <sstream>
#include <string>
#include <fstream>
#include <sys/mount.h>
#include <QDebug>

// temp mounts: partitions are mounted before running GRUB's os-prober
static const fs::path TmpMountPath {"/tmp/ali/mnt"};
static std::vector<std::string> temp_mounts;



void Install::log_stage_start(const std::string_view stage)
{
  qInfo() << "Stage start: " << stage;
  emit on_stage_start(QString::fromLocal8Bit(stage.data(), stage.size()));
}

void Install::log_stage_end(const std::string_view stage)
{
  qInfo() << "Stage end: " << stage;
  emit on_stage_end(QString::fromLocal8Bit(stage.data(), stage.size()));
}

void Install::log_critical(const std::string_view msg)
{
  qCritical() << msg;
  emit on_log_critical(QString::fromLocal8Bit(msg.data(), msg.size()));
}

void Install::log_warning(const std::string_view msg)
{
  qWarning() << msg;
  emit on_log_warning(QString::fromLocal8Bit(msg.data(), msg.size()));
}

void Install::log_info(const std::string_view msg)
{
  qInfo() << msg;
  emit on_log_info(QString::fromLocal8Bit(msg.data(), msg.size()));
}


void Install::install ()
{
  auto exec_stage = [this](std::function<bool(Install&)> f, const std::string_view stage) mutable
  {
    log_stage_start(std::format("{} - Start", stage));
    
    const bool ok = f(std::ref(*this));

    log_stage_end(std::format("{} - {}", stage, ok ? "Success" : "Fail"));
    return ok;
  };
  
  // TODO should probably have: exec_required() and exec_can_fail()
  //      because minimal operations must all suceed, but 'extra' can
  //      fail, so no need to return bool type

  try
  {
    const bool minimal =  exec_stage(&Install::filesystems, "filesystems") &&
                          exec_stage(&Install::mount, "mount") &&
                          exec_stage(&Install::pacman_strap, "pacstrap") &&
                          exec_stage(&Install::swap, "swap") &&
                          exec_stage(&Install::fstab, "fstab") &&                          
                          exec_stage(&Install::network, "network") &&
                          exec_stage(&Install::root_account, "root account") &&
                          exec_stage(&Install::user_account, "user account") &&
                          exec_stage(&Install::boot_loader, "bootloader");
    
    emit on_complete(minimal ? CompleteStatus::MinimalSuccess : CompleteStatus::MinimalFail);

    if (minimal)
    {
      // if any of these fail, they still return true because it is not a show stopper
      
      // TODO additional packages

      // shell is extra because 'bash' is installed as part of 'base'
      const bool extra =  exec_stage(&Install::shell, "shell") &&
                          exec_stage(&Install::profile, "profile") &&
                          exec_stage(&Install::packages, "packages") &&
                          exec_stage(&Install::gpu, "video") &&
                          exec_stage(&Install::localise, "locale");
      
      emit on_complete(extra ? CompleteStatus::ExtraSuccess : CompleteStatus::ExtraFail);
    }
  }
  catch(const std::exception& e)
  {
    log_critical(e.what());
  }
  catch (...)
  {
    log_critical("Install encountered an unknown exception");
  }
}


// filesystems
bool Install::filesystems()
{
  // TODO after adding btrfs, this function has become a slight mess

  const auto [valid, mounts] = Widgets::partitions()->get_data();

  // sanity: UI should prevent this
  if (!valid)
  {
    log_critical("Mounts are invalid");
    return false;
  }

  const bool create_home_partition = mounts.home.create_fs && mounts.home.dev != mounts.root.dev;

  bool root{false}, efi{true}, home{true};

  // wipefs, set partition type, create new fs for root, boot and home if applicable
  if (mounts.root.create_fs)
  {
    if (!wipe_fs(mounts.root.dev))
      return false;

    if (mounts.root.fs == "btrfs")
      root = create_btrfs_filesystem(mounts.root.dev, RootMnt, MountType::Root);
    else if (root)
      root = create_filesystem(mounts.root.dev, mounts.root.fs);

    if (root)
    {
      log_info("Setting root partition type");
      set_partition_type<SetPartitionAsLinuxRoot>(mounts.root.dev);
    }
  }
  
  if (mounts.efi.create_fs)
  {
    efi = wipe_fs(mounts.efi.dev) && create_filesystem(mounts.efi.dev, mounts.efi.fs);
    if (efi)
    {
      log_info("Setting efi partition type");
      set_partition_type<SetPartitionAsEfi>(mounts.efi.dev);
    }
  }
  
  if (create_home_partition)
  {
    if (!wipe_fs(mounts.home.dev))
      return false;
    
    if (mounts.home.fs == "btrfs")
      home = create_btrfs_filesystem(mounts.home.dev, HomeMnt, MountType::Home);
    else if (home)
      home = create_filesystem(mounts.home.dev, mounts.home.fs);

    if (home)
    {
      log_info("Setting home partition type");
      set_partition_type<SetPartitionAsLinuxHome>(mounts.home.dev);  
    }
  }
  else
  {
    // create subvol for @home, which is mounted to root
    home = create_btrfs_subvolume(mounts.root.dev, RootMnt, "@home");
  }

  return root && efi && home;
}


bool Install::wipe_fs(const std::string_view dev)
{
  log_info(std::format("Wiping filesystem on {}", dev));
  Command wipe {std::format ("wipefs -a -f {}", dev)};
  return wipe.execute() == CmdSuccess;
}


bool Install::create_btrfs_filesystem(const std::string_view part_dev, const fs::path mount, const MountType type)
{
  // for btrfs, we make btrfs, mount it without options, create subvolume, then unmount.
  // the mount step following this function will mount with appropriate btrfs options
  // so that fstab is created correctly

  bool ok{false};

  log_info(std::format("Creating btrfs on {}", part_dev));

  if (type == MountType::Esp)
    log_critical("Invalid mount type for BTRFS volume. Must be home or root");
  else
  {
    if (CreateBtrFs cmd{part_dev}; cmd.execute() == CmdSuccess)
      ok = create_btrfs_subvolume(part_dev, mount, type == MountType::Root ? "@" : "@home");
  }

  return ok;
}


bool Install::create_btrfs_subvolume(const std::string_view part_dev, const fs::path mount, const std::string_view subvolume)
{
  log_info(std::format("Creating subvolume {} on {}", subvolume, mount.string()));

  bool ok {false};

  if (do_mount(part_dev, mount.string(), "btrfs", no_fs_opts{}))
  {
    CreateBtrVolume cmd_volume{mount, subvolume};
    ok =  cmd_volume.execute() == CmdSuccess &&
          ::umount(mount.string().data()) == 0;
  }

  return ok;
}


bool Install::create_filesystem(const std::string_view part_dev, const std::string_view fs)
{
  log_info(std::format("Creating {} on {}", fs, part_dev));

  int res {CmdFail};
  
  if (fs == "ext4")
  {
    CreateExt4 cmd{part_dev};
    res = cmd.execute();
  }
  else if (fs == "vfat")
  {
    CreateFat32 cmd{part_dev};
    res = cmd.execute();
  }

  if (res != CmdSuccess)
  {
    qCritical() << "Failed to create filesystem: " << strerror(res);
    return false;
  }
  else
    return true;
}


// mounting
bool Install::mount()
{
  bool mounted_root{false}, mounted_efi{false}, mounted_home{true};

  const auto [valid, mount_data] = Widgets::partitions()->get_data();

  if (!valid)
    log_critical("Could not get partition paths and filesystems");
  else
  {
    // TODO this only checks if path is mounted (i.e. /mnt/boot), should it also 
    //      check if device is mounted (i.e. /dev/sda2)? If the device is mounted
    //      elsewhere, we should fail.

    if (PartitionUtils::is_path_mounted(EfiMnt.string()))
    {
      log_info(std::format("{} is already mounted, unmounting", EfiMnt.c_str()));
      ::umount(EfiMnt.c_str());
    }

    if (PartitionUtils::is_path_mounted(RootMnt.string()))
    {
      log_info(std::format("{} is already mounted, unmounting", RootMnt.c_str()));
      ::umount(RootMnt.c_str());
    }

    if (PartitionUtils::is_path_mounted(HomeMnt.string()))
    {
      log_info(std::format("{} is already mounted, unmounting", HomeMnt.c_str()));
      ::umount(HomeMnt.c_str());
    }

    const bool is_root_btr = mount_data.root.fs == "btrfs";
    const bool is_home_btr = mount_data.home.fs == "btrfs";

    mounted_root = do_mount(mount_data.root.dev, RootMnt.c_str(), mount_data.root.fs, is_root_btr ? "subvol=@" : no_fs_opts{});
    mounted_efi = do_mount(mount_data.efi.dev, EfiMnt.c_str(), mount_data.efi.fs);

    log_info(std::format("Mount of {} -> {} : {}", RootMnt.c_str(), mount_data.root.dev, mounted_root ? "Success" : "Fail"));
    log_info(std::format("Mount of {} -> {} : {}", EfiMnt.c_str(), mount_data.efi.dev, mounted_efi ? "Success" : "Fail"));

    // if btrfs, we still want to mount home, even if it's on the same partition as root, because it's a subvolume
    if (mount_data.root.fs == "btrfs" || mount_data.home.dev != mount_data.root.dev)
    {
      mounted_home = do_mount(mount_data.home.dev, HomeMnt.c_str(), mount_data.home.fs, is_home_btr ? "subvol=@home" : no_fs_opts{});
      log_info(std::format("Mount of {} -> {} : {}", HomeMnt.c_str(), mount_data.home.dev, mounted_home ? "Success" : "Fail"));
    }
  }

  return mounted_root && mounted_efi && mounted_home;
}


bool Install::do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs, const std::string_view options)
{
  if (!fs::exists(path))
    fs::create_directory(path);

  const int r = ::mount(dev.data(), path.data(), fs.data(), 0, options.data()) ;
  
  if (r != 0)
    log_critical(std::format("do_mount(): {} {}", path, ::strerror(errno)));

  qDebug() << "Leaving";

  return r == 0;    
}


// pacstrap
/// This only installs required, kernels, firmware and important,
/// i.e. packages that are required/important to a useful bootable Arch
bool Install::pacman_strap()
{
  auto create_cmd_string = []()
  {
    // pacstrap -K <root_mount> <package_list>

    std::stringstream cmd_string;
    cmd_string << "pacstrap -K " << RootMnt.string() << ' ';
    cmd_string << Packages::required();
    cmd_string << Packages::kernels();
    cmd_string << Packages::firmware();
    cmd_string << Packages::important();

    return cmd_string.str();
  };

  
  const auto cmd_string = create_cmd_string();

  log_info(cmd_string);
  
  // intercept missing firmware message from pacstrap
  Command pacstrap {cmd_string, [this](const std::string_view out)
  {    
    if (out.find("Possibly missing firmware for module:") != std::string::npos)
      log_warning("Possibly missing firmware. This can be fixed post-install");
    else
      log_info(out);
  }};
  
  bool ok = true;
  if (pacstrap.execute() != CmdSuccess)
  {
    log_critical("ERROR: pacstrap failed - manual intervention required");
    ok = false;
  }

  return ok;
}


bool Install::swap()
{
  bool ok = false;

  const auto data = Widgets::swap()->get_data();

  if (data.zram_enabled)
  {
    log_info("Installing zram generator");
    
    if (pacman_install({"zram-generator"}))
    {
      static const fs::path ZramConfig {RootMnt / "etc/systemd/zram-generator.conf"};

      {
        // write the config, keep it simple for now, with values suggested in wiki
        std::ofstream cfg_file{ZramConfig, std::ios_base::out | std::ios_base::trunc};
        cfg_file << "[zram0]\n";
        cfg_file << "zram-size = min(ram / 2, 4096)\n";
        cfg_file << "compression-algorithm = zstd";
      }
          
      if (fs::exists(ZramConfig) && fs::file_size(ZramConfig))
      {
        ok = enable_service("systemd-zram-setup@zram0.service");
      }
      else
      {
        log_critical("Failed to create zram config") ; 

        // delete if it exists, rather than leaving a potentially broken configuration
        if (fs::exists(ZramConfig))
          fs::remove(ZramConfig);
      }
    }
  }

  return ok;
}


// fstab
bool Install::fstab()
{
  fs::create_directory((RootMnt / FsTabPath).parent_path());

  Command fstab {std::format("genfstab -U {} > {}", RootMnt.string(), FsTabPath.string())};

  const bool ok = fstab.execute() == CmdSuccess && fs::exists(FsTabPath) && fs::file_size(FsTabPath);
  
  if (!ok)
    log_critical("fstab failed");
  
  return ok;
}


// locale
bool Install::localise()
{
  const auto locale_data = Widgets::start()->get_data();

  log_info("Setting timezone");
  if (!LocaleUtils::generate_timezone(locale_data.timezone))
  {
    log_warning("Setting timezone failed");
  }
  
  log_info("Generating locales");
  if (!LocaleUtils::generate_locale(locale_data.locales, locale_data.locales[0]))
  {
    log_warning("Generating/setting locales failed");
  }

  log_info(std::format("Setting keymap {}", locale_data.keymap));
  if (!LocaleUtils::generate_keymap(locale_data.keymap, true))
  {
    log_warning("Setting key map failed");
  }

  // locale not considered essential, failure is an annoyance
  return true; 
}


// network
bool Install::network()
{
  const auto data = Widgets::network()->get_data();
  
  {
    std::ofstream host_stream {RootMnt / "etc/hostname", std::ios_base::out | std::ios_base::trunc};
    if (host_stream.good())
      host_stream << data.hostname << '\n';
    else
      log_warning("Failed to set /etc/hostname");
  }
  
  // intentionally continuing if hostname is unset
  if (data.copy_config)
  {
    static const fs::path sysd_config_src {"/etc/systemd/network"};
    static const fs::path sysd_config_dest {RootMnt / "etc/systemd/network"};

    // some DE uses NetworkManager and others use IWD. 
    // TODO don't handle cases where NetworkManager is configured to use iwd as a backend
    if (Packages::have_package("iwd"))
    {
      // iwd config: https://wiki.archlinux.org/title/Iwd#Network_configuration
      static const fs::path iwd_config_src {"/var/lib/iwd"};
      static const fs::path iwd_config_dest {RootMnt / "var/lib/iwd"};

      if (copy_files(iwd_config_src, iwd_config_dest, {".psk", ".open", ".8021x"}))
        enable_service("iwd.service");
      else
        log_critical("Failed to configure iwd");  
    }

    // systemd-networkd: we always configure
    if (copy_files(sysd_config_src, sysd_config_dest, {".network", ".netdev", ".link"}))
    {
      enable_service("systemd-networkd.service");
      enable_service("systemd-resolved.service");
    }
    else
      log_critical("Failed to configure systemd-networkd");  
    

    // TODO ModemManager for wwlan (mobile 3g/4g, etc)?
    //      https://wiki.archlinux.org/title/Mobile_broadband_modem#ModemManager
  }

  if (data.ntp)
  {
    // this enables NTP
    enable_service("systemd-timesyncd.service");
  }

  // failing to setup network not an error
  return true; 
}


// accounts
bool Install::root_account()
{
  const std::string root_passwd = Widgets::accounts()->root_password();

  // sanity check: UI should prevent this
  if (root_passwd.empty())
  {
    log_critical("Root password empty");
    return false;
  }

  return set_password("root", root_passwd);
}


bool Install::user_account()
{
  const std::string username = Widgets::accounts()->user_username();
  const std::string password = Widgets::accounts()->user_password();

  bool user_created{false};

  // sanity check: username and password should be validated by UI
  if (username.empty())
  {
    user_created = true;
    log_info("No user to create");
  }
  else if (!username.empty() && password.empty())
  {
    user_created = false;
    log_critical("Have username but not password");
  }
  else
  {
    // this is called during minimal installation, hence it sets the shell to bash. The shell selected
    // in the UI is installed by shell()

    const bool can_sudo = Widgets::accounts()->user_is_sudo();
    const std::string wheel_group = can_sudo ? "-G wheel" : "";

    // use the `useradd` command, without the password, set that after with passwd
    //  -s shell
    //  -m create home directory
    //  -G wheel (if sudo permitted)
    ChRootCmd useradd{std::format("useradd -s /usr/bin/bash -m {} {}", wheel_group, username)} ;

    if (const int r = useradd.execute(); r != CmdSuccess)
    {
      log_info(std::format("User created failed: {}", strerror(r)));
    }
    else
    {
      if (!set_password(username, password))
        log_critical("Failed to set user password");
      else if (can_sudo && !add_to_sudoers(username))      
        log_critical("Failed to add user to sudoers");
      else
        user_created = true;
    }
  }

  return user_created;
}


bool Install::add_to_sudoers(const std::string& user)
{
  using namespace std::string_literals;

  static const fs::perms SudoersFilePerms = fs::perms::owner_read | fs::perms::group_read;
  static const fs::path SudoersD = RootMnt / "etc/sudoers.d";

  log_info("Adding user to sudoers");

  std::error_code ec;
  if (fs::create_directories(SudoersD, ec); ec.value() != 0)
  {
    log_info(std::format("Failed to add user to sudoers: {}", strerror(ec.value())));
    return false;
  }
  else
  {
    // sudoers filename: ##_<username>
    //  where ## a 2-digit number, incrementing, starting at 00
    const auto n_files = std::distance(fs::directory_iterator {SudoersD}, fs::directory_iterator{});
    const auto sudoers_path = fs::path(SudoersD / std::format("{:02}_{}", n_files, user));

    qInfo() << "sudoers file path: " << sudoers_path.string();

    // restrict scope to close stream before fiddling permissions
    {
      qDebug() << std::format("{} ALL=(ALL) ALL", user);

      // <username> <all hosts>=(<all users>) <all commands>
      std::ofstream sudoers_stream{sudoers_path};
      sudoers_stream << std::format("{} ALL=(ALL) ALL", user);
    }
    
    // set permissions
    if (fs::permissions(sudoers_path, SudoersFilePerms, ec); ec.value() != 0)
    {
      log_info(std::format("Failed to add user to sudoers: {}", strerror(ec.value())));
      return false;
    }
    else
    {
      return true;
    }
  }
}


bool Install::set_password(const std::string_view user, const std::string_view pass)
{
  log_info(std::format("Setting password for {}", user));

  bool pass_set{false};

  ChRootCmd cmd_passwd{std::format("echo {}:{} | chpasswd", user, pass), true};
  if (cmd_passwd.execute() == CmdSuccess)
  {
    qDebug() << "chpasswd success";

    // we don't launch shell so that the output of `passwd` command is the only output returned
    ChRootCmd cmd_check{std::format("passwd -S {}", user), [&user, &pass_set](const std::string_view out)
    {
      qDebug () << "Password check returns: " << out;

      if (out.size() > user.size()) // at least "<user> P"
      {
        if (const auto pos = out.find(' '); pos != std::string_view::npos)
          pass_set = out.substr(pos+1, 1) == "P";
      }
    }};

    cmd_check.execute(); // if this fails, pass_valid remains false
  }

  return pass_set;
}


// bootloader
bool Install::boot_loader()
{
  bool ok = false;
  int r = 0;

  // TODO: systemd-boot

  // TODO if btrfs, install grub-btrfs
  if (!pacman_install({"grub", "efibootmgr", "os-prober"}))
  {
    log_critical("pacman install failed");
  }
  else
  {
    const std::string install_cmd = std::format("grub-install --target=x86_64-efi --efi-directory=/efi --bootloader-id=GRUB");

    qDebug() << "GRUB install command: " << install_cmd;
    
    ChRootCmd grub_install{install_cmd, [this](const std::string_view out)
    {
      log_info(out);
    }};

    if (r = grub_install.execute(); r != CmdSuccess)
    {
      log_critical(std::format("grub-install failed: {}", strerror(r)));
    }
    else
    {
      if (!prepare_grub_probe())
      {
        // not a reason to stop: it's annoying for user but they can still have a working Arch
        log_critical("Failed to setup for grub's os-prober");
      }
      else
      {
        ChRootCmd grub_config{"grub-mkconfig -o /boot/grub/grub.cfg", [this](const std::string_view out)
        {
          log_info(out);
        }};
  
        if (r = grub_config.execute(); r != CmdSuccess)
          log_critical(std::format("grub-mkconfig failed: {}", strerror(r)));
        else
          ok = true;
      }

      cleanup_grub_probe();
    }
  }

  return ok;
}


bool Install::prepare_grub_probe()
{
  log_info("Preparing for GRUB os-probe");

  // NOTE from arch guide:
  //  "os-prober might not work properly when run in a chroot. Try again after rebooting into the system if you experience this."
  // GRUB_DISABLE_OS_PROBER is commented out, easier to just append
  bool updated_cfg{true}, mounted{true};
  
  std::ofstream grub_cfg{RootMnt / fs::path{"etc/default/grub"}, std::ios_base::app};
  if (grub_cfg.good())
    grub_cfg << "GRUB_DISABLE_OS_PROBER=false";
  else
  {
    updated_cfg = false;
    log_warning("Failed to update GRUB config file");
  }

  if (updated_cfg)
  {
    log_info("Mounting other partitions");

    // the os-prober uses the mount table to search partitions, so we must mount all applicable partitions
    if (!PartitionUtils::probe_for_os_discover())
    {
      log_critical("Failed to probe for other OS partitions");
      mounted = false;
    }
    else
    {
      // the mount point doesn't actually matter, just that we mount, updating the mount table
      // we need to do this as chroot, so use ChrootCmd rather than calling do_mount(), so we mount
      // to /tmp/ali/mnt/0, /tmp/ali/mnt/1 , etc . We later clean up with recursive `umount`
      // ChRootCmd create_mnt_dir {std::format("mkdir {}", TmpMountPath.string())};
      // create_mnt_dir.execute();
      const auto parts = PartitionUtils::partitions();
      const auto [_, mount_data] = Widgets::partitions()->get_data(); // '_' must be valid or we'd not be here

      unsigned int i = 0;
      for (const auto& part : parts)
      {
        // partition can't contain an OS if it's EFI or has no filesystem,
        // and we don't mount any of the partitions of the installed Arch
        if (!part.is_efi && !part.fs_type.empty() &&  part.dev != mount_data.efi.dev &&
                                                      part.dev != mount_data.root.dev && 
                                                      part.dev != mount_data.home.dev)
        {
          const auto fs = part.fs_type == "ntfs" ? "ntfs3" : part.fs_type; // TODO unreliable hack?

          const auto mount_point = std::format("{}/{}", TmpMountPath.string(), i++);
          const auto mount_cmd = std::format("mount --mkdir --read-only --target-prefix {} -t {} {} {}", RootMnt.string(), fs, part.dev, mount_point);
          
          if (ChRootCmd cmd{mount_cmd}; cmd.execute() != CmdSuccess)
          {
            log_warning(std::format("Failed to mount {} -> {}", part.dev, mount_point)); 
            mounted = false;
            break;
          }
          else
          {
            log_info(std::format("Mounted {} -> {}", part.dev, mount_point));
            temp_mounts.emplace_back(mount_point);
          }
        }
      }
    }
  }

  return updated_cfg && mounted;
}


void Install::cleanup_grub_probe()
{
  for (const auto& m : temp_mounts)
  {
    log_info(std::format("Unmounting: {}", m));

    ChRootCmd cmd{std::format("umount -f {}", m)};
    cmd.execute();
  }
}


// shell
bool Install::shell()
{
  using namespace std::string_view_literals;

  const auto created_user = Widgets::accounts()->user_username();
  const auto user = created_user.empty() ? "root" : created_user;
  
  // Packages permits multiple shells, but the UI and this function only installs one
  const auto selected_shells = Packages::shells();

  if (selected_shells.contains(Package{"bash"sv}))
  {
    log_info("bash selected, which is the default. Nothing to do");
  }
  else if (!selected_shells.empty()) // sanity
  {
    const auto shell_name = selected_shells.cbegin()->name.toStdString();

    log_info(std::format("Installing {}", shell_name));

    if (pacman_install(selected_shells))
    {
      GetShellPath get_cmd{shell_name};
      if (const auto path = get_cmd.get_path(); path.empty())
        log_warning("Failed to get path for installed shell, can't change your shell. But it is installed");
      else
      {
        log_info(std::format("Changing shell for {} to {}", user, path.string()));

        SetShell set_cmd{path, user};
        if (set_cmd.execute() != CmdSuccess)
          log_warning("Failed to change shell, but it is installed.");
      }
    }
  }
  
  // can fail, but not a reason to stop. 
  return true; 
}


bool Install::gpu()
{
  log_info(std::format("Installing video packages"));

  if (const auto packages = Packages::video(); !pacman_install(packages))
  {
    log_critical("Video packages install failed");
  }

  return true;
}


// profile
bool Install::profile()
{
  const auto [profile_name, greeter_name] = Widgets::profile()->get_data();
  const auto profile_packages = Packages::profile();
      
  log_info(std::format("Applying profile {}", profile_name.toStdString()));
 
  if (pacman_install(profile_packages))
  {
    // copy arch background
    std::error_code ec;
    fs::create_directories(RootMnt / "usr/share/backgrounds", ec);
    fs::copy_file("/root/wallpapers/bg.jpg", fs::path{RootMnt / "usr/share/backgrounds/arch.png"}, ec);


    const auto& profile = Profiles::get_profile(profile_name);
    
    run_sys_commands(profile.system_commands);
    run_user_commands(profile.user_commands);

    // greeter, if any
    if (Packages::greeter().empty())
      log_info("No greeter to install");
    else if (pacman_install(Packages::greeter()))
    {
      log_info (std::format("Installing packages for greeter {}", greeter_name.toStdString()));

      run_sys_commands(Profiles::get_greeter(greeter_name).system_commands);
      run_user_commands(Profiles::get_greeter(greeter_name).user_commands);
    }
    else
      log_warning("Greeter failed to install, you will likely be in tty");
  }

  return true;
}


void Install::run_sys_commands(const QStringList& commands)
{
  log_info(std::format("Executing {} system commands", commands.size()));

  ChRootCmd chroot{commands};
  if (chroot.execute() != CmdSuccess)
  {
    log_critical("System command failed");
  }
}


void Install::run_user_commands(const QStringList& commands)
{
  const auto username = Widgets::accounts()->user_username();

  if (!username.empty() && !commands.empty())
  {
    log_info("Executing user commands");

    ChRootCmd chroot{commands, username};
    
    if (chroot.execute() != CmdSuccess)
    //if (!chroot.execute_commands(username, commands))
    {
      log_warning("At least one user command failed");
    }
  }
}


// additional packages
bool Install::packages()
{
  const auto& additional = Packages::additional();

  log_info(std::format("Installing {} additional packages", additional.size()));

  pacman_install(additional);

  // failing to install additional is not a reason to prevent working system
  return true;
}


// useful
bool Install::enable_service(const std::string_view name)
{
  log_info(std::format("Enabling service {}", name));

  ChRootCmd cmd{std::format("systemctl enable {}", name)};
  
  const int r = cmd.execute();  
  if (r != CmdSuccess)
    log_critical("Failed to enable service");
  
  return r == CmdSuccess;
}


bool Install::copy_files(const fs::path& src, const fs::path& dest, const std::vector<std::string_view>& extensions)
{
  auto has_ext = [&extensions](const fs::path& src_ext)
  {
    return std::any_of(extensions.cbegin(), extensions.cend(), [src_ext](const auto ext){ return src_ext == ext; });
  };


  std::error_code ec;
  // if directory already exists, create_directories() returns false, which is not an error condition
  if (fs::create_directories(dest, ec); !fs::exists(dest))
  {
    log_critical(std::format("{} did not exist and create failed: {}", dest.string(), ec.message()));
    return false;
  }
  else
  {
    bool ok = true;

    for (const auto& entry : fs::directory_iterator{src})
    {
      if (entry.is_regular_file() && has_ext(entry.path().extension()))
      {
        const auto filename = entry.path().filename();
        const auto dest_full_path = dest / filename;
        
        log_info(std::format("Copy {} to {}", entry.path().string(), dest_full_path.string()));

        if (fs::copy_file(entry.path(), dest_full_path, fs::copy_options::overwrite_existing, ec); ec)
        {
          qCritical() << ec.message();
          ok = false;
          break;
        }
      }
    }
    
    // if error, remove the dest dir and files contained within so we don't have partial configuration
    if (!ok)
      fs::remove_all(dest, ec);

    return ok;
  }
}


bool Install::pacman_install(const PackageSet& packages)
{
  if (packages.empty())
  {
    log_info("PackageSet is empty");
    return true;
  }

  log_info(std::format("Installing {} packages", packages.size()));

  std::stringstream ss;
  ss << "pacman -S --noconfirm " << packages;

  const auto install_cmd = ss.str();

  qInfo() << install_cmd;

  ChRootCmd install {install_cmd, [this](const std::string_view out)
  {
    log_info(out);
  }};

  if (const int r = install.execute(); r != CmdSuccess)
  {
    log_critical(std::format("pacman install of packages failed with code: {}", r));
    return false;
  }

  return true;
}


bool Install::pacman_install(const QStringList& packages)
{
  PackageSet ps;
  for (const auto& p : packages)
    ps.emplace(Package{p.toStdString()});
  
  return pacman_install(ps);
}