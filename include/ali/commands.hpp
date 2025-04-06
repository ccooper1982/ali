#ifndef ALI_COMMANDS_H
#define ALI_COMMANDS_H

#include <string>
#include <string_view>
#include <functional>
#include <format>
#include <ali/disk_utils.hpp>
#include <ali/common.hpp>

inline const int CmdSuccess = 0;
inline const int CmdFail = -1;

using OutputHandler = std::function<void(const std::string_view)>;

struct Command
{
  Command ();
  // This constructor is used by PlatformSize. Don't like, redesign PlatformSize and bin this.
  Command (OutputHandler&& on_output) ; 
  // If command has no output or output is irrelevant.
  Command (const std::string_view cmd) ;
  // Run command and receive each line in the supplied callback.
  Command (const std::string_view cmd, OutputHandler&& on_output) ;
  
  virtual int execute (const std::string_view cmd, const int max_lines = -1);
  virtual int execute (const int max_lines = -1);
  virtual int execute (OutputHandler&& on_output, const int max_lines = -1);
  virtual int execute_write(const std::string_view s);
  
  int get_result() const { return m_result; }

protected:
  bool executed() const { return m_executed; }

private:
  std::string m_cmd;
  OutputHandler m_handler;
  bool m_executed{false};
  int m_result{CmdSuccess};
};


// Runs a single command as arch-chroot.
struct ChRootCmd : public Command
{
  ChRootCmd(const std::string_view cmd) :
    Command(std::format("arch-chroot {} {}", RootMnt.string(), cmd))
  {

  }

  ChRootCmd(const std::string_view cmd, std::function<void(const std::string_view)>&& on_output) :
    Command(std::format("arch-chroot {} {}", RootMnt.string(), cmd), std::move(on_output))
  {

  }
};


struct CommandExist : public Command
{
public:
  CommandExist (const std::string_view program) ;
  bool exists();


private:
  
  void on_output(const std::string_view line);
  
private:
  // default true because "command -v" returns nothing if it 
  // program doesn't exist
  bool m_missing{true};
};


struct PlatformSize : public Command
{
  PlatformSize() ;

  void on_output(const std::string_view line);

  int get_size ();

  bool platform_file_exist() const;

private:
  int m_size{0};
  bool m_exists{false};
};


struct CpuVendor : public Command
{
  enum class Vendor { None, Amd, Intel };

  CpuVendor() ;

  void on_output(const std::string_view line);

  Vendor get_vendor ();

  

private:
  Vendor m_vendor {Vendor::None};
};


struct TimezoneList : public Command
{
  TimezoneList();

  void on_output(const std::string_view line);

  QStringList get_zones();

private:
  QStringList m_zones;  
};


struct KeyMaps : public Command
{
  KeyMaps();

  QStringList get_list();

private:
  QStringList m_keys;
};


struct SysClockSync : public Command
{
  SysClockSync();
};


struct VideoVendor : public Command
{
  VideoVendor();

  GpuVendor get_vendor();
  
private:
  unsigned short n_amd{0};
  unsigned short n_nvidia{0};
  unsigned short n_vm{0};
};


struct CommandAsUser : public ChRootCmd
{
  CommandAsUser(const std::string_view user, const std::string_view cmd) : ChRootCmd(std::format("su -c {} {}", cmd, user))
  {

  }
};




// Create filesystem
template<class FS>
struct CreateFilesystem : public Command
{
  CreateFilesystem(const std::string_view dev) : Command(std::format("mkfs.{} {}", FS::cmd, dev))
  {
    qDebug() << std::format("mkfs.{} {}", FS::cmd, dev);
  }

private:
  std::string dev;
};

struct Ext4
{
  static constexpr char cmd[] = "ext4";
};

struct Fat32
{
  static constexpr char cmd[] = "vfat -F 32";
};

using CreateExt4 = CreateFilesystem<Ext4>;
using CreateFat32 = CreateFilesystem<Fat32>;


// set partition type
template<class T>
struct SetPartitionType : public Command
{
  SetPartitionType(const int part_num, const std::string_view parent_dev)
    : Command(std::format("sgdisk -t{}:{} {}", part_num, T::hex, parent_dev))
  {
    qDebug() << std::format("sgdisk -t{}:{} {}", part_num, T::hex, parent_dev);
  }
};

// Hex codes taken from: https://gist.github.com/gotbletu/a05afe8a76d0d0e8ec6659e9194110d2
struct EfiType
{
  static constexpr char hex[] = "ef00";
};

struct LinuxRootType
{
  static constexpr char hex[] = "8304";
};

struct LinuxHomeType
{
  static constexpr char hex[] = "8302";
};

// Not used yet
// struct LinuxSwapType
// {
//   static constexpr char hex[] = "8200";
// };

using SetPartitionAsEfi = SetPartitionType<EfiType>;
using SetPartitionAsLinuxRoot = SetPartitionType<LinuxRootType>;
using SetPartitionAsLinuxHome = SetPartitionType<LinuxHomeType>;


#endif
