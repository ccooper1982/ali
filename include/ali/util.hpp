#ifndef ALI_UTILS_H
#define ALI_UTILS_H

#include <vector>
#include <string_view>
#include <iostream>
#include <QDebug>
#include <ali/common.hpp>


// TODO this could benefit from a refresh, the functions have become
//      like a C api

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
  std::string dev;        // /dev/sda1, /dev/nvmen1p3, etc
  std::string parent_dev; // /dev/sda, /dev/nvmen1, etc
  std::string fs_type;    // ext4, vfat, etc
  std::string type_uuid;  // partition type UUID (useful to identify EFI)
  int64_t size{0};        // partition/filesystem size  
  int part_number{0};     // partition number
  bool is_efi{false};     // if part type UUID is for EFI
  bool is_fat32{false};   // if fs_type is VFAT and version is FAT32
};


inline QDebug operator<<(QDebug q, const Partition& p)
{
  q << p.dev << '\n';
  q << '\t' << "Filesystem: " << p.fs_type << '\n';
  q << '\t' << "Size: " << p.size << '\n';
  q << '\t' << "Part Type UUID: " << p.type_uuid << '\n';
  q << '\t' << "Part Num: " << p.part_number << '\n';
  q << '\t' << "EFI: " << p.is_efi << '\n';
  q << '\t' << "FAT32: " << p.is_fat32 << '\n';
  return q;
}

using Partitions = std::vector<Partition>;

enum class PartitionOpts
{
  All,
  UnMounted
};

const Partitions& probe_partitions(const PartitionOpts opts);
// get from cached results, from a previous call to probe_partitions(const PartitionsOpts&)
const Partitions& get_partitions();

std::string get_partition_fs_from_cached (const Partitions& parts, const std::string_view dev);
int get_partition_part_number_from_cached (const Partitions& parts, const std::string_view dev);
std::string get_partition_parent_from_cached (const Partitions& parts, const std::string_view dev);

bool is_path_mounted(const std::string_view path);
bool is_dev_mounted(const std::string_view path);


#endif
