#ifndef ALI_COMMANDS_H
#define ALI_COMMANDS_H

#include <string>
#include <string_view>
#include <functional>
#include <format>



inline const int CmdSuccess = 0;
inline const int CmdFail = -1;


struct Command
{
  Command ();
  
  Command (std::function<void(const std::string_view)>&& on_output) ;

  Command (const std::string_view cmd) ;
  
  Command (const std::string_view cmd, std::function<void(const std::string_view)>&& on_output) ;
  
  virtual int operator()();
  
  int execute (const std::string_view cmd, const int max_lines = -1);
  int execute (const int max_lines = -1);
  
private:
  std::string m_cmd;
  std::function<void(const std::string_view)> m_handler;
};


struct CommandExist : public Command
{
public:
  CommandExist (const std::string_view program) ;
  bool exists();


private:
  virtual int operator()();
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

  virtual int operator()();

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

  virtual int operator()();

private:
  Vendor m_vendor {Vendor::None};
};



#endif