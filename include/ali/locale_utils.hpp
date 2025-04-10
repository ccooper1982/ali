#ifndef ALI_LOCALEUTILS_H
#define ALI_LOCALEUTILS_H


#include <string>
#include <QString>
#include <QStringList>
#include <ali/common.hpp>



/// Locality consists of:
///   1. Locales (language, charset) governing text, monetary, time/dates, etc
///   2. Keyboard mapping 
///   3. Timezone
///
///   Locales:
///     - Available locales in `/etc/locale.gen`
///     - In live ISO, they are all commented out
///     - Required locales are uncommented, then `locale-gen` is called
///     - Finally set the active/current locale in `locale.conf`
///   
///   Keyboard:
///     - List of available keys from `localectl list-keymaps`
///     - Then then set with `loadkeys`
///     - Then set virtual console keymap in `/etc/vconsole.conf`
///   
///   Timezone: 
///     - Get: `timedatectl list-timezones`
///     - Set with `ln -sf /usr/share/zoneinfo/Region/City /etc/localtime`
class LocaleUtils
{
public:
  static bool read_locales();
  static bool read_keymaps();
  static bool read_timezones();
  

  static const QStringList& get_locales() { return m_locales; }
  static const QStringList& get_keymaps() { return m_keymaps; }
  static const QStringList& get_timezones() { return m_timezones; }

  static bool generate_locale(const QStringList& user_locales, const QString& current);
  static bool generate_keymap(const std::string& keys);
  static bool generate_timezone(const std::string& zone);
  
private:
  static bool write_locale_gen(const QStringList& user_locales);

private:
  static QStringList m_locales;
  static QStringList m_keymaps;
  static QStringList m_timezones;
};

#endif
