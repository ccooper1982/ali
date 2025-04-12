#ifndef ALI_PROFILEDATA_H
#define ALI_PROFILEDATA_H

#include <ali/common.hpp>
#include <QString>
#include <QStringList>
#include <map>

struct Profile
{
  QString name; // 'Minimal', 'Hyprland', etc. Taken from profile directory name
  QStringList packages; 
  QStringList system_commands; // commands to run as root
  QStringList user_commands; // commands to run as user
  QString info; // info to user for post-install guide
  bool is_tty; // true if profile is tty (no x11/wayland)
};


class Profiles
{
public:
  static bool read();
  static QStringList get_desktop_profile_names();
  static QStringList get_tty_profile_names();

  // throws runtime_error if name does not exist
  static const Profile& get_profile (const QString& name) ;
  
private:
  enum class Commands { System, User };

  static bool read_profiles(const fs::path& dir, std::map<QString, Profile>& map);
  static QStringList read_packages(const fs::path& dir);
  static QStringList read_commands(const fs::path& dir, const Commands type);
  static QString read_info(const fs::path& dir);

private:
  static std::map<QString, Profile> m_tty_profiles;
  static std::map<QString, Profile> m_desktop_profiles;
};

#endif
