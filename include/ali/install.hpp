#ifndef ALI_INSTALL_H
#define ALI_INSTALL_H

#include <functional>
#include <QString>
#include <QObject>


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
  
  bool mount();
  bool do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs);
  bool pacman_strap();
  bool fstab();
  
  bool root_account();
  bool user_account();
  bool add_to_sudoers(const std::string& user);
  bool set_password(const std::string_view user, const std::string_view pass);

  bool boot_loader();
  bool localise();

private:
  //ProgressHandler m_progress;
};

#endif
