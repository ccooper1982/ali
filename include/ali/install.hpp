#ifndef ALI_INSTALL_H
#define ALI_INSTALL_H

#include <functional>
#include <QString>
#include <QObject>
#include <ali/commands.hpp>

class Install : public QObject
{
  Q_OBJECT

public:
  //using ProgressHandler = std::function<void(const std::string_view&)>;

  Install() = default;
  virtual ~Install() = default;

  bool install ();

signals:
  void on_stage_start(const QString msg);
  void on_log(const QString msg);
  void on_stage_end(const QString msg);
  void on_complete(const bool);

private:
  void log(const std::string_view msg);
  void log_critical(const std::string_view msg);
  void log_stage_start(const std::string_view msg);
  void log_stage_end(const std::string_view msg);
    
  bool filesystems();
  bool create_filesystem(const std::string_view dev, const std::string_view fs);

  // TODO not convinced I like this
  template<class Cmd>
  void set_partition_type(const std::string_view part_dev)
  {
    const int part_num =  PartitionUtils::get_partition_part_number(part_dev);
    const std::string parent_dev = PartitionUtils::get_partition_parent(part_dev);

    if (!part_num || parent_dev.empty())
    {
      log("Cannot get parent device and/or partition number, cannot set partition type. Not an error");
    }
    else
    {
      if (Cmd cmd{part_num, parent_dev}; cmd.execute() != CmdSuccess)
        log(std::format("Failed to set partition type for {}. This is not an error.", part_dev));
    }
  }
  

  bool mount();
  bool do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs, const bool read_only = false);
  bool pacman_strap();
  bool fstab();
  
  bool root_account();
  bool user_account();
  bool add_to_sudoers(const std::string& user);
  bool set_password(const std::string_view user, const std::string_view pass);

  bool boot_loader();
  bool prepare_grub_probe();
  void cleanup_grub_probe();
  
  
  bool localise();
  
  bool network();

  bool enable_service(const std::string_view name);

  // utils
  bool copy_files(const fs::path& src, const fs::path& dest, const std::vector<std::string_view>& extensions);

  
};

#endif
