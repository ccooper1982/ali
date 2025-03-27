#include <ali/util.hpp>
#include <string.h>
#include <cstring>
#include <sys/mount.h>
#include <blkid/blkid.h>
#include <libmount/libmount.h>
#include <libudev.h>
#include <QDebug>

Partitions PartitionUtils::m_parts;


static const std::string EfiPartitionType {"c12a7328-f81f-11d2-ba4b-00a0c93ec93b"};




static Partitions partitions;



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


/*

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


static std::optional<std::reference_wrapper<const Partition>> get_partition_from_cache(const Partitions& parts, const std::string_view dev)
{
  const auto it = std::find_if(std::cbegin(parts), std::cend(parts), [&dev](const Partition& part)
  {
    return part.dev == dev;
  });

  if (it == std::cend(parts))
    return std::nullopt;
  else
    return std::ref(*it);
}


const Partitions& probe_partitions(const PartitionOpts opts)
{  
  partitions.clear();

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
        {
          qDebug() << part;
          partitions.push_back(std::move(part));
        }
      }

      blkid_dev_iterate_end(dev_it);
    }
  }

  std::sort(partitions.begin(), partitions.end(), [](const Partition& a, const Partition& b)
  {
    return a.dev < b.dev;
  });

  return partitions;
}


const Partitions& get_partitions()
{
  return partitions;
}


bool is_path_mounted(const std::string_view path)
{
  return is_mounted(path, false);
}


bool is_dev_mounted(const std::string_view path)
{
  return is_mounted(path, true);
}


std::string get_partition_fs_from_cached (const Partitions& parts, const std::string_view dev)
{
  if (const auto opt = get_partition_from_cache(parts, dev) ; opt)
    return opt->get().fs_type;
  else
    return std::string{};
}


std::string get_partition_parent_from_cached (const Partitions& parts, const std::string_view dev)
{
  if (const auto opt = get_partition_from_cache(parts, dev) ; opt)
    return opt->get().parent_dev;
  else
    return std::string{};
}


int get_partition_part_number_from_cached (const Partitions& parts, const std::string_view dev)
{
  if (const auto opt = get_partition_from_cache(parts, dev) ; opt)
    return opt->get().part_number;
  else
    return 0;
}
*/


bool PartitionUtils::probe(const ProbeOpts opts)
{
  qDebug() << "Enter";

  m_parts.clear();

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

        if (opts == ProbeOpts::UnMounted && is_dev_mounted(dev_name))
          continue;
        
        if (auto [status, part] = read_partition(dev_name); status == PartitionStatus::Ok)
        {
          qDebug() << part;
          m_parts.push_back(std::move(part));
        }
      }

      blkid_dev_iterate_end(dev_it);
    }
  }

  std::sort(m_parts.begin(), m_parts.end(), [](const Partition& a, const Partition& b)
  {
    return a.dev < b.dev;
  });

  qDebug() << "Leave";

  return true;
}


std::tuple<PartitionStatus, Partition> PartitionUtils::read_partition(const std::string_view part_dev)
{
  static const unsigned SectorsPerPartSize = 512;

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
    Partition partition {.dev = std::string{part_dev}};
    
    if (blkid_probe_has_value(pr, "PTTYPE"))
    {
      const char * pt_type {nullptr};
      blkid_probe_lookup_value(pr, "PTTYPE", &pt_type, nullptr);
      qCritical() << "pt_type: " << pt_type;
    }

    if (blkid_probe_has_value(pr, "TYPE"))
    {
      const char * type {nullptr};
      blkid_probe_lookup_value(pr, "TYPE", &type, nullptr);
      partition.fs_type = type;
    }

    if (blkid_probe_has_value(pr, "PART_ENTRY_SIZE"))
    {
      const char * part_size{nullptr};
      if (blkid_probe_lookup_value(pr, "PART_ENTRY_SIZE", &part_size, nullptr); part_size)
        partition.size = SectorsPerPartSize * std::strtoull (part_size, nullptr, 10);
    }

    if (blkid_probe_has_value(pr, "PART_ENTRY_TYPE"))
    {
      const char * type_uuid{nullptr};
      blkid_probe_lookup_value(pr, "PART_ENTRY_TYPE", &type_uuid, nullptr);
      partition.type_uuid = type_uuid;
      partition.is_efi = partition.type_uuid == EfiPartitionType;
    }

    if (blkid_probe_has_value(pr, "PART_ENTRY_NUMBER"))
    {
      const char * number{nullptr};
      blkid_probe_lookup_value(pr, "PART_ENTRY_NUMBER", &number, nullptr);
      partition.part_number = static_cast<int>(std::strtol(number, nullptr, 10) & 0xFFFFFFFF);
    }

    if (partition.fs_type == "vfat")
    {
      if (blkid_probe_has_value(pr, "VERSION"))
      {
        const char * vfat_version{nullptr};
        blkid_probe_lookup_value(pr, "VERSION", &vfat_version, nullptr);
        partition.is_fat32 = (strcmp(vfat_version, "FAT32") == 0) ;
      }
      else
      {
        qCritical() << "could not get vfat version";
        status = PartitionStatus::Error;
      }
    }

    if (dev_t blk_dev_num = blkid_probe_get_wholedisk_devno(pr); !blk_dev_num)
      qCritical() << "Could not parent device for partition: " << part_dev;
    else
    {
      if (char * name = blkid_devno_to_devname(blk_dev_num); name)
      {
        qCritical() << "devno_to_devname: " << name;
        partition.parent_dev = name;
        ::free(name);
      }
    }

    if (status == PartitionStatus::None)
      return {PartitionStatus::Ok, partition};
    else
      return make_error();
  }
}


std::optional<std::reference_wrapper<const Partition>> PartitionUtils::get_partition(const std::string_view dev)
{
  const auto it = std::find_if(std::cbegin(m_parts), std::cend(m_parts), [&dev](const Partition& part)
  {
    return part.dev == dev;
  });

  if (it == std::cend(m_parts))
    return std::nullopt;
  else
    return std::ref(*it);
}


std::string PartitionUtils::get_partition_fs (const std::string_view dev)
{
  if (const auto opt = get_partition(dev) ; opt)
    return opt->get().fs_type;
  else
    return std::string{};
}


std::string PartitionUtils::get_partition_parent (const std::string_view dev)
{
  if (const auto opt = get_partition(dev) ; opt)
    return opt->get().parent_dev;
  else
    return std::string{};
}


int PartitionUtils::get_partition_part_number (const std::string_view dev)
{
  if (const auto opt = get_partition(dev) ; opt)
    return opt->get().part_number;
  else
    return 0;
}


bool PartitionUtils::is_mounted(const std::string_view path_or_dev, const bool is_dev)
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


bool PartitionUtils::is_path_mounted(const std::string_view path)
{
  return is_mounted(path, false);
}


bool PartitionUtils::is_dev_mounted(const std::string_view path)
{
  return is_mounted(path, true);
}
