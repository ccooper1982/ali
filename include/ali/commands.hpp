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


// TODO Commands have become untidy:
//      - Maybe split out between ReadCommand and WriteCommand
//      - Find a tidier way of handling ChRootCmd, which can do a single command or multiple (with execute_commands())


struct Command
{
  Command ();
  // This constructor is used by PlatformSize. Don't like, redesign PlatformSize and bin this.
  Command (OutputHandler&& on_output) ; 
  // If command has no output or output is irrelevant.
  Command (const std::string_view cmd) ;
  // Run command and receive each line in the supplied callback.
  Command (const std::string_view cmd, OutputHandler&& on_output) ;

  ~Command();
  
  virtual int execute (const std::string_view cmd, const int max_lines = -1);
  virtual int execute (const int max_lines = -1);
  virtual int execute (OutputHandler&& on_output, const int max_lines = -1);
  
  virtual int execute_write(const std::string_view s);

  bool write (const std::string_view s);
  void end_write();

  int get_result() const { return m_result; }

protected:
  bool executed() const { return m_executed; }
  bool start_write(const std::string_view cmd);
  

private:
  int close();

private:
  std::string m_cmd;
  OutputHandler m_handler;
  bool m_executed{false};
  int m_result{CmdSuccess};
  FILE * m_fd{nullptr};
};


// This command is equivalent of doing this in terminal for user1:
//  (echo "cd /home/user1"; echo "su user1"; echo "<command1>"; echo "<command2>";) | arch-chroot /mnt
// We `su` so that `~` can be used in the user commands.

// Can't use `arch-chroot -u <username>` because it doesn't appear to 'become' the user, i.e.
// `cd ~` doesn't change to the user's home (it remains in /root). Or I'm misusing it.
struct ChRootCmd : public Command
{
private:
  static std::string create_cmd(const std::string_view cmd)
  {
    return std::format("arch-chroot {} {}", RootMnt.string(), cmd) ;
  }

  static std::string create_shell_cmd(const std::string_view cmd)
  {
    return create_shell_cmd({QString::fromLocal8Bit(cmd.data(), cmd.size())}) ;
  }


  static std::string create_shell_cmd(const QStringList& cmds, const std::string_view user = "")
  {
    std::stringstream ss;
    ss << "(";

    if (!user.empty())
    {
      ss << std::format(R"!(echo "cd /home/{}"; )!", user);
      ss << std::format(R"!(echo "su {}"; )!", user);
    }

    for (const auto& cmd : cmds)
      ss << std::format(R"!(echo "{}"; )!", cmd.toStdString());

    ss << " exit;) | arch-chroot " << RootMnt.string();

    const auto cmd_string = ss.str();

    qDebug() << cmd_string;
    return cmd_string;
  }


public:
  ChRootCmd(const std::string_view cmd, const bool launch_shell = true) : Command(launch_shell ? create_shell_cmd(cmd) : create_cmd(cmd))
  {

  }

  ChRootCmd(const std::string_view cmd, std::function<void(const std::string_view)>&& on_output, const bool launch_shell = true) :
    Command(launch_shell ? create_shell_cmd(cmd) : create_cmd(cmd), std::move(on_output))
  {

  }

  // run commands are user: when user is set, when entering chroot, we `su {user}` before
  // executing commands. If running at root, leave `user` empty.
  ChRootCmd(const QStringList& cmds, const std::string_view user = "") : Command(create_shell_cmd(cmds, user))
  {

  }
};


/*
// Runs a single command as chroot.
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



struct ChRootUserCmd : public Command
{
  ChRootUserCmd() :  Command()
  {
    
  }


  bool execute_commands (const std::string_view user, const QStringList& cmds)
  {
    std::stringstream ss;
    ss << "(";
    ss << std::format(R"!(echo "cd /home/{}"; )!", user);
    ss << std::format(R"!(echo "su {}"; )!", user);

    for (const auto& cmd : cmds)
      ss << std::format(R"!(echo "{}"; )!", cmd.toStdString());

    ss << " exit;) | arch-chroot " << RootMnt.string();

    const auto cmd_string = ss.str();

    qDebug() << cmd_string;
    
    return execute(cmd_string) == CmdSuccess;
  }
};
*/


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


struct GetCpuVendor : public Command
{
  GetCpuVendor() ;

  void on_output(const std::string_view line);

  CpuVendor get_vendor ();

  

private:
  CpuVendor m_vendor {CpuVendor::None};
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
  unsigned short n_intel{0};
};


struct GetShellPath : public ChRootCmd
{
  // TODO or use: pacman -Qo <shell>
  GetShellPath(const std::string_view shell_name) :
    ChRootCmd(std::format("chsh -l"), std::bind_front(&GetShellPath::on_output, std::ref(*this))),
    m_shell_name(shell_name)
  {

  }

  void on_output (const std::string_view entry)
  {
    if (entry.empty() || m_shell_name.empty())
      return;

    // TODO check if std::filesystem has a way to resolve target path of a link
    if (!fs::is_symlink(fs::path{entry}))
    {
      if (const auto perms = fs::status(entry).permissions(); static_cast<int>(perms & fs::perms::owner_exec))
      {
        if (fs::path{entry}.stem() == fs::path{m_shell_name})
          m_shell_path = entry;
      }
    }
  }

  fs::path get_path()
  {
    if (m_shell_path.empty())
      execute();

    return m_shell_path;
  }

private:
  fs::path m_shell_path;
  std::string_view m_shell_name;
};


struct SetShell : public ChRootCmd
{
  SetShell(const fs::path path, const std::string_view user) : ChRootCmd(std::format("chsh -s {} {}", path.string(), user))
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
