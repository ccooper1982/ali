#include <ali/profiles.hpp>

#include <fstream>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


std::map<QString, Profile> Profiles::m_tty_profiles;
std::map<QString, Profile> Profiles::m_desktop_profiles;


static const fs::path ProfilesPath = "profiles";
static const fs::path DesktopProfilesPath = ProfilesPath / "desktop";
static const fs::path TtyProfilesPath = ProfilesPath / "tty";
static const fs::path PackagesFile = "packages";
static const fs::path SysCommandsFile = "system_commands";
static const fs::path UserCommandsFile = "user_commands";
static const fs::path InfoFile = "info";


bool Profiles::read()
{
  const fs::path base_dir = fs::current_path();

  qInfo() << "Profiles location: " << (base_dir / ProfilesPath).string();
  
  return read_profiles(base_dir / DesktopProfilesPath, m_desktop_profiles) &&
         read_profiles(base_dir / TtyProfilesPath, m_tty_profiles);
}


bool Profiles::read_profiles(const fs::path& dir, ProfilesMap& profiles)
{
  for (const fs::path& entry : fs::directory_iterator{dir})
  { 
    if (!(entry.extension() == ".jsonc" || entry.extension() == ".json"))
      continue;

    if (QFile file {entry}; file.open(QIODevice::ReadOnly))
    {
      if (const auto doc = QJsonDocument::fromJson(file.readAll()); doc.isNull())
      {
        qCritical() << "Profile JSON invalid: " << entry.string();
        return false;
      }
      else if (!read_profile(entry, doc, profiles))
      {
        qCritical () << "Profile file error. JSON is valid but structure is not: " << entry.string();
        return false;
      }
    }
  }

  return true;
}


bool Profiles::read_profile (const fs::path path, const QJsonDocument& doc, Profiles::ProfilesMap& profiles)
{
  if (!doc.isObject())
    return false;

  const auto root = doc.object();

  const auto valid = [&root](const QString& key, const QJsonValue::Type t, const bool required = true)
  {
    if (required)
      return root.contains(key) && root[key].type() == t;
    else if (root.contains(key) && root[key].type() != t)
      return false;
    else
      return true;
  };

  auto is_valid = valid("packages", QJsonValue::Array) &&
                  valid("system_commands", QJsonValue::Array) &&
                  valid("user_commands", QJsonValue::Array) &&
                  valid("name", QJsonValue::String);


  if (!is_valid)
    return false;

  Profile profile;
  profile.is_tty = std::addressof(profiles) == std::addressof(m_tty_profiles);
  profile.name = root["name"].toString();


  auto to_stringlist = [&profile](const QJsonArray& arr, QStringList& dest)
  {
    for (const auto& v : arr)
    {
      if (!v.isString())
        return false;

      dest.append(v.toString());
    }
    return true;
  };

  is_valid = to_stringlist(root["packages"].toArray(), profile.packages) && 
             to_stringlist(root["system_commands"].toArray(), profile.system_commands) &&
             to_stringlist(root["user_commands"].toArray(), profile.user_commands);

  if (is_valid)
    profiles.emplace(root["name"].toString(), std::move(profile));

  return is_valid;
}


QStringList Profiles::get_desktop_profile_names()
{
  QStringList list;

  for(const auto& [name, profile] : m_desktop_profiles)
    list.append(name);

  list.sort();
  return list;
}


QStringList Profiles::get_tty_profile_names()
{
  QStringList list;

  for(const auto& [name, profile] : m_tty_profiles)
    list.append(name);

  list.sort();
  return list;
}


const Profile& Profiles::get_profile(const QString& name)
{
  if (m_desktop_profiles.contains(name))
    return m_desktop_profiles.at(name);
  else if (m_tty_profiles.contains(name))
    return m_tty_profiles.at(name);
  else
    throw std::runtime_error{std::format("Profile {} does not exist", name.toStdString())};
}