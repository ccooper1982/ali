#include <ali/commands.hpp>
#include <ali/common.hpp>
#include <iostream>


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

Command::Command (const std::string_view cmd, std::function<void(const std::string_view)>&& on_output) :
  m_cmd(cmd),
  m_handler(std::move(on_output))
{
}

int Command::operator()()
{
  return execute();
}

int Command::execute (const std::string_view cmd, const int max_lines)
{
  m_executed = true;

  if (cmd.empty())
    return CmdSuccess;

  if (FILE * fd = ::popen(cmd.data(), "r"); fd)
  {
    char buff[1024];
    int n_lines{0};

    while (fgets(buff, sizeof(buff), fd) != nullptr)
    {
      if (m_handler)
      {
        std::string_view sv{buff};
        
        if (m_trim_newline && sv.ends_with('\n'))
          sv.remove_suffix(1);
        
        m_handler(sv);
      }        

      if (++n_lines == max_lines)
        break;
    }
    
    return pclose(fd);
  }
  else
    return CmdFail;
}

int Command::execute (const int max_lines)
{
  return execute(m_cmd, max_lines);
}


//
CommandExist::CommandExist (const std::string_view program) :
  Command(std::format("command -v {}", program), std::bind_front(&CommandExist::on_output, std::ref(*this)))
{
}
    
bool CommandExist::exists()
{
  execute();
  return !m_missing;
}

int CommandExist::operator()()
{
  return exists() ? CmdSuccess : -1;
} 

void CommandExist::on_output(const std::string_view line)
{
  // if program not found, fgets() has no work so this is not
  // called, but have this here for brevity
  m_missing = line.empty() || line.size() == 1;
}



//
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
    std::cerr << e.what() << '\n';
  }
}

int PlatformSize::get_size ()
{
  static const std::string file {"/sys/firmware/efi/fw_platform_size"};

  if (m_exists = fs::exists(file) ; m_exists)
    return execute(std::format("cat {}", file), 1) == CmdSuccess ? m_size : CmdFail;
  else
    return CmdFail;
}

bool PlatformSize::platform_file_exist() const
{
  return m_exists;
}

int PlatformSize::operator()()
{
  return get_size() ;
}


//
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

int CpuVendor::operator()()
{
  return get_vendor() == Vendor::None ? CmdFail : CmdSuccess;
}


//
TimezoneList::TimezoneList(std::vector<std::string>& zones) :
  Command("timedatectl list-timezones", std::bind_front(&TimezoneList::on_output, std::ref(*this))),
  m_zones(zones)
{
  m_zones.reserve(600); // "timedatectl list-timezones | wc -l" returns 597
}

void TimezoneList::on_output(const std::string_view line)
{
  if (!line.empty())
    m_zones.emplace_back(line);
}

int TimezoneList::operator()()
{
  get_zones();  
  return m_zones.empty() ? CmdFail : CmdSuccess;
}

void TimezoneList::get_zones()
{
  if (!executed())
    execute();
}