#include <ali/install.hpp>
#include <ali/util.hpp>
#include <ali/commands.hpp>
#include <ali/widgets/widgets.hpp>
#include <sstream>
#include <QDebug>


// The UI uses Qt which has its own types, including QString, QChar, etc. The install process
// involves calling C functions. It's tidier to use std types, and only convert to QString
// when calling the progress handler.


Install::Install(ProgressHandler&& progress_handler) : m_progress(std::move(progress_handler))
{

}


bool Install::install ()
{
  if (!m_progress)
  {
    qCritical() << "Log handler not set";
    return false;
  }

  
  // mount, pacman_strap, fstab
  // locacle(timezone, locale.conf, keymap for vconsole), clock sync, 
  // network conf, passwords, bootloader
  
  bool ok = false;
  try
  {
    m_progress("Starting");
    
    ok = mount() && pacman_strap();
    
    m_progress("Complete");
  }
  catch(const std::exception& e)
  {
    qCritical() << e.what();
    m_progress(e.what());
  }
  catch (...)
  {
    qCritical() << "Install::install() encountered an unknown exception";
    m_progress("Install::install() encountered an unknown exception");
  }

  return ok;
}


// mounting
bool Install::do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs)
{
  if (!fs::exists(path))
    fs::create_directory(path);

  const int r = ::mount(dev.data(), path.data(), fs.data(), 0, nullptr) ;
  
  if (r != 0)
    qCritical() << path << " : " << ::strerror(errno) << '\n';

  return r == 0;    
}


bool Install::mount()
{
  const PartitionData parts_data = Widgets::partitions()->get_data();
  
  if (is_dir_mounted(BootMnt.string()))
  {
    m_progress(std::format("{} is already mounted, unmounting", BootMnt.c_str()));
    ::umount(BootMnt.c_str());
  }

  if (is_dir_mounted(RootMnt.string()))
  {
    m_progress(std::format("{} is already mounted, unmounting", RootMnt.c_str()));
    ::umount(RootMnt.c_str());
  }

  const bool mounted_root = do_mount(parts_data.root, RootMnt.c_str(), "ext4");  // TODO: get fs from widget
  const bool mounted_boot = do_mount(parts_data.boot, BootMnt.c_str(), "vfat");
  
  if (mounted_root)
    m_progress(std::format("Mounted {} -> {}", RootMnt.c_str(), parts_data.root));

  if (mounted_boot)
    m_progress(std::format("Mounted {} -> {}", BootMnt.c_str(), parts_data.boot));

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

  m_progress("Running pacstrap");

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
    m_progress("ERROR: pacstrap failed - manual intervention required");
    ok = false;
  }
  else if (firmware_warning)
  {
    m_progress("NOTE: warnings of missing firmware. This can be fixed post-install");
  }
  else
    m_progress("Done");


  qDebug() << "Leaving";

  return ok;
}