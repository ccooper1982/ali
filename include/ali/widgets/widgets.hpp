#ifndef ALI_WIDGETS_H
#define ALI_WIDGETS_H

#include <ali/widgets/content_widget.hpp>
#include <ali/widgets/start_widget.hpp>
#include <ali/widgets/partitions_widget.hpp>
#include <ali/widgets/network_widget.hpp>
#include <ali/widgets/accounts_widget.hpp>
#include <ali/widgets/packages_widget.hpp>
#include <ali/widgets/swap_widget.hpp>
#include <ali/widgets/profile_widget.hpp>
#include <ali/widgets/install_widget.hpp>


// A struct which contains instances of all
// the widgets. 
//
// These is this because the InstallWidget requires 
// access to each widget to request their data.
// 
// - This can't be a factory style templated function because
//   we only want one instance of each widget
// - A QWidget cannot be created prior to instanciating
//   QApplication
struct Widgets
{
  static StartWidget * start()
  {
    static auto * w = new StartWidget;
    return w;
  }

  static PartitionsWidget * partitions()
  {
    static auto * w = new PartitionsWidget;
    return w;
  }

  static SwapWidget * swap()
  {
    static auto * w = new SwapWidget;
    return w;
  }

  static NetworkWidget * network()
  {
    static auto * w = new NetworkWidget;
    return w;
  }

  static AccountsWidget * accounts()
  {
    static auto * w = new AccountsWidget;
    return w;
  }

  static ProfileWidget * profile()
  {
    static auto * w = new ProfileWidget;
    return w;
  }

  static PackagesWidget * packages()
  {
    static auto * w = new PackagesWidget;
    return w;
  }

  static InstallWidget * install()
  {
    static auto * w = new InstallWidget;
    return w;
  }

  static const std::vector<ContentWidget*>& all()
  {
    static const std::vector<ContentWidget*> widgets = 
    {
      Widgets::start(),
      Widgets::partitions(),
      Widgets::swap(),
      Widgets::network(),
      Widgets::accounts(),
      Widgets::profile(),
      Widgets::packages(),
      Widgets::install()
      // Profile, Audio, Video
    };

    return widgets;
  }
};

#endif
