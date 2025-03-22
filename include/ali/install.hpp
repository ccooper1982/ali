#ifndef ALI_INSTALL_H
#define ALI_INSTALL_H

#include <functional>
#include <QString>

class Install
{
public:
  using ProgressHandler = std::function<void(const std::string_view&)>;

  Install(ProgressHandler&& log_handler);

  bool install ();

private:
  bool do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs);
  bool mount();
  
  bool pacman_strap();

private:
  ProgressHandler m_progress;
};

#endif
