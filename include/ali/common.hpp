#ifndef ALI_COMMON_H
#define ALI_COMMON_H

#include <filesystem>

namespace fs = std::filesystem;

static inline const fs::path InstallLogPath {"/var/log/ali/install.log"};

#endif
