#ifndef ALI_PACKAGES_H
#define ALI_PACKAGES_H

#include <set>
#include <string>
#include <ostream>
#include <QString>
#include <QDebug>


// // unused for now
// enum class PackageStatus
// {
//   Ok,
//   Conflict 
// };

struct Package
{
  Package(const QString& name) : name(name)
  {
  }
  
  Package(const Package&) = default;
  Package& operator=(const Package&) = default;
  Package& operator=(Package&&) = default;

  QString name;
};

struct PackageCmp
{
  bool operator()(const Package& a, const Package& b) const
  {
    return a.name < b.name;
  }
};


using PackageSet = std::set<Package, PackageCmp>;


class Packages
{
public:
  static void add_kernel(const QString& name) { add(name, m_kernels); }
  static void remove_kernel(const QString& name) { remove(name, m_kernels); }

  static void add_required(const QString& name)  { add(name, m_required); }
  static void remove_required(const QString& name)  { remove(name, m_required); }

  static void add_firmware(const QString& name)  { add(name, m_firmware); }
  static void remove_firmware(const QString& name)  { remove(name, m_firmware); };

  static void add_important(const QString& name)  { add(name, m_important); }
  static void remove_important(const QString& name)  { remove(name, m_important); };

  static void add_additional(const QStringList& names)  { add(names, m_additional); }
  static void remove_additional(const QStringList& names)  { remove(names, m_additional); }; 

  static void set_shell(const QString& name)  { set({name}, m_shells); }

  static void set_profile_packages(const QStringList& names) { set(names, m_profile); }
  static void set_video_packages(const QStringList& names) { set(names, m_video); }


  static const PackageSet& kernels() { return m_kernels; }
  static const PackageSet& required() { return m_required; }
  static const PackageSet& firmware() { return m_firmware; }
  static const PackageSet& important() { return m_important; }
  static const PackageSet& shells() { return m_shells; }
  static const PackageSet& profile() { return m_profile; }
  static const PackageSet& video() { return m_video; }
  

  static void dump_kernels (QDebug& q) { dump(q, m_kernels); }
  static void dump_required (QDebug& q) { dump(q, m_required); }
  static void dump_firmware (QDebug& q) { dump(q, m_firmware); }
  static void dump_important (QDebug& q) { dump(q, m_important); }
  static void dump_shells (QDebug& q) { dump(q, m_shells); }
  static void dump_profile (QDebug& q) { dump(q, m_profile); }
  static void dump_additional (QDebug& q) { dump(q, m_additional); }


  static bool have_kernel () { return !m_kernels.empty(); }
  static bool have_required () { return !m_required.empty(); }

private:

  static void add(const QString& name, PackageSet& ps)
  {
    ps.emplace(name);
  }

  static void add(const QStringList& names, PackageSet& ps)
  {
    ps.insert(names.cbegin(), names.cend());
  }

  static void set(const QStringList& names, PackageSet& ps)
  {
    ps = PackageSet{names.cbegin(), names.cend()};
    // ps.clear();
    // ps.insert(names.cbegin(), names.cend());
  }

  static void remove (const QStringList& names, PackageSet& ps)
  {
    for(const auto& name : names)
      ps.erase(name);
  }

  static void remove (const QString& name, PackageSet& ps)
  {
    ps.erase(name);
  }

  static void dump(QDebug& q, const PackageSet& packages, const char sep = '\n')
  {
    for (const auto& p : packages)
      q << p.name << sep;
  }

  static void dump(std::ostream& s, const PackageSet& packages, const char sep)
  {
    for (const auto& p : packages)
      s << p.name.toStdString() << sep;
  }


private:
  static PackageSet m_kernels;
  static PackageSet m_required;
  static PackageSet m_firmware;
  static PackageSet m_important;
  static PackageSet m_shells;     // UI only permits one for now
  static PackageSet m_profile;    // packages for the profile
  static PackageSet m_video;      // packages for the video/gpu  
  static PackageSet m_additional; // user-typed
};


inline std::ostream& operator<<(std::ostream& s, const PackageSet& ps)
{ 
  for (const auto& p : ps)
    s << p.name.toStdString() << ' ';
  return s;
}


inline QDebug operator<<(QDebug q, const Packages& p)
{
  q << "Required:\n";
  p.dump_required(q);

  q << "Kernels:\n";
  p.dump_kernels(q);

  q << "Firmware:\n";
  p.dump_firmware(q);

  q << "Important:\n";
  p.dump_important(q);

  q << "Shells:\n";
  p.dump_shells(q);

  q << "Profile:\n";
  p.dump_profile(q);

  q << "Additional:\n";
  p.dump_additional(q);

  return q;
}


#endif
