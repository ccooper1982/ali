#include <ali/util.hpp>
#include <cstring>


static const std::string EfiPartitionType {"C12A7328-F81F-11D2-BA4B-00A0C93EC93B"};


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


static std::tuple<PartitionStatus, Partition> read_partition(const std::string_view part_dev)
{
  std::cout << __FUNCTION__ << " - " << part_dev << '\n';

  Probe probe (part_dev);

  if (!probe.valid())
    return {PartitionStatus::Error, Partition{}};

  PartitionStatus status{PartitionStatus::None};

  auto pr = probe.pr;

  // full probe required for PART_ENTRY values
  blkid_do_fullprobe(pr); 

  const char * type {nullptr}, * fs_size{nullptr}, * vfat_version{nullptr}, * type_uuid{nullptr};
  int64_t size = 0;
  size_t type_uuid_len{0};
  bool isfat32 = false;

  if (blkid_probe_has_value(pr, "TYPE") && blkid_probe_has_value(pr, "FSSIZE") && blkid_probe_has_value(pr, "PART_ENTRY_TYPE"))
  {
    blkid_probe_lookup_value(pr, "TYPE", &type, nullptr);
    blkid_probe_lookup_value(pr, "FSSIZE", &fs_size, nullptr);
    blkid_probe_lookup_value(pr, "PART_ENTRY_TYPE", &type_uuid, &type_uuid_len);

    size = std::strtoll (fs_size, nullptr, 10);

    if (std::strcmp(type, "vfat") == 0)
    {
      if (blkid_probe_has_value(pr, "VERSION"))
      {
        blkid_probe_lookup_value(pr, "VERSION", &vfat_version, nullptr);
        if (isfat32 = (strcmp(vfat_version, "FAT32") == 0) ; !isfat32)
        {
          std::cout << "ERROR: vfat is not version FAT32\n";
          status = PartitionStatus::Type;
        }
      }
      else
      {
        std::cout << "ERROR: could not get vfat version\n";
        status = PartitionStatus::Error;
      }
    }
  }
  else
  {
    std::cout << "ERROR: Could not get TYPE, FSSIZE and PART_ENTRY_TYPE\n";
    status = PartitionStatus::Error;
  }

  Partition partition;

  if (status == PartitionStatus::None)
  {
    // transform UUID to uppercase, avoiding trailing '\0'
    std::for_each_n(type_uuid, type_uuid_len, [&dest = partition.type_uuid](const auto c)
    {
      if (c != '\0')
        dest += ::toupper(c);
    });

    partition.is_efi = partition.type_uuid == EfiPartitionType;
    partition.type = isfat32 ? vfat_version : type;
    partition.path = part_dev;
    partition.size = size;
    status = PartitionStatus::Ok;
  }

  return {status, partition};
}


static std::vector<Partition> read_partitions(const std::string_view dev)
{
  std::cout << __FUNCTION__ << " - " << dev << '\n';

  std::vector<Partition> partitions;

  const blkid_probe pr = blkid_new_probe_from_filename(dev.data());

  if (!pr)
    return partitions;
  
  blkid_probe_enable_partitions(pr, 1);

  if (blkid_do_probe(pr) != BLKID_PROBE_ERROR)
  {
    if (blkid_partlist ls = blkid_probe_get_partitions(pr); ls)
    {
      // primary partition table
      blkid_parttable root_tab = blkid_partlist_get_table(ls);

      if (root_tab && std::strcmp("gpt", blkid_parttable_get_type(root_tab)) == 0)
      {
        std::cout << std::format( "Size: {}, Type: {}, ID: {}", blkid_probe_get_size(pr),
                                                                blkid_parttable_get_type(root_tab),
                                                                blkid_parttable_get_id(root_tab));
        std::cout << '\n';
        
        // probe each partition
        if (const int nparts = blkid_partlist_numof_partitions(ls); nparts == BLKID_PROBE_ERROR)
        {
          std::cout << "Failed to request number of partitions for: " << dev << '\n';
        }
        else
        {    
          for (int i = 0; i < nparts; ++i)
          {
            blkid_partition par = blkid_partlist_get_partition(ls, i);
            const int partNumber = blkid_partition_get_partno(par);
            const std::string part_path = std::format("{}{}", dev, partNumber);
            
            if (auto [status,partition] = read_partition(part_path); status == PartitionStatus::Ok)
              partitions.push_back(std::move(partition));
          }
        }
      }
    }
  }

  blkid_free_probe(pr);

  return partitions;
}


DiskTree create_disk_tree ()
{
  static const fs::path SysPath {"/sys/block"};
  static const fs::path DevPath {"/dev"};

  DiskTree tree;

  for (const auto& entry : fs::directory_iterator{SysPath})
  {
    const auto dev_path = (DevPath / entry.path().stem()).c_str();

    std::cout << "Probing " << dev_path << '\n';
    
    if (blkid_probe pr = blkid_new_probe_from_filename(dev_path); !pr)
    {
      std::cout << "Failed to create a new probe for: " << dev_path << '\n';
    }
    else
    {
      if (blkid_probe_enable_partitions(pr, 1) == BLKID_PROBE_OK)
      {
        if (blkid_do_probe(pr) == BLKID_PROBE_OK && blkid_probe_is_wholedisk(pr))
        {
          if (auto parts = read_partitions(dev_path); !parts.empty())
            tree.set_partitions(dev_path, parts);
        }
      }

      blkid_free_probe(pr);
    }
  }

  return tree;
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