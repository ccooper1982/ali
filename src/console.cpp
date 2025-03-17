
#include <sys/mount.h>
#include <string.h> 
#include <iostream>
#include <ali/console.hpp>
#include <ali/util.hpp>
#include <ali/commands.hpp>


struct Menu
{
  virtual void show () = 0;
  virtual ~Menu() {}
  
protected:
  int prompt (const std::string_view msg = "> ")
  {
    int i; 
    std::cout << "\n" << msg;   
    std::cin >> i;
    std::cin.ignore();
    return i;
  }

  std::string prompt_string (const std::string_view msg = "> ")
  {
    std::string s;
    std::cout << "\n" << msg;
    std::cin >> s;
    std::cin.ignore();

    return s;
  }
};


struct LocalesMenu : public Menu
{
  virtual void show () override
  {
    for (bool exit = false; !exit ;)
    {
      std::cout <<  "1. Set Locale\n" <<
                    "2. Back";
    
      if (const auto n = prompt(); n == 2)
        exit = true;      
      else if (n == 1)
      {
        std::cout << "Enter locale name\nCommon names: uk, us\n"
                  << "locale>";
        std::string name;
        std::cin >> name;

        if (!name.empty())
        {
          locale = name;
          exit = true;
        }
      }
    }
  }

  private:
    std::string locale;
};


struct PartitionsMenu : public Menu
{
  PartitionsMenu(std::shared_ptr<DiskTree> tree) : m_tree(tree)
  {

  }

  virtual void show () override
  {
    while (m_state != State::Complete)
    {
      switch (m_state)
      {
      case State::Boot:
      {
        if (m_boot_set = set_mount("boot (/boot)"); m_boot_set)
          m_state = State::Root;
      }
      break;

      case State::Root:
      {
        if (m_root_set = set_mount("root (/)"); m_root_set)
          m_state = State::Home;
      }
      break;

      case State::Home:
      {
        std::cout << "Use root partition for /home?\nEnter 1 to confirm or 0 to use a different partition";
        if (const int n = prompt(); n == 1)
        {
          m_tree->assign_home_to_root();
          m_home_is_root = m_home_set = true;
        }
        else if (n == 0)
        {
          if (m_home_set = set_mount("home (/home)"); m_home_set)
            m_state = State::Home;
        }

        if (m_home_set)
          m_state = State::Complete;
      }
      break;

      default:
      break;

      }
    }

    display_disk_tree();
    display_mount_points();
  }


private:
  bool set_mount(const std::string_view mount_name)
  {
    static const int64_t BootSizeWarning = 1024 * 1024 * 255;
    static const int64_t RootSizeWarning = (int64_t)(1024 * 1024) * (int64_t)8000;


    display_disk_tree();

    std::cout << "Enter partition path for " << mount_name << " << \n";
    const std::string entry = prompt_string();
    
    bool set = false;
    if (!entry.empty())
    {
      if (m_state == State::Boot)
      {
        if (!m_tree->is_partition_efi(entry))
        {
          std::cout << "ERROR: partition is not an EFI type\n";
        }
        else
        {
          if (m_tree->get_partition_size(entry) < BootSizeWarning)
            std::cout << "WARNING: " << entry << " size is less than 256MB. Not reommended.\n";
          
          set = m_tree->assign_boot(entry);
        }
      }
      else if (m_state == State::Root)
      {
        if (m_tree->get_partition_size(entry) < RootSizeWarning)
          std::cout << "WARNING: " << entry << " size is less than 8GB. This may be limiting for a desktop environment.\n";
        
        set = m_tree->assign_root(entry);
      }
      else if (m_state == State::Home)
        set = m_tree->assign_home(entry);

      if (!set)
        std::cout << "Error: does not exist or is not a partition\n";
    }

    return set;
  }

  void display_disk_tree()
  {
    for (const auto& disk : m_tree->get_disks())
    {
      std::cout << disk.first << '\n';

      for (const auto& part : disk.second)
      {
        std::cout << "  -- " << part.path << " : " << part.type << " : " << part.size;
        
        if (part.assigned)
          std::cout << "  [ASSIGNED]";

        std::cout << '\n';
      }
    }

    std::cout << '\n';
  }

  void display_mount_points()
  {
    std::cout << "------\n";
    std::cout << "boot -> " << m_tree->get_boot() << '\n'
              << "root -> " << m_tree->get_root() << '\n'
              << "home -> " << m_tree->get_home() << '\n';
    std::cout << "------\n";
  }

private:
  enum class State { Boot, Root, Home, Complete }; 

  bool m_home_is_root{true};
  bool m_home_set{false};
  bool m_root_set{false};  
  bool m_boot_set{false};
  State m_state{State::Boot};
  std::shared_ptr<DiskTree> m_tree;
};


struct InstallMenu : public Menu
{
  InstallMenu(std::shared_ptr<DiskTree> tree) : m_tree(tree)
  {

  }

private:  
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


  virtual void show () override
  {
    #ifdef ALI_DEV
      std::cout << "In dev mode, saving you.\n";
      return;
    #endif

    const bool ok = mount() && pacman_strap() && fstab() /*&& chroot()*/;

    if (ok)
    {
      // TODO 
      ChRootCmd tz {std::format("ln -sf /usr/share/zoneinfo/{} {}", "Europe/London", "/etc/localtime")};
      if (tz.execute() != CmdSuccess)
        std::cout << "ERROR: timezone config failed\n";
      else
      {
        if (ChRootCmd syncHwClock{"hwclock --systohc"}; syncHwClock.execute() != CmdSuccess)
          std::cout << "ERROR: clock set failed\n";
        else
        {
          // TODO check `timedatectl | grep -i "NTP service:" ` returns "NTP Service: Active"

          if (localisation())
          {
            if (!network_config())
              std::cout << "NOTE: network config failed, but continuing anyway";
            
            if (passwords())
            {
              // if (boot_loader())
              // {
              //   std::cout << "\n\n--- SUCCESS ---\n"
              //             << "You can reboot now.\n\n"
              //             << "Remember to remove installion media (USB) if applicable.\n"
              //             << "--------------\n";
              // }
            }
          }
        }
      }
    }
  }

  
private:
  bool do_mount(const std::string_view dev, const std::string_view path, const std::string_view fs)
  {
    if (!fs::exists(path))
      fs::create_directory(path);

    const int r = ::mount(dev.data(), path.data(), fs.data(), 0, nullptr) ;
    
    if (r != 0)
      std::cout << "ERROR for " << path << " : " << ::strerror(errno) << '\n';

    return r == 0;    
  }


  bool mount()
  {
    std::cout << __FUNCTION__ << '\n';

    /*
    auto do_mount = [](const std::string_view dev, const std::string_view path, const std::string_view fs)
    {
      if (!fs::exists(path))
        fs::create_directory(path);

      const int r = ::mount(dev.data(), path.data(), fs.data(), 0, nullptr) ;
      
      if (r != 0)
        std::cout << "ERROR for " << path << " : " << ::strerror(errno) << '\n';

      return r == 0;
    };
    */

    // if (is_dir_mounted(EfiMnt.string()))
    //   ::umount(EfiMnt.c_str());

    if (is_dir_mounted(BootMnt.string()))
      ::umount(BootMnt.c_str());

    if (is_dir_mounted(RootMnt.string()))
      ::umount(RootMnt.c_str());
    
    const bool mounted =  do_mount(m_tree->get_root().c_str(), RootMnt.c_str(), "ext4") &&
                          do_mount(m_tree->get_boot().c_str(), BootMnt.c_str(), "vfat")/* &&
                          do_mount(m_tree->get_boot().c_str(), EfiMnt.c_str(), "vfat")*/;
    
    if (mounted)
    {
      std::cout << "Mounted " << BootMnt << " -> " << m_tree->get_boot() << "\n";
      //std::cout << "Mounted " << EfiMnt  << " -> " << m_tree->get_boot() << "\n";
      std::cout << "Mounted " << RootMnt << " -> " << m_tree->get_root() << "\n";
    }

    return mounted;
  }


  bool pacman_strap()
  {
    std::cout << __FUNCTION__ << '\n';

    CpuVendor cmd_cpu_vendor;
    const CpuVendor::Vendor cpu_vendor = cmd_cpu_vendor.get_vendor();

    const auto cmd = std::format("pacstrap -K {} base linux linux-firmware {}", RootMnt.string(),
                                                                                cpu_vendor == CpuVendor::Vendor::Amd ? "amd-ucode" : "intel-ucode");


    bool firmware_warning = false;
    Command pacstrap {cmd, [&firmware_warning](const std::string_view out)
    {
      std::cout << out;

      if (out.find("Possibly missing firmware for module:") != std::string::npos)
        firmware_warning = true;
    }};
    
    pacstrap.trim_newline(false); // otherwise we wait for days to see progress
    
    bool ok = true;
    if (pacstrap.execute() != CmdSuccess)
    {
      std::cout << "ERROR: pacstrap failed - manual intervention required to fix before running the installer\n";
      ok = false;
    }
    else if (firmware_warning)
      std::cout << "NOTE: warnings of missing firmware can be fixed post-install\n";
  
    return ok;
  }


  bool fstab()
  {
    std::cout << __FUNCTION__ << '\n';

    fs::create_directory((RootMnt / FsTabPath).parent_path());

    Command fstab {std::format("genfstab -U {} > {}", RootMnt.string(), FsTabPath.string())};

    const bool ok = fstab.execute() == CmdSuccess && fs::exists(FsTabPath) && fs::file_size(FsTabPath);
    
    if (!ok)
      std::cout << "ERROR: fstab failed\n";
    
    return ok;
  }


  bool localisation()
  {
    std::cout << __FUNCTION__ << '\n';

    // TODO these
    
    bool ok = false;

    const std::string cmd_locale {std::format("echo \"LANG={}\" >> /etc/locale.conf", "en_GB.UTF-8")};
    const std::string cmd_console_keymap {std::format("echo \"KEYMAP={}\" >> /etc/vconsole.conf", "uk")};
    
    if (ChRootCmd set_locale {cmd_locale}; set_locale.execute() != CmdSuccess)
      std::cout << "ERROR: Setting locale.conf failed\n";
    else if (ChRootCmd set_console_keymap {cmd_console_keymap}; set_console_keymap.execute() != CmdSuccess)
      std::cout << "ERROR: Setting vconsole.conf failed\n";
    else
      ok = true;

    return ok;
  }


  bool network_config()
  {
    std::cout << __FUNCTION__ << '\n';

    std::string hostname = "archlinux"; // TODO

    const std::string cmd_host {std::format("echo \"{}\" >> /etc/hostname", hostname)};

    if (ChRootCmd set_hostname {cmd_host}; set_hostname.execute() != CmdSuccess)
    {
      std::cout << "ERROR: Setting hostname failed\n";
      return false;
    }
    else
    {
      // TODO network manager (https://wiki.archlinux.org/title/Network_configuration#Network_managers)
      // probably package: networkmanager
      return true;
    }
  }


  bool passwords()
  {
    std::cout << __FUNCTION__ << '\n';

    std::string root_passwd = "arch"; // TODO
    
    // unlock root: this was locked during dev and prevented log in, 
    //              so leave here to ensure it is usable.
    ChRootCmd cmd_unlock{std::format("passwd -u root", root_passwd)};
    cmd_unlock.execute();

    ChRootCmd cmd_passwd{std::format("echo \"{}\" | passwd --stdin", root_passwd), [](const std::string_view out)
    {
      std::cout << out;
    }};
    cmd_passwd.trim_newline(false);
    
    // TODO could use "passwd -S" to confirm second argument is 'P'

    if (cmd_passwd.execute() != CmdSuccess)
    {
      std::cout << "Set root password failed";
      return false;
    }
    else
      return true;
  }


  bool boot_loader()
  {
    std::cout << __FUNCTION__ << '\n';

    bool ok = false;

    // TODO (grub, efibootmgr) or (systemd)

    ChRootCmd install {"pacman -Sy --noconfirm grub efibootmgr", [](const std::string_view out)
    {
      std::cout << out; 
    }};

    install.trim_newline(false);

    if (install.execute() != CmdSuccess)
      std::cout << "ERROR: bootloader install failed. Do not reboot until fixed\n";
    else
    {
      const std::string cmd_init = std::format("grub-install --target=x86_64-efi --efi-directory=/boot --bootloader-id=GRUB");
      
      ChRootCmd grub_init{cmd_init, [](const std::string_view out)
      {
        std::cout << out;
      }};

      grub_init.trim_newline(false);

      if (grub_init.execute() != CmdSuccess)
        std::cout << "ERROR: GRUB initialise failed. Do not reboot until fixed\n";
      else
      {
        ChRootCmd grub_config{"grub-mkconfig -o /boot/grub/grub.cfg", [](const std::string_view out)
        {
          std::cout << out;
        }};
  
        grub_config.trim_newline(false);

        if (grub_config.execute() != CmdSuccess)
          std::cout << "ERROR: GRUB config failed. Do not reboot until fixed\n";
        else
          ok = true;
      }
    }

    return ok;
  }


private:
  std::shared_ptr<DiskTree> m_tree;
};


struct MainMenu : public Menu
{
  MainMenu() : m_tree(std::make_shared<DiskTree>())
  {
    m_menus = decltype(m_menus)
    {
      std::make_shared<LocalesMenu>(),
      std::make_shared<PartitionsMenu>(m_tree),
      std::make_shared<InstallMenu>(m_tree)
    };
  }
  
  virtual void show () override
  {
    *m_tree = create_disk_tree();

    for (bool exit = false; !exit ;)
    {
      std::cout <<  "1. Locales\n" <<
                    "2. Partitions and Mounts\n" <<
                    "3. Proceed\n" <<
                    "4. Exit";
    
      if (const auto n = prompt(); n == 4)
        exit = true;      
      else if (n >= 1 && n < 4)
      {
        m_menus[n-1]->show();
      }
    }
  }

private:
  std::vector<std::shared_ptr<Menu>> m_menus;
  std::shared_ptr<DiskTree> m_tree;
};



void Console::show ()
{
  MainMenu menu;
  menu.show();
}