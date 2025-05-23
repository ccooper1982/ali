#include <ali/commands.hpp>
#include <ali/common.hpp>
#include <iostream>
#include <functional>
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

Command::~Command()
{
  close();
}

int Command::execute (const std::string_view cmd, const int max_lines)
{
  m_executed = true;

  if (cmd.empty())
    return CmdSuccess;

  // redirect stderr to stdout (pacstrap, and perhaps others, output errors to stderr)
  if (m_fd = ::popen(std::format("{} {}", cmd, "2>&1").data(), "r"); m_fd)
  {
    char buff[4096];
    int n_lines{0};
    size_t n_chars{0};
    
    for (char c ; ((c = fgetc(m_fd)) != EOF) && n_chars < sizeof(buff); )
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

    m_result = close();

    if (m_result != CmdSuccess)
    {
      qCritical() << "Command {" << cmd << "} failed: " << ::strerror(m_result);
    }
    
    return m_result;
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
  if (!m_fd)
    m_fd = ::popen(m_cmd.data(), "w");
    
  if (m_fd)
  {
    fputs(s.data(), m_fd);
    return close();
  }
  
  return CmdFail;
}


bool Command::start_write(const std::string_view cmd)
{
  qDebug() << "opening pipe for " << cmd;

  m_fd = ::popen(m_cmd.data(), "w");

  qDebug() << (m_fd == nullptr ? "Failed open" : "Ok open");

  return m_fd != nullptr;
}


bool Command::write (const std::string_view s)
{
  if (m_fd)
  {
    qDebug() << "pipe open for: " << s;
    const bool ok = fputs(s.data(), m_fd) != EOF;
    qDebug() << (ok ? "fputs ok" : "fputs fail");
    return ok;
  }
  else
  {
    qCritical() << "pipe closed";
    return false;
  }
}

void Command::end_write()
{
  qDebug() << "closing pipe";
  close();
}

int Command::close()
{
  int r = CmdSuccess;

  if (m_fd)
  {
    r = ::pclose(m_fd);
    m_fd = nullptr;
  }
  
  return r;
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
GetCpuVendor::GetCpuVendor() :
  Command("cat /proc/cpuinfo | grep \"^model name\"", std::bind_front(&GetCpuVendor::on_output, std::ref(*this)))
{

}

void GetCpuVendor::on_output(const std::string_view line)
{
  if (line.find("AMD") != std::string::npos)
    m_vendor = CpuVendor::Amd;
  else if (line.find("Intel") != std::string::npos)
    m_vendor = CpuVendor::Intel;
}

CpuVendor GetCpuVendor::get_vendor ()
{
  return execute(1) == CmdSuccess ? m_vendor : CpuVendor::None;
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
    m_keys.reserve(250); // `localectl list-keymaps | wc -l` returns 247

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


//
VideoVendor::VideoVendor() : Command("lshw -C display | grep 'vendor: '")
{

}


GpuVendor VideoVendor::get_vendor()
{
  GpuVendor vendor{GpuVendor::Unknown};

  const int res = execute([this](const std::string_view out)
  {
    if (!out.empty())
    {
      n_amd += ::strcasestr(out.data(), "amd") ? 1 : 0;
      n_nvidia += ::strcasestr(out.data(), "nvidia") ? 1 : 0;
      n_vm += ::strcasestr(out.data(), "vmware") ? 1 : 0; 
      n_intel += ::strcasestr(out.data(), "intel") ? 1 : 0;
    }
  });
  
  // if more than one identified, leave as unknown
  if (res == CmdSuccess && n_amd + n_nvidia + n_vm + n_intel == 1)
  {
    if (n_amd)
      vendor = GpuVendor::Amd;
    else if (n_nvidia)
      vendor = GpuVendor::Nvidia;
    else if (n_intel)
      vendor = GpuVendor::Intel;
    else
      vendor = GpuVendor::VM;
  }
  
  return vendor;
}
