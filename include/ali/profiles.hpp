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
  QStringList commands; // commands required post-install (typically services)
  QString info; // info to user for post-install guide
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
  static bool read_profiles(const fs::path& dir, std::map<QString, Profile>& map);
  static QStringList read_packages(const fs::path& dir);
  static QStringList read_commands(const fs::path& dir);
  static QString read_info(const fs::path& dir);

private:
  static std::map<QString, Profile> m_tty_profiles;
  static std::map<QString, Profile> m_desktop_profiles;
};

#endif
