
//#define _GNU_SOURCE
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <format>
#include <iostream>
#include <filesystem>
#include <map>
#include <net/if.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ali/console.hpp>
#include <ali/commands.hpp>


bool check_commands_exist ()
{
  std::cout << __FUNCTION__ << '\n';

  static const std::vector<std::string> Commands =
  {
    "pacman", "localectl", "locale-gen", "loadkeys", "setfont", "timedatectl", "ip", "lsblk", 
    "mount", "swapon", "ln", "hwclock", "chpasswd", "passwd"

    #ifdef ALI_PROD
      ,"pacstrap", "genfstab", "arch-chroot"
    #endif
  };
  
  for (const auto& cmd : Commands)
  {
    if (CommandExist command {cmd}; !command.exists())
    {
      std::cout << "Does not exist: " << cmd << '\n';
      return false;
    }
  }
  
  return true;
}


bool check_platform_size ()
{
  std::cout << __FUNCTION__ << '\n';
  
  bool valid = false;

  PlatformSize ps;
  if (const auto size = ps.get_size() ; size == 0)    
  {
    std::cout << "Could not determine platform size";
    if (!ps.platform_file_exist())
      std::cout << "You may have booted in BIOS or CSM mode";
  }    
  else if (size == 64)
    valid = true;
  else
    std::cout << "Only 64bit supported\n";  

  return valid;
}


bool get_keymap (std::vector<std::string>& keys)
{
  std::cout << __FUNCTION__ << '\n';

  Command cmd{"localectl list-keymaps", [&keys](std::string_view line)
  {
    keys.emplace_back(line);
  }};

  return cmd.execute() == CmdSuccess;
}


bool get_cpu_vendor ()
{
  std::cout << __FUNCTION__ << '\n';

  CpuVendor cmd;
  return cmd.get_vendor() != CpuVendor::Vendor::None;
}



// network
struct NetworkInf
{
  std::string name;
  std::string ip;
};


//std::map<unsigned int, std::string> networkInterfaces;
std::vector<NetworkInf> networkInterfaces2;
std::vector<std::string> keysMap;


bool get_network_interfaces()
{
  ifaddrs * addr_head;
  
  if (::getifaddrs(&addr_head) == -1) 
    return false;

  for (auto node = addr_head; node ; node = node->ifa_next)
  {
    if (node->ifa_addr == nullptr)
      continue;

    const int family = node->ifa_addr->sa_family;

    if (family == AF_INET || family == AF_INET6)
    {
      char host[NI_MAXHOST];

      if (getnameinfo(node->ifa_addr,
                      family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6),
                      host, NI_MAXHOST,
                      nullptr, 0,  // dont want service name
                      NI_NUMERICHOST) == 0)
      {
        networkInterfaces2.emplace_back(node->ifa_name, host);
      }
    }
  }

  ::freeifaddrs(addr_head);

  return true;
  
  // if (struct if_nameindex * if_ni = ::if_nameindex(); if_ni == nullptr)
  //   return false;
  // else
  // {
  //   for (auto i = if_ni; !(i->if_index == 0 && i->if_name == NULL); ++i)
  //     networkInterfaces.emplace(i->if_index, i->if_name);

  //   ::if_freenameindex(if_ni);
  
  //   return true;
  // }
}


bool check_connection()
{
  std::cout << __FUNCTION__ << '\n';

  bool good = false;

  if (int sockfd = socket(AF_INET, SOCK_STREAM, 0); sockfd > -1)
  {
    const timeval timeout { .tv_sec = 1,
                            .tv_usec = 0};

    const sockaddr_in addr = {.sin_family = AF_INET,
                              .sin_port = htons(443),
                              .sin_addr = inet_addr("8.8.8.8")};
    
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    good = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == 0;

    close(sockfd);
  }
	
  if (!good)
    std::cout << "Network connection failed\n";

  return good;
}


bool init()
{
  return  check_commands_exist() &&
          check_platform_size() &&
          get_keymap(keysMap) &&
          get_cpu_vendor() &&
          check_connection();
}


int main (int argc, char ** argv)
{
  //static Command CmdSetFont {"setfont ter-132b"}; // probably don't need this since we'll be firing up GTK
  
  if (!init())
  {
    return -1;
  }
  else
  {
    Console c;
    c.show();

    /*
    if (get_network_interfaces())
    {
      for (const auto& inf : networkInterfaces2)
        std::cout << inf.name << " = " << inf.ip << '\n';

      // std::for_each(std::cbegin(networkInterfaces), std::cend(networkInterfaces), [](const auto& pair)
      // {
      //   std::cout << pair.first << ":" << pair.second;
      // });
    }
    */
  }

  return 0;
}
