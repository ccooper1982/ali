#include <ali/install.hpp>
#include <ali/util.hpp>
#include <ali/commands.hpp>
#include <ali/widgets/widgets.hpp>
#include <sstream>
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



bool Install::install ()
{
  auto exec_stage = [this](std::function<bool(Install&)> f, const std::string_view stage) mutable
  {
    log_stage_start(std::format("Stage: {} - Start", stage));
    
    const bool ok = f(std::ref(*this));

    log_stage_end(std::format("Stage: {} - {}", stage, ok ? "Success" : "Fail"));
    return ok;
  };

  // mount, pacman_strap, fstab
  // locale(timezone, locale.conf, keymap for vconsole), clock sync, 
  // network conf, passwords, bootloader
  
  bool minimal = false;
  try
  {
    minimal = exec_stage(&Install::mount, "mount") &&
              exec_stage(&Install::pacman_strap, "pacstrap") &&
              exec_stage(&Install::fstab, "fstab") &&
              exec_stage(&Install::passwords, "passwords") &&
              exec_stage(&Install::boot_loader, "bootloader");

    emit on_complete(minimal);
  }
  catch(const std::exception& e)
  {
    qCritical() << e.what();
    log(e.what());
  }
  catch (...)
  {
    qCritical() << "Install::install() encountered an unknown exception";
    log("Install::install() encountered an unknown exception");
  }

  return minimal;
}


// mounting
bool Install::do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs)
{
  qDebug() << "Enter";

  if (!fs::exists(path))
    fs::create_directory(path);

  const int r = ::mount(dev.data(), path.data(), fs.data(), 0, nullptr) ;
  
  if (r != 0)
    qCritical() << path << " : " << ::strerror(errno) << '\n';

  qDebug() << "Leaving";

  return r == 0;    
}


bool Install::mount()
{
  qDebug() << "Enter";

  const PartitionData parts_data = Widgets::partitions()->get_data();
  
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

  const bool mounted_root = do_mount(parts_data.root, RootMnt.c_str(), "ext4");  // TODO: get fs from widget
  const bool mounted_boot = do_mount(parts_data.boot, BootMnt.c_str(), "vfat");
  
  if (mounted_root)
    log(std::format("Mounted {} -> {}", RootMnt.c_str(), parts_data.root));

  if (mounted_boot)
    log(std::format("Mounted {} -> {}", BootMnt.c_str(), parts_data.boot));

  qDebug() << "Leave";

  return mounted_root && mounted_boot;
}


// pacman
bool Install::pacman_strap()
{
  qDebug() << "Enter";

  auto create_cmd_string = [](const PackageData& data)
  {
    // pacstrap -K <root_partition> <package_list>

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
    qCritical() << "pacstrap failed";
    log("ERROR: pacstrap failed - manual intervention required");
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
  {
    qCritical() << "fstab failed";
    log("ERROR: fstab failed - manual intervention required");
  }
  
  return ok;

  qDebug() << "Leave";
}


// accounts and passwords
bool Install::passwords()
{
  qDebug() << "Enter";

  std::string root_passwd = "arch"; // TODO
  
  ChRootCmd cmd_passwd{"passwd --stdin"};
  cmd_passwd.execute_write(root_passwd);

  bool pass_set{false};
  ChRootCmd cmd_check{"passwd -S", [&pass_set](const std::string_view out)
  {
    if (out.size() > 6) // at least "root P"
    {
      if (const auto pos = out.find(' '); pos != std::string_view::npos)
        pass_set = out.substr(pos+1, 1) == "P";
    }
  }};
  cmd_check.execute();

  qDebug() << "Leave";

  return pass_set;
}


// bootloader
bool Install::boot_loader()
{
  qDebug() << "Enter";

  bool ok = false;

  // TODO (grub, efibootmgr) or (systemd)

  ChRootCmd install {"pacman -Sy --noconfirm grub efibootmgr", [](const std::string_view out)
  {
    std::cout << out; 
  }};

  if (install.execute() != CmdSuccess)
  {
    qCritical() << "ERROR: bootloader install failed";
    log("ERROR: bootloader install failed");
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
      qCritical() << "ERROR: GRUB initialise failed";
      log("ERROR: GRUB initialise failed");
    }
    else
    {
      ChRootCmd grub_config{"grub-mkconfig -o /boot/grub/grub.cfg", [](const std::string_view out)
      {
        std::cout << out;
      }};

      if (grub_config.execute() != CmdSuccess)
      {
        qCritical() << "ERROR: GRUB config failed";
        log("ERROR: GRUB config failed");
      }        
      else
        ok = true;
    }
  }

  qDebug() << "Leave";

  return ok;
}