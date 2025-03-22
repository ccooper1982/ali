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
};


using Partitions = std::vector<Partition>;


struct DiskTree
{
  using PartitionIt = std::vector<Partition>::iterator;
  using Disks = std::map<std::string, std::vector<Partition>>;


  void add_disk(const std::string_view dev)
  {
    disks.emplace(dev, std::vector<Partition>{});
  }

  void add_partition(const std::string& disk_dev, const std::string& part_dev, const std::string& type, const int64_t size)
  {
    if (disks.contains(disk_dev))
      disks[disk_dev].emplace_back(part_dev, type, size);
  }

  void set_partitions(const std::string& disk_dev, const std::vector<Partition>& parts)
  {
    if (!disks.contains(disk_dev))
      disks.emplace(disk_dev, parts);
    else
      disks[disk_dev] = parts;
  }

  const Disks& get_disks() const
  {
    return disks;
  }

  
  bool assign_boot (const std::string& dev)
  {
    const bool set = assign_partition(dev);
    m_boot = set ? dev : "";
    return set;
  }

  bool assign_root (const std::string& dev)
  {
    const bool set = assign_partition(dev);
    m_root = set ? dev : "";
    return set;
  }

  bool assign_home (const std::string& dev)
  {
    const bool set = assign_partition(dev);
    m_home = set ? dev : "";
    return set;
  }

  void assign_home_to_root()
  {
    m_home = m_root;
  }

  
  const std::string& get_boot() const
  {
    return m_boot;
  }

  const std::string& get_root() const
  {
    return m_root;
  }

  const std::string& get_home() const
  {
    return m_home;
  }

  int64_t get_partition_size(const std::string& part_dev)
  {
    if (const auto it = do_get_partition(part_dev); it != PartitionIt{})
      return it->size;
    return 0;
  }

  bool is_partition_efi (const std::string& part_dev)
  {
    if (const auto it = do_get_partition(part_dev); it != PartitionIt{})
      return it->is_efi;
    return false;
  }

private:
  PartitionIt do_get_partition(const std::string& part_dev)
  {
    for(auto& disk : disks)
    {
      for (auto it = disk.second.begin(); it != disk.second.end() ; ++it)
      {
        if (it->path == part_dev)
          return it;
      }
    }

    return PartitionIt{};
  }

  bool assign_partition (const std::string& part_dev)
  {
    if (const auto it = do_get_partition(part_dev); it != PartitionIt{})
    {
      it->assigned = true;
      return true;
    }
    return false;
  }

private:
  Disks disks;
  std::string m_boot, m_root, m_home;
};


std::vector<Partition> read_disk(const std::string_view dev);
PartitionStatus check_partition_status(const std::string_view part);
std::vector<std::string> get_installable_devices();
DiskTree create_disk_tree (); // TODO remove
Partitions get_partitions();

bool is_dir_mounted(const std::string_view path);
bool is_dev_mounted(const std::string_view path);

#endif
