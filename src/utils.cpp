#include <ali/util.hpp>
#include <string.h>
#include <cstring>
#include <libmount/libmount.h>

static const std::string EfiPartitionType {"c12a7328-f81f-11d2-ba4b-00a0c93ec93b"};


// TODO could create child classes: BlocksProbe, PartsProbe, PartsBlocksProbe.
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
      std::cout << "probe on " << dev << " failed: " << strerror(errno) << '\n';
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

  blkid_probe pr;
};


static std::tuple<PartitionStatus, Partition> read_partition2(const std::string_view part_dev)
{
  std::cout << __FUNCTION__ << " - " << part_dev << '\n';

  auto make_error = []{ return std::make_pair(PartitionStatus::Error, Partition{}); };


  Probe probe (part_dev);

  if (!probe.valid())
    return make_error();

  PartitionStatus status{PartitionStatus::None};

  auto pr = probe.pr;

  // full probe required for PART_ENTRY values
  if (const int r = blkid_do_fullprobe(pr) ; r == BLKID_PROBE_ERROR)
  {
    std::cout << "ERROR: fullprobe failed: " << strerror(r) << '\n';
    return make_error();
  }
  else
  {
    Partition partition;

    if (!(blkid_probe_has_value(pr, "TYPE") && blkid_probe_has_value(pr, "FSSIZE") && blkid_probe_has_value(pr, "PART_ENTRY_TYPE")))
      return make_error();
    
    const char * type {nullptr}, * fs_size{nullptr},
               * vfat_version{nullptr}, * type_uuid{nullptr};
    size_t type_uuid_len{0};
    bool isfat32 = false;

    blkid_probe_lookup_value(pr, "TYPE", &type, nullptr);
    blkid_probe_lookup_value(pr, "FSSIZE", &fs_size, nullptr);
    blkid_probe_lookup_value(pr, "PART_ENTRY_TYPE", &type_uuid, &type_uuid_len);

    const int64_t size = std::strtoll (fs_size, nullptr, 10);

    if (std::strcmp(type, "vfat") == 0)
    {
      if (blkid_probe_has_value(pr, "VERSION"))
      {
        blkid_probe_lookup_value(pr, "VERSION", &vfat_version, nullptr);
        if (isfat32 = (strcmp(vfat_version, "FAT32") == 0) ; !isfat32)
        {
          std::cout << "ERROR: vfat is not version FAT32\n";
          status = PartitionStatus::Error;
        }
      }
      else
      {
        std::cout << "ERROR: could not get vfat version\n";
        status = PartitionStatus::Error;
      }
    }

    if (status == PartitionStatus::None)
    {
      partition.type_uuid = type_uuid;
      partition.is_efi = partition.type_uuid == EfiPartitionType;
      partition.type = isfat32 ? vfat_version : type;
      partition.path = part_dev;
      partition.size = size;
      status = PartitionStatus::Ok;
    }

    return {status, partition};
  }
}


Partitions get_partitions()
{
  Partitions parts;

  if (blkid_cache cache; blkid_get_cache(&cache, nullptr) != 0)
    std::cout << "ERROR: could not create blkid cache\n";
  else
  {
    if (blkid_probe_all(cache) != 0)
      std::cout << "ERROR: blkid cache probe failed\n";
    else
    {
      const auto dev_it = blkid_dev_iterate_begin (cache);

      for (blkid_dev dev; blkid_dev_next(dev_it, &dev) == 0; )
      {
        const auto dev_name = blkid_dev_devname(dev);
        
        if (auto [status, part] = read_partition2(dev_name); status == PartitionStatus::Ok)
          parts.push_back(std::move(part));
      }

      blkid_dev_iterate_end(dev_it);
    }
  }

  return parts;
}


PartitionStatus check_partition_status(const std::string_view part_dev)
{
  std::cout << __FUNCTION__ << " - for " << part_dev << '\n';

  PartitionStatus status{PartitionStatus::Error};

  blkid_probe pr = blkid_new_probe_from_filename(part_dev.data());
	if (!pr)
  {
    std::cout << "Failed to create a new probe for: " << part_dev << '\n';
  }
  else
  {
    if (blkid_probe_enable_partitions(pr, 1) != BLKID_PROBE_OK)
    {
      std::cout << "ERROR: failed to enable partitions on probe\n";
      status = PartitionStatus::Error;
    }
    else if (blkid_probe_is_wholedisk(pr))
      status = PartitionStatus::NotPartition;
    else
    {

    }

    blkid_free_probe(pr);
  }

  return status;
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


bool is_dir_mounted(const std::string_view path)
{
  return is_mounted(path, false);
}


bool is_dev_mounted(const std::string_view path)
{
  return is_mounted(path, true);
}