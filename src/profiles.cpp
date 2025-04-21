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
std::map<QString, Profile> Profiles::m_greeters;


static const fs::path ProfilesPath = "profiles";
static const fs::path DesktopProfilesDir = ProfilesPath / "desktop";
static const fs::path TtyProfilesDir = ProfilesPath / "tty";
static const fs::path GreetersProfilesPath = ProfilesPath / "greeters.json";

bool Profiles::read()
{
  const fs::path base_dir = fs::current_path();

  qInfo() << "Profiles location: " << (base_dir / ProfilesPath).string();
  
  return read_profiles(base_dir / DesktopProfilesDir, m_desktop_profiles) &&
         read_profiles(base_dir / TtyProfilesDir, m_tty_profiles) && 
         read_greeters(base_dir / GreetersProfilesPath);
}


bool Profiles::read_profiles(const fs::path& dir, ProfilesMap& profiles)
{
  for (const fs::path& entry : fs::directory_iterator{dir})
  { 
    if (!(fs::is_regular_file(entry) && entry.extension() == ".json"))
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

  auto is_valid = validate(root, "packages", QJsonValue::Array) &&
                  validate(root, "system_commands", QJsonValue::Array) &&
                  validate(root, "user_commands", QJsonValue::Array) &&
                  validate(root, "name", QJsonValue::String);

  if (!is_valid)
    return false;

  Profile profile;
  profile.is_tty = std::addressof(profiles) == std::addressof(m_tty_profiles);
  profile.name = root["name"].toString();

  is_valid = to_stringlist(root["packages"].toArray(), profile.packages) && 
             to_stringlist(root["system_commands"].toArray(), profile.system_commands) &&
             to_stringlist(root["user_commands"].toArray(), profile.user_commands);

  if (is_valid)
    profiles.emplace(root["name"].toString(), std::move(profile));

  return is_valid;
}


bool Profiles::read_greeters (const fs::path& path)
{
  if (!(fs::is_regular_file(path) && path.extension() == ".json"))
    return false;

  bool ok {false};

  if (QFile file {path}; file.open(QIODevice::ReadOnly))
  {
    if (const auto doc = QJsonDocument::fromJson(file.readAll()); doc.isNull())
    {
      qCritical() << "Profile JSON invalid: " << path.string();
    }
    else if (!doc.isArray())
    {
      qCritical() << "greeters file root is not an array";
    }
    else
    {
      const auto root = doc.array();

      for (const auto& entry : root)
      {
        if (!entry.isObject())
        {
          qCritical() << "Must be an object";
          continue;
        }

        const auto& greeter = entry.toObject();
        
        if (validate(greeter, "name", QJsonValue::String) &&
            validate(greeter, "tty", QJsonValue::Bool) &&
            validate(greeter, "packages", QJsonValue::Array) &&
            validate(greeter, "system_commands", QJsonValue::Array) &&
            validate(greeter, "user_commands", QJsonValue::Array, false))
        {
          Profile p {.name = greeter["name"].toString(), .is_tty = greeter["tty"].toBool()};
          
          ok = to_stringlist(greeter["packages"].toArray(), p.packages) &&
               to_stringlist(greeter["system_commands"].toArray(), p.system_commands);

          if (ok && greeter.contains("user_commands"))
            ok = to_stringlist(greeter["user_commands"].toArray(), p.user_commands);

          if (ok)
            m_greeters.emplace(greeter["name"].toString(), std::move(p));
        }
      }
    }
  }

  return ok;
}


bool Profiles::validate(const QJsonObject& root, const QString& key, const QJsonValue::Type t, const bool required)
{
  if (required)
    return root.contains(key) && root[key].type() == t;
  else if (!root.contains(key))
    return true;
  else
    return root[key].type() == t;
}


bool Profiles::to_stringlist(const QJsonArray& arr, QStringList& dest)
{
  for (const auto& v : arr)
  {
    if (!v.isString())
      return false;

    dest.append(v.toString());
  }
  return true;
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


QStringList Profiles::get_greeter_names(const bool tty)
{
  QStringList list;

  for(const auto& [name, profile] : m_greeters)
  {
    if (profile.is_tty == tty)
      list.append(name);
  }

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


const Profile& Profiles::get_greeter(const QString& name)
{
  if (m_greeters.contains(name))
    return m_greeters.at(name);
  else
    throw std::runtime_error{std::format("Greeter {} does not exist", name.toStdString())};
}