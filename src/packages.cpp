#include <ali/packages.hpp>

std::set<Package, PackageCmp> Packages::m_kernels;
std::set<Package, PackageCmp> Packages::m_required;
std::set<Package, PackageCmp> Packages::m_firmware;

