#ifndef ALI_UTILS_H
#define ALI_UTILS_H

#include <vector>
#include <string_view>
#include <iostream>
#include <QDebug>
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


enum class ProbeOpts
{
  All,
  UnMounted
};


class PartitionUtils
{
public:
  // probe all block devices. clears previous probe results
  static bool probe(const ProbeOpts opts);
  
  static const Partitions& partitions() { return m_parts; }
  static std::size_t num_partitions() { return m_parts.size(); }
  static bool have_partitions() { return !m_parts.empty(); }

  static std::string get_partition_fs (const std::string_view dev);
  static int get_partition_part_number (const std::string_view dev);
  static std::string get_partition_parent (const std::string_view dev);

  static bool is_path_mounted(const std::string_view path);
  static bool is_dev_mounted(const std::string_view path);

private:
  static std::tuple<PartitionStatus, Partition> read_partition(const std::string_view part_dev);
  static std::optional<std::reference_wrapper<const Partition>> get_partition(const std::string_view dev);
  static bool is_mounted(const std::string_view path_or_dev, const bool is_dev);

private:
  static Partitions m_parts;
};

#endif
