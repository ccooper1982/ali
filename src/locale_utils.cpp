#include <ali/locale_utils.hpp>
#include <ali/disk_utils.hpp> // for RootMnt
#include <ali/commands.hpp>
#include <QDebug>
#include <fstream>


// these paths use RootMnt (/mnt) because they are used outside of chroot, with
// std::filesystem and std::fstream functions
static const fs::path LiveLocaleGenPath {"/etc/locale.gen"};
static const fs::path InstalledLocaleGenPath {RootMnt / "etc/locale.gen"};
static const fs::path InstalledLocaleConfPath {RootMnt / "etc/locale.conf"};
static const fs::path InstalledVirtualConsolePath {RootMnt / "etc/vconsole.conf"};
static const fs::path TimezonePath {"etc/localtime"};

std::string LocaleUtils::m_intro;
QStringList LocaleUtils::m_locales;
QStringList LocaleUtils::m_timezones;
QStringList LocaleUtils::m_keymaps;


bool LocaleUtils::read_locales()
{
  if (!fs::exists(LiveLocaleGenPath))
  {
    qCritical() << "File " << LiveLocaleGenPath.string() << " does not exist";
    return false;
  }

  try
  {
    m_intro.reserve(512);
    m_intro.clear();

    char buff[4096] = {'\0'};
    std::ifstream stream{LiveLocaleGenPath};
    
    while (stream.getline(buff, sizeof(buff)).good())
    {
      if (buff[0] == '#' && !isalpha(buff[1]))
      {
        m_intro += buff; // save the description, so we can write it later.
        m_intro += '\n';
      }
      else
      {
        //format: "#<locale> <charset>  "
        auto is_utf8 = [](const std::string_view line) -> std::pair<bool, std::string_view>
        {
          // the charset is followed by two spaces, snip out just the charset
          const auto pos_space = line.find(' '); 
          if (pos_space != std::string_view::npos && pos_space+1 < line.size())
          {
            const auto charset_start = pos_space+1;
            const auto charset_end = line.find(' ', charset_start);

            if (charset_end != std::string_view::npos)
            {
              if (line.substr(charset_start, charset_end - charset_start) == "UTF-8")
                return {true, std::string_view{line.data(), pos_space}};
            }
          }

          return {false, std::string_view{}};
        };
        
        if (const auto [store, locale_name] = is_utf8(buff+1); store) // +1 skip '#'
        {
          m_locales.emplace_back(QString::fromLocal8Bit(locale_name.data(), locale_name.size()));
        }
      }
    }
  }
  catch(const std::exception& e)
  {
    qCritical() << e.what();
  }

  return !m_locales.empty(); 
}


bool LocaleUtils::read_timezones()
{
  // store in list, to retain order, and we can later use the '/'
  // token in the name as a path when we create the sym link with `ln`
  TimezoneList cmd;
  m_timezones = cmd.get_zones();
  return !m_timezones.empty();
}


bool LocaleUtils::read_keymaps()
{
  KeyMaps cmd; 
  m_keymaps = cmd.get_list();
  return !m_keymaps.empty();
}


bool LocaleUtils::generate_locale(const QStringList& user_locales, const QString& current)
{
  if (write_locale_gen(user_locales))
  {
    ChRootCmd keys{std::format("locale-gen")};
    if (keys.execute() == CmdSuccess)
    {
      try
      {
        std::ofstream stream{InstalledLocaleConfPath};
        stream << "LANG=" << current.toStdString() << '\n';
        return true;      
      }
      catch(const std::exception& e)
      {
        qCritical() << e.what();
      }
    }
  }

  return false;
}


bool LocaleUtils::write_locale_gen(const QStringList& user_locales)
{
  // write out the intro from the live file,
  // the locales selected, then the original commented locales

  bool set{true};
  try
  {
    std::ofstream stream{InstalledLocaleGenPath};
    stream << m_intro << '\n';
    
    for (const auto& l : user_locales)
      stream << l.toStdString() << '\n';

    stream << '\n';

    for (const auto& l : m_locales)
      stream << '#' << l.toStdString() << '\n';
  }
  catch(const std::exception& e)
  {
    qCritical() << e.what() ;
    set = false;
  }

  return set;
}


bool LocaleUtils::generate_keymap(const std::string& keys)
{
  bool set{false};

  ChRootCmd load_cmd{std::format("loadkeys {}", keys)};

  if (load_cmd.execute() == CmdSuccess)
  {
    try
    {
      std::ofstream stream{InstalledVirtualConsolePath};
      stream << "KEYMAP=" << keys << '\n';

      set = true;
    }
    catch(const std::exception& e)
    {
      qCritical() << e.what() ;      
    }
  }
  
  return set;
}


bool LocaleUtils::generate_timezone(const std::string& zone)
{
  ChRootCmd set_cmd{std::format("ln -sf /usr/share/zoneinfo/{} {}", zone, TimezonePath.string())};
  if (set_cmd.execute() == CmdSuccess)
  {
    ChRootCmd set_hw_clock{"hwclock --systohc"};
    return set_hw_clock.execute() == CmdSuccess;
  }

  return false;
}