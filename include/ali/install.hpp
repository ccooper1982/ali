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
  void on_log(const QString msg);

private:
  void log(const std::string_view msg);

  bool do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs);
  bool mount();
  
  bool pacman_strap();

private:
  //ProgressHandler m_progress;
};

#endif
