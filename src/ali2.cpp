#include <iostream>  // for basic_ostream::operator<<, operator<<, endl, basic_ostream, basic_ostream<>::__ostream_type, cout, ostream
#include <memory>    // for shared_ptr, __shared_ptr_access
#include <string>    // for to_string, allocator
 
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Radiobox, Renderer, Tab, Toggle, Vertical
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for Element, separator, operator|, vbox, border


using namespace ftxui;

static const std::string Welcome_Waffle = "Some introductory waffle";

std::vector<std::string> keymaps = {" -- ", "gb", "fr", "de"};
int keymap_selected{0};

std::vector<std::string> locales = {" -- ", "en_GB.UTF-8", "en_US.UTF-8"};
int locale_selected{0};

std::vector<std::string> timezones = {" -- ", "Europe/London", "America/New York"};
int timezone_selected{0};


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

  //int tab_1_selected = 0;
  int tab_2_selected = 0;
  int tab_3_selected = 0;

  std::vector<std::string> tab_1_entries{"Forest","Water","I don't know"};
  std::vector<std::string> tab_2_entries{"Hello","Hi","Hay"};
  std::vector<std::string> tab_3_entries{"Table","Nothing","Is","Empty"};
  
  auto tab_container = Container::Tab(
  {
    Renderer([]{ return text(Welcome_Waffle);}),
    Container::Vertical(
      {
        Container::Horizontal({Renderer([]{ return text("Keymap") | vcenter | size(WIDTH, EQUAL, 15) ; }), Dropdown(&keymaps, &keymap_selected) | size(WIDTH, EQUAL, 25)}),
        Container::Horizontal({Renderer([]{ return text("Locale") | vcenter | size(WIDTH, EQUAL, 15); }), Dropdown(&locales, &locale_selected) | size(WIDTH, EQUAL, 25)}),
        Container::Horizontal({Renderer([]{ return text("Timezone") | vcenter | size(WIDTH, EQUAL, 15); }), Dropdown(&timezones, &timezone_selected) | size(WIDTH, EQUAL, 25)})
      }),
    Radiobox(&tab_2_entries, &tab_2_selected),
    Radiobox(&tab_3_entries, &tab_3_selected)
  },
  &tab_selected);

  auto container = Container::Vertical(
  {
    tab_toggle,
    tab_container,
  });

  auto renderer = Renderer(container, [&]
  {
    return vbox(
            {
              tab_toggle->Render(),
              separator(),
              tab_container->Render(),
            }) |
            border;
  });
  
  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(renderer);
  
  return 0;
}