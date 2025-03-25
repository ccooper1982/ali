#include <ali/install.hpp>
#include <ali/util.hpp>
#include <ali/commands.hpp>
#include <ali/widgets/widgets.hpp>
#include <sstream>
#include <string>
#include <fstream>
#include <sys/mount.h>
#include <QDebug>


// The UI uses Qt which has its own types, including QString, QChar, etc. The install process
// involves calling C functions. It's tidier to use std types, and only convert to QString
// when calling the progress handler.


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

  // minimal: mount, pacman_strap, fstab, bootloader
  //          required for bootable system, even if in < ideal state.

  // locale(timezone, locale.conf, keymap for vconsole, clock sync)
  // network conf
  
  bool minimal = false;
  try
  {
    // minimal = exec_stage(&Install::filesystems, "filesystems");
    minimal = exec_stage(&Install::filesystems, "filesystems") &&
              exec_stage(&Install::mount, "mount") &&
              exec_stage(&Install::pacman_strap, "pacstrap") &&
              exec_stage(&Install::fstab, "fstab") &&
              exec_stage(&Install::root_account, "root account") &&
              exec_stage(&Install::user_account, "user account") &&
              exec_stage(&Install::boot_loader, "bootloader");

    emit on_complete(minimal);
  }
  catch(const std::exception& e)
  {
    log_critical(e.what());
  }
  catch (...)
  {
    log_critical("Install::install() encountered an unknown exception");
  }

  return minimal;
}


// filesystems
bool Install::filesystems()
{
  const auto [valid, mounts] = Widgets::partitions()->get_data();

  // sanity: UI should prevent this
  if (!valid)
  {
    qCritical("Mounts are invalid");
    return false;
  }

  if (mounts.root.create_fs)
  {
    if (!create_filesystem(mounts.root.dev, mounts.root.fs))
      return false;
  }

  if (mounts.boot.create_fs)
  {
    if (!create_filesystem(mounts.boot.dev, mounts.boot.fs))
      return false;
  }

  return true;
}


bool Install::create_filesystem(const std::string_view dev, const std::string_view fs)
{
  qDebug() << "Enter";

  log(std::format("Creating {} on {}", fs, dev));

  bool created{true};
  int res{0};

  if (fs == "ext4")
  {
    CreateExt4 cmd{dev};
    
    if (res = cmd.execute() ; res != CmdSuccess)
      created = false;
  }
  else if (fs == "vfat")
  {
    CreateFat32 cmd{dev};
    
    if (res = cmd.execute() ; res != CmdSuccess)
      created = false;
  }

  if (!created)
    qCritical() << "Failed to create filesystem: " << strerror(res);

  qDebug() << "Leave";

  return created;
}


// mounting
bool Install::mount()
{
  qDebug() << "Enter";

  bool mounted_root{false}, mounted_boot{false};

  const auto [valid, mount_data] = Widgets::partitions()->get_data();

  if (!valid)
    log_critical("Could not get the partitions paths and filesystem");
  else
  {
    if (is_dir_mounted(BootMnt.string()))
    {
      log(std::format("{} is already mounted, unmounting", BootMnt.c_str()));
      ::umount(BootMnt.c_str());
    }

    if (is_dir_mounted(RootMnt.string()))
    {
      log(std::format("{} is already mounted, unmounting", RootMnt.c_str()));
      ::umount(RootMnt.c_str());
    }

    mounted_root = do_mount(mount_data.root.dev, RootMnt.c_str(), mount_data.root.fs);  // TODO: get fs from widget
    mounted_boot = do_mount(mount_data.boot.dev, BootMnt.c_str(), mount_data.boot.fs);
    
    if (mounted_root)
      log(std::format("Mounted {} -> {}", RootMnt.c_str(), mount_data.root.dev));

    if (mounted_boot)
      log(std::format("Mounted {} -> {}", BootMnt.c_str(), mount_data.boot.dev));
  }

  qDebug() << "Leave";

  return mounted_root && mounted_boot;
}


bool Install::do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs)
{
  qDebug() << "Enter";

  if (!fs::exists(path))
    fs::create_directory(path);

  const int r = ::mount(dev.data(), path.data(), fs.data(), 0, nullptr) ;
  
  if (r != 0)
    log_critical(std::format("do_mount(): {} {}", path, ::strerror(errno)));

  qDebug() << "Leaving";

  return r == 0;    
}


// pacman
bool Install::pacman_strap()
{
  qDebug() << "Enter";

  auto create_cmd_string = [](const PackageData& data)
  {
    // pacstrap -K <root_mount> <package_list>

    std::stringstream cmd_string;

    auto append = [&cmd_string](const QSet<QString> packages)
    {
      for (const auto& p : packages)
        cmd_string << ' ' << p.toLatin1().constData() ;
    };
    
    cmd_string << "pacstrap -K " << RootMnt.string();
    
    append(data.required);
    append(data.kernels);
    append(data.firmware);

    return cmd_string.str();
  };

  
  const PackageData data = Widgets::packages()->get_data();
  const auto cmd_string = create_cmd_string(data);

  qInfo() << "Calling: " << cmd_string;

  // intercept missing firmware from pacstrap
  bool firmware_warning = false;
  Command pacstrap {cmd_string, [&firmware_warning](const std::string_view out)
  {    
    qInfo() << out;
    
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
  else
    log("Done");


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
  
  return ok;

  qDebug() << "Leave";
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

  // TODO: systemd

  ChRootCmd install {"pacman -Sy --noconfirm grub efibootmgr", [](const std::string_view out)
  {
    std::cout << out; 
  }};

  if (install.execute() != CmdSuccess)
  {
    log_critical("ERROR: bootloader install failed");
  }
  else
  {
    const std::string cmd_init = std::format("grub-install --target=x86_64-efi --efi-directory=/boot --bootloader-id=GRUB");
    
    ChRootCmd grub_init{cmd_init, [](const std::string_view out)
    {
      qInfo() << out;
    }};

    if (grub_init.execute() != CmdSuccess)
    {
      log_critical("ERROR: GRUB initialise failed");
    }
    else
    {
      ChRootCmd grub_config{"grub-mkconfig -o /boot/grub/grub.cfg", [](const std::string_view out)
      {
        std::cout << out;
      }};

      if (grub_config.execute() != CmdSuccess)
        log_critical("ERROR: GRUB config failed");
      else
        ok = true;
    }
  }

  qDebug() << "Leave";

  return ok;
}


bool Install::localise()
{
  return false;
}