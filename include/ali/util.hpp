#ifndef ALI_UTILS_H
#define ALI_UTILS_H

#include <vector>
#include <map>
#include <string_view>
#include <iostream>
#include <format>
#include <QString>
#include <blkid/blkid.h>
#include <ali/common.hpp>


inline const fs::path BootMnt{"/mnt/boot"};
inline const fs::path RootMnt{"/mnt"};
inline const fs::path EfiMnt{"/mnt/boot/efi"};
inline const fs::path FsTabPath{"/mnt/etc/fstab"};


// partitions, mounting, filesystems

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
  // struct Assign
  // {
  //   std::string fs_type;
  //   bool is_efi{false};
  // };

  Partition() = default;

  Partition(const std::string& path, const std::string& type, const int64_t size)
    : fs_type(type), path(path), size(size)
  {

  }

  std::string fs_type;    // ext4, vfat, etc
  std::string path;       // /dev/sda1, /dev/nvmen1p3, etc
  std::string type_uuid;  // partition type UUID (useful to identify EFI)
  int64_t size{0};        // fs size  
  bool is_efi{false};
  bool is_fat32{false};
  //Assign assigned;        // in UI, user can assign a filesystem to a partition
};


using Partitions = std::vector<Partition>;

enum class PartitionOpts
{
  All,
  UnMounted
};

Partitions get_partitions(const PartitionOpts opts);

std::string get_partition_fs_from_data (const Partitions& parts, const std::string_view dev);

bool is_dir_mounted(const std::string_view path);
bool is_dev_mounted(const std::string_view path);


#endif
