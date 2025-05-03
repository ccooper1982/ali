#include <iostream>  // for basic_ostream::operator<<, operator<<, endl, basic_ostream, basic_ostream<>::__ostream_type, cout, ostream
#include <memory>    // for shared_ptr, __shared_ptr_access
#include <string>    // for to_string, allocator
 
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Radiobox, Renderer, Tab, Toggle, Vertical
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for Element, separator, operator|, vbox, border


using namespace ftxui;


std::string debug;



static const std::string Welcome_Waffle = "Some introductory waffle";

std::vector<std::string> keymaps = {" -- ", "gb", "fr", "de"};
int keymap_selected{0};

std::vector<std::string> locales = {" -- ", "en_GB.UTF-8", "en_US.UTF-8"};
int locale_selected{0};

std::vector<std::string> timezones = {" -- ", "Europe/London", "America/New York"};
int timezone_selected{0};

std::vector<std::string> devices = {" -- ", "/dev/sda1", "/dev/sda2"};

std::vector<std::string> parts1 = {"/dev/sda1p1", "/dev/sda1p1"};
std::vector<std::string> parts2 = {"/dev/sda2p1", "/dev/sda2p1"};
std::vector<std::string> parts;

struct Mounts : public ComponentBase
{
  Mounts()
  {
    devices_dd = Dropdown(&devices, &m_selected_device) | size(WIDTH, EQUAL, 25);
    devices_dd |= CatchEvent([this](Event evt)
    {
      if (m_selected_device == 1)
        parts = parts1;
      else if (m_selected_device == 2)
        parts = parts2;
      else
        parts.clear();
      
      return false;
    });

    auto dev = Container::Horizontal(
    {
      Renderer([]{ return text("Device") | vcenter | size(WIDTH, EQUAL, 15) ; }),
      devices_dd
    });


    partitions_dd = Dropdown(&parts, &m_selected_part) | size(WIDTH, EQUAL, 25);

    partitions = Maybe(Container::Horizontal(
    {
      Renderer([]{ return text("Partitions") | vcenter | size(WIDTH, EQUAL, 15) ; }),
      partitions_dd
    }), [this]{ return m_selected_device != 0;}) ;


    auto layout = Container::Vertical({dev, partitions});

    Add(layout);
  }

  virtual bool OnEvent(Event evt) override
  {
    //debug = "Mounts::OnEvent";
    return ComponentBase::OnEvent(evt); 
  }

private:
  Component devices_dd;
  Component partitions_dd;
  Component partitions;
  int m_selected_device{0};
  int m_selected_part{0};
};



int main (int argc, char ** argv)
{
  std::vector<std::string> tab_values{
    "Start",
    "Locale",
    "Mounts",
    "Network",
    "Swap",
  };
  int tab_selected = 0;
  auto tab_toggle = Toggle(&tab_values, &tab_selected);

  auto mounts = ftxui::Make<Mounts>();

  auto tab_container = Container::Tab(
  {
    Renderer([]{ return text(Welcome_Waffle);}),
    Container::Vertical(
    {
      Container::Horizontal({Renderer([]{ return text("Keymap") | vcenter | size(WIDTH, EQUAL, 15) ; }), Dropdown(&keymaps, &keymap_selected) | size(WIDTH, EQUAL, 25)}),
      Container::Horizontal({Renderer([]{ return text("Locale") | vcenter | size(WIDTH, EQUAL, 15); }), Dropdown(&locales, &locale_selected) | size(WIDTH, EQUAL, 25)}),
      Container::Horizontal({Renderer([]{ return text("Timezone") | vcenter | size(WIDTH, EQUAL, 15); }), Dropdown(&timezones, &timezone_selected) | size(WIDTH, EQUAL, 25)})
    }),
    Container::Vertical(
    {
      mounts
    }),
    std::make_shared<ComponentBase>(),
    std::make_shared<ComponentBase>()
  },
  &tab_selected);

  auto container = Container::Vertical(
  {
    tab_toggle,
    tab_container,
  });

  
  auto screen = ScreenInteractive::Fullscreen();


  auto renderer = Renderer(container, [&]
  {
    return vbox(
            {
              text(debug),
              tab_toggle->Render(),
              separator(),
              tab_container->Render(),
            }) | border;
  });
  

  renderer |= CatchEvent([&](Event evt)
  {
    static int n_q{0};
    
    n_q = (evt == Event::q ? n_q+1 : 0);

    if (n_q == 2)
    {      
      screen.Exit();
      return true;
    }
    else
      return false;
  });

  screen.Loop(renderer);
  
  return 0;
}