#include <ali/packages.hpp>

std::set<Package, PackageCmp> Packages::m_kernels;
std::set<Package, PackageCmp> Packages::m_required;
std::set<Package, PackageCmp> Packages::m_firmware;
std::set<Package, PackageCmp> Packages::m_important;
std::set<Package, PackageCmp> Packages::m_shells;
std::set<Package, PackageCmp> Packages::m_profile;
std::set<Package, PackageCmp> Packages::m_video;
std::set<Package, PackageCmp> Packages::m_additional;

