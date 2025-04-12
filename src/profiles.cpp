#include <ali/profiles.hpp>

#include <fstream>
#include <QDebug>
#include <QFile>
#include <QTextStream>


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


bool Profiles::read_profiles(const fs::path& dir, std::map<QString, Profile>& map)
{
  qInfo() << "Reading profiles from: " << dir.string();
  
  bool done{true};

  try
  {
    for (const auto& entry : fs::directory_iterator{dir})
    {
      if (const auto path = entry.path(); entry.is_directory() && !path.empty())
      {
        const auto name = QString::fromStdString(path.stem().string());
        
        qInfo() << "Found " << name;

        Profile profile { .name = name,
                          .packages = read_packages(path),
                          .system_commands = read_commands(path, Commands::System),
                          .user_commands = read_commands(path, Commands::User),
                          .info = read_info(path),
                          .is_tty = std::addressof(map) == std::addressof(m_tty_profiles) //dir.stem() == "tty"
                        };

        map.emplace(name, std::move(profile));
      }
    }

    qInfo() << "Have " << map.size() << " profiles";
  }
  catch(const std::exception& e)
  {
    qCritical() << "ERROR: " << e.what();
    map.clear();
    done = false;
  }
  catch(...)
  {
    qCritical() << "ERROR: unknown";
    map.clear();
    done = false;
  }

  return done;
}


QStringList Profiles::read_packages(const fs::path& dir)
{
  QStringList list;

  try
  {
    if (fs::exists(dir / PackagesFile))
    { 
      if (QFile file {dir / PackagesFile}; file.open(QIODevice::ReadOnly))
      {
        QTextStream stream(&file);
        for (QString line = stream.readLine(); !line.isNull() ; line = stream.readLine())
        {
          if (!line.isEmpty() && line[0] != '#')
            list.append(line);
        }
      }
    }
  }
  catch(const std::exception& e)
  {
    qCritical() << "ERROR: " << e.what();
    list.clear();
  } 

  return list;
}


QStringList Profiles::read_commands(const fs::path& dir, const Commands type)
{
  QStringList list;

  try
  {
    const auto path = dir / (type == Commands::System ? SysCommandsFile : UserCommandsFile);

    if (fs::exists(path))
    {
      if (QFile file {path}; file.open(QIODevice::ReadOnly))
      {
        QTextStream stream(&file);
        
        for (QString line = stream.readLine(); !line.isNull() ; line = stream.readLine())
        {
          if (!line.isEmpty() && line[0] != '#')
            list.append(line);
        }
      }
    }
  }
  catch(const std::exception& e)
  {
    qCritical() << "ERROR: " << e.what();
    list.clear();
  } 

  return list;
}


QString Profiles::read_info(const fs::path& dir)
{
  try
  {
    if (fs::exists(dir / InfoFile))
    {
      if (QFile file {dir / InfoFile}; file.open(QIODevice::ReadOnly))
      {
        QTextStream stream(&file);
        return stream.readAll();
      }      
    }
  }
  catch(const std::exception& e)
  {
    qCritical() << "ERROR: " << e.what();
  }

  return "";
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