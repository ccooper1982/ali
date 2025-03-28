#include <ali/commands.hpp>
#include <ali/common.hpp>
#include <iostream>
#include <QDebug>


// Command
Command::Command ()
{
}

Command::Command (std::function<void(const std::string_view)>&& on_output) :
  m_handler(std::move(on_output))
{
}

Command::Command (const std::string_view cmd) :
  m_cmd(cmd)
{
}

Command::Command (const std::string_view cmd, OutputHandler&& on_output) :
  m_cmd(cmd),
  m_handler(std::move(on_output))
{
}


int Command::execute (const std::string_view cmd, const int max_lines)
{
  m_executed = true;

  if (cmd.empty())
    return CmdSuccess;

  // redirect stderr to stdout (pacstrap, and perhaps others, can outpput errors to stderr)
  if (FILE * fd = ::popen(std::format("{} {}", cmd, "2>&1").data(), "r"); fd)
  {
    char buff[4096];
    int n_lines{0};
    size_t n_chars{0};
    
    for (char c ; ((c = fgetc(fd)) != EOF) && n_chars < sizeof(buff); )
    {
      if (!m_handler)
        continue;
            
      if (c == '\n')
      {
        m_handler(std::string_view {buff, n_chars});

        if (++n_lines == max_lines)
          break;

        n_chars = 0;
      }
      else
      {
        buff[n_chars++] = c;
      }
    }
    
    if (m_handler && n_chars)
      m_handler(std::string_view {buff, n_chars});

    return (m_result = pclose(fd));
  }
  else
    return CmdFail;
}


int Command::execute (const int max_lines)
{
  return execute(m_cmd, max_lines);
}


int Command::execute (OutputHandler&& on_output, const int max_lines)
{
  m_handler = std::move(on_output);
  return execute(m_cmd, max_lines);
}


int Command::execute_write(const std::string_view s)
{
  std::cout << "execute_write()\n";

  if (FILE * fd = ::popen(m_cmd.data(), "w"); fd)
  {
    std::cout << "writing: " << s << '\n';

    fputs(s.data(), fd);
    return ::pclose(fd);
  }
  return CmdFail;
}



//
CommandExist::CommandExist (const std::string_view program) :
  Command(std::format("command -v {}", program), std::bind_front(&CommandExist::on_output, std::ref(*this)))
{
}
    
bool CommandExist::exists()
{
  execute();
  return get_result() == CmdSuccess;
}

void CommandExist::on_output(const std::string_view line)
{
  m_missing = line.empty() || line.size() == 1;
}


// TODO don't like this: get_size() returning int because it's same as also used
//      to signal failure. Should be std::size_t, and fail is 0.
PlatformSize::PlatformSize() : Command(std::bind_front(&PlatformSize::on_output, std::ref(*this)))
{
}

void PlatformSize::on_output(const std::string_view line)
{
  try
  {
    if (!line.empty())
      m_size = static_cast<int>(std::strtol(line.data(), nullptr, 10));
  }
  catch(const std::exception& e)
  {
    qCritical() << e.what();
    m_size = 0;
  }
}

int PlatformSize::get_size ()
{
  static const std::string file {"/sys/firmware/efi/fw_platform_size"};

  if (m_exists = fs::exists(file) ; m_exists)
    return execute(std::format("cat {}", file), 1) == CmdSuccess ? m_size : 0;
  else
    return CmdFail;
}

bool PlatformSize::platform_file_exist() const
{
  return m_exists;
}


// CPU vendor
CpuVendor::CpuVendor() :
  Command("cat /proc/cpuinfo | grep \"^model name\"", std::bind_front(&CpuVendor::on_output, std::ref(*this)))
{

}

void CpuVendor::on_output(const std::string_view line)
{
  if (line.find("AMD") != std::string::npos)
    m_vendor = Vendor::Amd;
  else if (line.find("Intel") != std::string::npos)
    m_vendor = Vendor::Intel;
}

CpuVendor::Vendor CpuVendor::get_vendor ()
{
  return execute(1) == CmdSuccess ? m_vendor : Vendor::None;
}


// Timezones
TimezoneList::TimezoneList() : Command("timedatectl list-timezones")
{
  
}

QStringList TimezoneList::get_zones()
{
  if (m_zones.empty())
  {
    m_zones.reserve(600); // "timedatectl list-timezones | wc -l" returns 598

    execute([this](const std::string_view m)
    {
      if (!m.empty())
        m_zones.emplace_back(QString::fromLocal8Bit(m.data(), m.size()));
    });
  }

  return m_zones;
}


// KeyMaps
KeyMaps::KeyMaps() : Command("localectl list-keymaps")
{

}

QStringList KeyMaps::get_list()
{
  if (m_keys.empty())
  {
    m_keys.reserve(250);

    execute([this](const std::string_view m)
    {
      if (!m.empty())
        m_keys.emplace_back(QString::fromLocal8Bit(m.data(), m.size()));
    });
  }
  
  return m_keys;
}


//
SysClockSync::SysClockSync() : Command("timedatectl")
{

}