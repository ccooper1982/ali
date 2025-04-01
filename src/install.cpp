#include <ali/install.hpp>
#include <ali/disk_utils.hpp>
#include <ali/locale_utils.hpp>
#include <ali/packages.hpp>
#include <ali/widgets/widgets.hpp>
#include <sstream>
#include <string>
#include <fstream>
#include <sys/mount.h>
#include <QDebug>

static const fs::path TmpMountPath {"/tmp/ali/mnt"};
static std::vector<std::string> temp_mounts;



void Install::log(const std::string_view msg)
{
  emit on_log(QString::fromLocal8Bit(msg.data(), msg.size()));
}


void Install::log_stage_start(const std::string_view msg)
{
  emit on_stage_start(QString::fromLocal8Bit(msg.data(), msg.size()));
}


void Install::log_stage_end(const std::string_view msg)
{
  emit on_stage_end(QString::fromLocal8Bit(msg.data(), msg.size()));
}


void Install::log_critical(const std::string_view msg)
{
  qCritical() << msg;
  log(msg);
}


bool Install::install ()
{
  auto exec_stage = [this](std::function<bool(Install&)> f, const std::string_view stage) mutable
  {
    log_stage_start(std::format("{} - Start", stage));
    
    const bool ok = f(std::ref(*this));

    log_stage_end(std::format("{} - {}", stage, ok ? "Success" : "Fail"));
    return ok;
  };


  bool done = false;
  try
  {
    done =  exec_stage(&Install::filesystems, "filesystems") &&
            exec_stage(&Install::mount, "mount") &&
            exec_stage(&Install::pacman_strap, "pacstrap") &&
            exec_stage(&Install::fstab, "fstab") &&
            exec_stage(&Install::localise, "locale") &&
            exec_stage(&Install::network, "network") &&
            exec_stage(&Install::root_account, "root account") &&
            exec_stage(&Install::user_account, "user account") &&
            exec_stage(&Install::boot_loader, "bootloader");

    emit on_complete(done);
  }
  catch(const std::exception& e)
  {
    log_critical(e.what());
  }
  catch (...)
  {
    log_critical("Install encountered an unknown exception");
  }

  return done;
}


// filesystems
bool Install::filesystems()
{
  const auto [valid, mounts] = Widgets::partitions()->get_data();

  // sanity: UI should prevent this
  if (!valid)
  {
    log_critical("Mounts are invalid");
    return false;
  }

  log("Setting root partition type");
  set_partition_type<SetPartitionAsLinuxRoot>(mounts.root.dev);
  
  log("Setting boot partition type");
  set_partition_type<SetPartitionAsEfi>(mounts.boot.dev);

  if (mounts.root.create_fs && !create_filesystem(mounts.root.dev, mounts.root.fs))
    return false;

  if (mounts.boot.create_fs && !create_filesystem(mounts.boot.dev, mounts.boot.fs))
    return false;

  // the UI should ensure create_fs is false if home uses root, but sanity check here
  if (mounts.home.dev != mounts.root.dev && mounts.home.create_fs)
  {
    log("Setting home partition type");
    set_partition_type<SetPartitionAsLinuxHome>(mounts.home.dev);

    if (!create_filesystem(mounts.home.dev, mounts.home.fs))
      return false;
  }

  return true;
}


bool Install::create_filesystem(const std::string_view part_dev, const std::string_view fs)
{
  qDebug() << "Enter";

  log(std::format("Creating {} on {}", fs, part_dev));

  int res{0};
  
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

  const bool created = res == CmdSuccess;

  if (!created)
    qCritical() << "Failed to create filesystem: " << strerror(res);

  qDebug() << "Leave";

  return created;
}


// mounting
bool Install::mount()
{
  qDebug() << "Enter";

  bool mounted_root{false}, mounted_boot{false}, mounted_home{true};

  const auto [valid, mount_data] = Widgets::partitions()->get_data();

  if (!valid)
    log_critical("Could not get partition paths and filesystems");
  else
  {
    // TODO this only checks if path is mounted (i.e. /mnt/boot), should it also 
    //      check if device is mounted (i.e. /dev/sda2)? If the device is mounted
    //      elsewhere, we should fail.

    if (PartitionUtils::is_path_mounted(BootMnt.string()))
    {
      log(std::format("{} is already mounted, unmounting", BootMnt.c_str()));
      ::umount(BootMnt.c_str());
    }

    if (PartitionUtils::is_path_mounted(RootMnt.string()))
    {
      log(std::format("{} is already mounted, unmounting", RootMnt.c_str()));
      ::umount(RootMnt.c_str());
    }

    if (PartitionUtils::is_path_mounted(HomeMnt.string()))
    {
      log(std::format("{} is already mounted, unmounting", HomeMnt.c_str()));
      ::umount(RootMnt.c_str());
    }

    mounted_root = do_mount(mount_data.root.dev, RootMnt.c_str(), mount_data.root.fs);
    mounted_boot = do_mount(mount_data.boot.dev, BootMnt.c_str(), mount_data.boot.fs);

    log(std::format("Mount of {} -> {} : {}", RootMnt.c_str(), mount_data.root.dev, mounted_root ? "Success" : "Fail"));
    log(std::format("Mount of {} -> {} : {}", BootMnt.c_str(), mount_data.boot.dev, mounted_boot ? "Success" : "Fail"));

    if (mount_data.home.dev != mount_data.root.dev)
    {
      mounted_home = do_mount(mount_data.home.dev, HomeMnt.c_str(), mount_data.home.fs);
      log(std::format("Mount of {} -> {} : {}", HomeMnt.c_str(), mount_data.home.dev, mounted_home ? "Success" : "Fail"));
    }
  }

  qDebug() << "Leave";

  return mounted_root && mounted_boot && mounted_home;
}


bool Install::do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs, const bool read_only)
{
  qDebug() << "Enter";

  if (!fs::exists(path))
    fs::create_directory(path);

  const int r = ::mount(dev.data(), path.data(), fs.data(), read_only ? MS_RDONLY : 0, nullptr) ;
  
  if (r != 0)
    log_critical(std::format("do_mount(): {} {}", path, ::strerror(errno)));

  qDebug() << "Leaving";

  return r == 0;    
}


// pacstrap
bool Install::pacman_strap()
{
  qDebug() << "Enter";

  auto create_cmd_string = []()
  {
    // pacstrap -K <root_mount> <package_list>

    std::stringstream cmd_string;
    cmd_string << "pacstrap -K " << RootMnt.string() << ' ';
    cmd_string << Packages::required();
    cmd_string << Packages::kernels();
    cmd_string << Packages::firmware();

    return cmd_string.str();
  };

  
  const auto cmd_string = create_cmd_string();

  qInfo() << cmd_string;
  log(cmd_string);
  
  // intercept missing firmware message from pacstrap
  
  bool firmware_warning = false;
  Command pacstrap {cmd_string, [this, &firmware_warning](const std::string_view out)
  {    
    qInfo() << out;
    log(out);
    
    if (out.find("Possibly missing firmware for module:") != std::string::npos)
      firmware_warning = true;
  }};
  
  bool ok = true;
  if (pacstrap.execute() != CmdSuccess)
  {
    log_critical("ERROR: pacstrap failed - manual intervention required");
    ok = false;
  }
  else if (firmware_warning)
    log("NOTE: warnings of missing firmware. This can be fixed post-install");

  qDebug() << "Leave";

  return ok;
}


// fstab
bool Install::fstab()
{
  qDebug() << "Enter";

  fs::create_directory((RootMnt / FsTabPath).parent_path());

  Command fstab {std::format("genfstab -U {} > {}", RootMnt.string(), FsTabPath.string())};

  const bool ok = fstab.execute() == CmdSuccess && fs::exists(FsTabPath) && fs::file_size(FsTabPath);
  
  if (!ok)
    log_critical("fstab failed");
  
  qDebug() << "Leave";

  return ok;
}


// locale
bool Install::localise()
{
  const auto locale_data = Widgets::start()->get_data();

  log(std::format("Setting keymap {}", locale_data.keymap));
  if (!LocaleUtils::generate_keymap(locale_data.keymap))
  {
    log_critical("Setting key map failed"); // not actually critital, but no "log_error()" yet
  }

  log("Generating locales");
  if (!LocaleUtils::generate_locale(locale_data.locales, locale_data.locales[0]))
  {
    log_critical("Generating/setting locales failed");
  }

  log("Setting timezone");
  if (!LocaleUtils::generate_timezone(locale_data.timezone))
  {
    log_critical("Setting timezone failed");
  }

  // locale not considered essential, more of an annoyance if it fails
  return true; 
}


// network
bool Install::network()
{
  // This assumes `iwd` is the only network manager. The UI should warn
  // if NetworkManager is installed.

  const auto data = Widgets::network()->get_data();

  ChRootCmd hostname_cmd{std::format("echo \"{}\" > /etc/hostname", data.hostname)};
  if (hostname_cmd.execute() != CmdSuccess)
    log("Failed to set /etc/hostname");

  // intentionally continuing if hostname is unset
  if (data.copy_config)
  {
    // iwd config: https://wiki.archlinux.org/title/Iwd#Network_configuration
    static const fs::path iwd_config_src {"/var/lib/iwd"};
    static const fs::path iwd_config_dest {RootMnt / "var/lib/iwd"};
    static const fs::path sysd_config_src {"/etc/systemd/network"};
    static const fs::path sysd_config_dest {RootMnt / "etc/systemd/network"};

    
    // iwd
    if (copy_files(iwd_config_src, iwd_config_dest, {".psk", ".open", ".8021x"}))
      enable_service("iwd.service");
    else
      log_critical("Failed to configure iwd");  

    // systemd-networkd
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
  qDebug() << "Enter";

  const std::string root_passwd = Widgets::accounts()->root_password();

  // sanity check: UI should prevent this
  if (root_passwd.empty())
  {
    log_critical("Root password empty");
    return false;
  }

  const bool pass_set = set_password("root", root_passwd);
  
  qDebug() << "Leave";

  return pass_set ;
}


bool Install::user_account()
{
  qDebug() << "Enter";

  const std::string username = Widgets::accounts()->user_username();
  const std::string password = Widgets::accounts()->user_password();

  bool user_created{false};

  // sanity check: username and password should be validated by UI
  if (username.empty())
  {
    user_created = true;
    log("No user to create");
  }
  else if (!username.empty() && password.empty())
  {
    user_created = false;
    log_critical("Have username but not password");
  }
  else
  {
    const bool can_sudo = Widgets::accounts()->user_is_sudo();
    const std::string wheel_group = can_sudo ? "-G wheel" : "";

    // use the `useradd` command, without the password, set that after with passwd
    //  -s shell
    //  -m create home directory
    //  -G wheel (if sudo permitted)
    ChRootCmd useradd{std::format("useradd -s /usr/bin/bash -m {} {}", wheel_group, username)} ;

    if (const int r = useradd.execute(); r != CmdSuccess)
    {
      log(std::format("User created failed: {}", strerror(r)));
    }
    else
    {
      if (!set_password(username, password))
        log_critical("Failed to set user password");
      else if (can_sudo)      
      {
        if (!add_to_sudoers(username))
          log_critical("Failed to add user to sudoers");
        else
          user_created = true;
      }
      else
        user_created = true;
    }
  }
  
  qDebug() << "Leave";
  return user_created;
}


bool Install::add_to_sudoers(const std::string& user)
{
  using namespace std::string_literals;

  static const fs::perms SudoersFilePerms = fs::perms::owner_read | fs::perms::group_read;
  static const fs::path SudoersD = RootMnt / "etc/sudoers.d";

  log("Adding user to sudoers");

  std::error_code ec;
  if (fs::create_directories(SudoersD, ec); ec.value() != 0)
  {
    log(std::format("Failed to add user to sudoers: {}", strerror(ec.value())));
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
      log(std::format("Failed to add user to sudoers: {}", strerror(ec.value())));
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
  qDebug() << "Enter";
  log(std::format("Setting password for {}", user));

  bool pass_set{false}, pass_valid{false};

  ChRootCmd cmd_passwd{"chpasswd"};
  if (cmd_passwd.execute_write(std::format("{}:{}", user, pass)) == CmdSuccess)
  {
    pass_set = true;

    ChRootCmd cmd_check{std::format("passwd -S {}", user), [&user, &pass_valid](const std::string_view out)
    {
      if (out.size() > user.size()) // at least "<user> P"
      {
        if (const auto pos = out.find(' '); pos != std::string_view::npos)
          pass_valid = out.substr(pos+1, 1) == "P";
      }
    }};

    cmd_check.execute(); // if this fails, pass_valid remains false
  }
  
  qDebug() << "Leave";

  return pass_set && pass_valid;
}


// bootloader
bool Install::boot_loader()
{
  qDebug() << "Enter";

  bool ok = false;
  int r = 0;

  // TODO: systemd-boot

  ChRootCmd install {"pacman -Sy --noconfirm grub efibootmgr os-prober", [this](const std::string_view out)
  {
    qInfo() << out;
    log(out);
  }};

  if (r = install.execute(); r != CmdSuccess)
  {
    log_critical(std::format("pacman install of grub and efibootmgr: {}", strerror(r)));
  }
  else
  {
    const std::string cmd_init = std::format("grub-install --target=x86_64-efi --efi-directory=/boot --bootloader-id=GRUB");
    
    ChRootCmd grub_install{cmd_init, [this](const std::string_view out)
    {
      qInfo() << out;
      log(out);
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
      
      ChRootCmd grub_config{"grub-mkconfig -o /boot/grub/grub.cfg", [this](const std::string_view out)
      {
        qInfo() << out;
        log(out);
      }};

      if (r = grub_config.execute(); r != CmdSuccess)
        log_critical(std::format("grub-mkconfig failed: {}", strerror(r)));
      else
        ok = true;
    
      
      cleanup_grub_probe();
    }
  }

  qDebug() << "Leave";

  return ok;
}


bool Install::prepare_grub_probe()
{
  log("Preparing for GRUB os-probe");

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
    log_critical("Failed to update GRUB config file"); // TODO need log_warning()
  }

  if (updated_cfg)
  {
    log ("Mounting other partitions");

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
        // partition can't contain OS if: it's EFI; has no filesystem;
        // and we don't want mount any of the partitions of the installed Arch
        if (!part.is_efi && !part.fs_type.empty() &&  part.dev != mount_data.boot.dev &&
                                                      part.dev != mount_data.root.dev && 
                                                      part.dev != mount_data.home.dev)
        {
          const auto fs = part.fs_type == "ntfs" ? "ntfs3" : part.fs_type; // TODO unreliable hack?

          const auto mount_point = std::format("{}/{}", TmpMountPath.string(), i++);
          const auto mount_cmd = std::format("mount --mkdir --read-only --target-prefix {} -t {} {} {}", RootMnt.string(), fs, part.dev, mount_point);
          
          if (Command cmd{mount_cmd}; cmd.execute() != CmdSuccess)
          {
            log_critical(std::format("Failed to mount {} -> {}", part.dev, mount_point)); // TODO need log_warning()
            mounted = false;
            break;
          }
          else
          {
            log(std::format("Mounted {} -> {}", part.dev, mount_point));
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
    const auto path = std::format("{}{}", RootMnt.string(), m);

    log(std::format("Unmounting: {}", path));

    Command cmd{std::format("umount -f {}", path)};
    cmd.execute();
  }
}


// services
bool Install::enable_service(const std::string_view name)
{
  log(std::format("Enabling service {}", name));

  ChRootCmd cmd{std::format("systemctl enable {}", name)};
  
  const int r = cmd.execute();  
  if (r != CmdSuccess)
    log_critical("Failed to enable service");
  
  return r == CmdSuccess;
}


// useful
bool Install::copy_files(const fs::path& src, const fs::path& dest, const std::vector<std::string_view>& extensions)
{
  auto is_ext = [&extensions](const fs::path& src_ext)
  {
    return std::any_of(extensions.cbegin(), extensions.cend(), [src_ext](const auto ext){ return src_ext == ext; });
  };


  std::error_code ec;
  // if directories already exists, create_directories() returns false,
  // which is not an error condition
  if (fs::create_directories(dest, ec); !fs::exists(dest))
  {
    log_critical(std::format("{} does not exist and it failed to create: {}", dest.string(), ec.message()));
    return false;
  }
  else
  {
    bool ok = true;

    for(const auto& entry : fs::directory_iterator{src})
    {
      if (entry.is_regular_file() && is_ext(entry.path().extension()))
      {
        if (fs::copy(entry.path(), dest, ec); ec)
        {
          ok = false;
          break;
        }
      }
    }
    
    // remove any files copied if we have error, so we don't have partial configuration
    if (!ok)
      fs::remove_all(dest, ec);

    return ok;
  }
}