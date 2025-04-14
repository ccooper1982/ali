#ifndef ALI_PROFILEDATA_H
#define ALI_PROFILEDATA_H

#include <ali/common.hpp>
#include <QString>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonValue>
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
  using ProfilesMap = std::map<QString, Profile>;

public:
  static bool read();
  static QStringList get_desktop_profile_names();
  static QStringList get_tty_profile_names();
  static QStringList get_greeter_names(const bool tty);

  // throws runtime_error if name does not exist
  static const Profile& get_profile (const QString& name) ;
  static const Profile& get_greeter (const QString& name) ;
  
private:
  enum class Commands { System, User };

  static bool read_profiles(const fs::path& dir, ProfilesMap& profiles);
  static bool read_profile (const fs::path path, const QJsonDocument& doc, ProfilesMap& profiles);
  static bool read_greeters (const fs::path& path);

  static bool validate(const QJsonObject& root, const QString& key, const QJsonValue::Type t, const bool required = true);
  static bool to_stringlist(const QJsonArray& arr, QStringList& dest);

private:
  static ProfilesMap m_tty_profiles;
  static ProfilesMap m_desktop_profiles;
  static ProfilesMap m_greeters;
};

#endif
