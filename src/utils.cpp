#include <ali/util.hpp>
#include <string.h>
#include <cstring>
#include <sys/mount.h>
#include <libmount/libmount.h>
#include <libudev.h>
#include <QDebug>


static const std::string EfiPartitionType {"c12a7328-f81f-11d2-ba4b-00a0c93ec93b"};


struct Probe
{
  Probe(const std::string_view dev,
        const int part_flags = BLKID_PARTS_ENTRY_DETAILS,
        const int super_block_flags = BLKID_SUBLKS_VERSION | BLKID_SUBLKS_FSINFO | BLKID_SUBLKS_TYPE)
  {
    if (pr = blkid_new_probe_from_filename(dev.data()); pr)
    {
      blkid_probe_enable_partitions(pr, 1);
      blkid_probe_set_partitions_flags(pr, part_flags);

      blkid_probe_enable_superblocks(pr, 1);
      blkid_probe_set_superblocks_flags(pr, super_block_flags);
    }
    else
      qCritical() << "probe on " << dev << " failed: " << strerror(errno) << '\n';
  }

  ~Probe()
  {
    if (pr)
      blkid_free_probe(pr);
  }

  bool valid() const
  {
    return pr != nullptr;
  }

  blkid_probe pr{nullptr};
};


static std::tuple<PartitionStatus, Partition> read_partition(const std::string_view part_dev)
{
  qDebug() << "Enter: " << part_dev;

  auto make_error = []{ return std::make_pair(PartitionStatus::Error, Partition{}); };

  PartitionStatus status{PartitionStatus::None};

  Probe probe (part_dev);

  if (!probe.valid())
    return make_error();

  auto pr = probe.pr;

  // full probe required for PART_ENTRY values
  if (const int r = blkid_do_fullprobe(pr) ; r == BLKID_PROBE_ERROR)
  {
    qCritical() << "fullprobe failed: " << strerror(r);
    return make_error();
  }
  else
  {
    Partition partition;
    
    const char * type {nullptr}, * vfat_version{nullptr}, * type_uuid{nullptr};
    uint64_t size{0};
    bool isfat32 = false;

    if (blkid_probe_has_value(pr, "TYPE"))
      blkid_probe_lookup_value(pr, "TYPE", &type, nullptr);

    if (blkid_probe_has_value(pr, "FSSIZE"))
    {
      const char * fs_size{nullptr};
      if (blkid_probe_lookup_value(pr, "FSSIZE", &fs_size, nullptr); fs_size)
        size = std::strtoull (fs_size, nullptr, 10);
    } 
    else if (blkid_probe_has_value(pr, "PART_ENTRY_SIZE"))
    {
      const char * part_size{nullptr};
      if (blkid_probe_lookup_value(pr, "PART_ENTRY_SIZE", &part_size, nullptr); part_size)
      {
        // PART_ENTRY_SIZE is size in 512 sectors
        size = 512 * std::strtoull (part_size, nullptr, 10);
      }
    }

    if (blkid_probe_has_value(pr, "PART_ENTRY_TYPE"))
      blkid_probe_lookup_value(pr, "PART_ENTRY_TYPE", &type_uuid, nullptr);

    if (type && std::strcmp(type, "vfat") == 0)
    {
      if (blkid_probe_has_value(pr, "VERSION"))
      {
        blkid_probe_lookup_value(pr, "VERSION", &vfat_version, nullptr);
        isfat32 = (strcmp(vfat_version, "FAT32") == 0) ;
      }
      else
      {
        qCritical() << "could not get vfat version";
        status = PartitionStatus::Error;
      }
    }

    if (status == PartitionStatus::None)
    {
      if (type_uuid)
      {
        partition.type_uuid = type_uuid;
        partition.is_efi = partition.type_uuid == EfiPartitionType;
      }
      
      if (type)
        partition.fs_type = type;

      partition.size = size;
      partition.path = part_dev;
      partition.is_fat32 = isfat32;
      status = PartitionStatus::Ok;
    }

    return {status, partition};
  }
}


static bool is_mounted(const std::string_view path_or_dev, const bool is_dev)
{
  bool mounted = false;

  if (auto table = mnt_new_table(); table)
  {
    if (mnt_table_parse_mtab(table, nullptr) == 0)
    {
      if (is_dev)
        mounted = nullptr != mnt_table_find_source(table, path_or_dev.data(), MNT_ITER_FORWARD);
      else
        mounted = nullptr != mnt_table_find_target(table, path_or_dev.data(), MNT_ITER_FORWARD);
    }

    mnt_free_table(table);
  }

  return mounted;
}


Partitions get_partitions(const PartitionOpts opts)
{
  Partitions parts;
  
  if (blkid_cache cache; blkid_get_cache(&cache, nullptr) != 0)
    qCritical() << "could not create blkid cache";
  else
  {
    if (blkid_probe_all(cache) != 0)
      qCritical() << "blkid cache probe failed";
    else
    {
      const auto dev_it = blkid_dev_iterate_begin (cache);

      for (blkid_dev dev; blkid_dev_next(dev_it, &dev) == 0; )
      {
        const auto dev_name = blkid_dev_devname(dev);

        if (opts == PartitionOpts::UnMounted && is_dev_mounted(dev_name))
          continue;
        
        if (auto [status, part] = read_partition(dev_name); status == PartitionStatus::Ok)
          parts.push_back(std::move(part));
      }

      blkid_dev_iterate_end(dev_it);
    }
  }

  std::sort(parts.begin(), parts.end(), [](const Partition& a, const Partition& b)
  {
    return a.path < b.path;
  });

  return parts;
}


bool is_dir_mounted(const std::string_view path)
{
  return is_mounted(path, false);
}


bool is_dev_mounted(const std::string_view path)
{
  return is_mounted(path, true);
}


std::string get_partition_fs_from_data (const Partitions& parts, const std::string_view dev)
{
  const auto it = std::find_if(std::cbegin(parts), std::cend(parts), [&dev](const Partition& part)
  {
    return part.path == dev;
  });

  return it == std::cend(parts) ? std::string{} : it->fs_type;
}