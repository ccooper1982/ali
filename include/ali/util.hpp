#ifndef ALI_UTILS_H
#define ALI_UTILS_H

#include <vector>
#include <map>
#include <string_view>
#include <iostream>
#include <format>
#include <sys/mount.h>
#include <blkid/blkid.h>
#include <ali/common.hpp>


inline const fs::path BootMnt{"/mnt/boot"};
inline const fs::path RootMnt{"/mnt"};
inline const fs::path EfiMnt{"/mnt/boot/efi"};
inline const fs::path FsTabPath{"/mnt/etc/fstab"};


enum class PartitionStatus
{
  None,
  Ok,  
  SizeSmall,
  Type,
  NotPartition,
  Error
};


struct Partition
{
  Partition() = default;

  Partition(const std::string& path, const std::string& type, const int64_t size)
    : type(type), path(path), size(size)
  {

  }


  std::string type; // ext4, vfat, etc
  std::string path; // /dev/sda1, /dev/nvmen1p3, etc
  std::string type_uuid;
  int64_t size{0};
  bool assigned{false};
  bool is_efi{false};
  bool is_fat32{false};
};


using Partitions = std::vector<Partition>;


//PartitionStatus check_partition_status(const std::string_view part);
Partitions get_partitions();

bool is_dir_mounted(const std::string_view path);
bool is_dev_mounted(const std::string_view path);

#endif
