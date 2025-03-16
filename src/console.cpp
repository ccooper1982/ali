
#include <iostream>
#include <ali/console.hpp>
#include <ali/util.hpp>


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


struct SetMount : public Menu
{
  virtual void setRoot ()
  {
    m_prompt = "/";

    for (bool exit = false; !exit ;)
    {
      std::cout <<  "1. Set same as root\n" <<
                    "2. Set separate partion\n" <<
                    "3. Back";
    }
  }

  virtual void setBoot ()
  {
    m_prompt = "/boot";
  }

private:
  virtual void show () override
  {
  }

  std::string m_prompt;
};


struct PartitionsMenu : public Menu
{
  virtual void show () override
  {
    m_tree = create_disk_tree();
    
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
          m_tree.assign_home_to_root();
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
        if (!m_tree.is_partition_efi(entry))
        {
          std::cout << "ERROR: partition is not an EFI type\n";
        }
        else
        {
          if (m_tree.get_partition_size(entry) < BootSizeWarning)
            std::cout << "WARNING: " << entry << " size is less than 256MB. Not reommended.\n";
          
          set = m_tree.assign_boot(entry);
        }
      }
      else if (m_state == State::Root)
      {
        if (m_tree.get_partition_size(entry) < RootSizeWarning)
          std::cout << "WARNING: " << entry << " size is less than 8GB. This may be limiting for a desktop environment.\n";
        
        set = m_tree.assign_root(entry);
      }
      else if (m_state == State::Home)
        set = m_tree.assign_home(entry);

      if (!set)
        std::cout << "Error: does not exist or is not a partition\n";
    }

    return set;
  }

  void display_disk_tree()
  {
    for (const auto& disk : m_tree.get_disks())
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
    std::cout << "boot -> " << m_tree.get_boot() << '\n'
              << "root -> " << m_tree.get_root() << '\n'
              << "home -> " << m_tree.get_home() << '\n';
    std::cout << "------\n";
  }

private:
  enum class State { Boot, Root, Home, Complete }; 

  bool m_home_is_root{true};
  bool m_home_set{false};
  bool m_root_set{false};  
  bool m_boot_set{false};
  State m_state{State::Boot};
  DiskTree m_tree;
};


struct MainMenu : public Menu
{
  MainMenu()
  {
    menus = std::vector<Menu*>
    {
      new LocalesMenu,
      new PartitionsMenu
    };
  }

  virtual void show () override
  {
    for (bool exit = false; !exit ;)
    {
      std::cout <<  "1. Locales\n" <<
                    "2. Partitions and Mounts\n" <<
                    "3. Exit";
    
      if (const auto n = prompt(); n == 3)
        exit = true;      
      else if (n >= 1 && n < 3)
      {
        menus[n-1]->show();
      }
    }
  }

private:
  std::vector<Menu*> menus;
};



void Console::show ()
{
  MainMenu menu;
  menu.show();
}