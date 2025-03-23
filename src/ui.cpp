
#include <iostream>
#include <fstream>
#include <map>
#include <net/if.h>
#include <arpa/inet.h>
#include <QDebug>
#include <ali/widgets/widgets.hpp>
#include <ali/commands.hpp>
#include <ali/common.hpp>


static const QString log_format{"%{type} - %{if-debug}%{function} - %{endif}%{message}"};

static QFile log_file{InstallLogPath};
static QFile log_file_alt{"./" / InstallLogPath.filename()};
static QTextStream log_stream;


void log_handler(const QtMsgType type, const QMessageLogContext& ctx, const QString& m)
{
  log_stream << qFormatLogMessage(type, ctx, m) << '\n';
  log_stream.flush();
}


void configure_log_file(QWidget * parent)
{
  // attempt to open log file in /var/log/ali,
  // if it fails (which it shouldn't on live ISO), but will
  // fail on dev when not run as sudo, so attempt path './'
  if (!fs::exists(InstallLogPath.parent_path()))
    fs::create_directory(InstallLogPath.parent_path());
  
  if (log_file.open(QFile::WriteOnly | QFile::Truncate))
  {
    log_stream.setDevice(&log_file);
  }
  else
  {
    const fs::path alt_path{fs::current_path() / InstallLogPath.filename()};

    std::string msg{"Cannot open preferred log file for writing:\n" + InstallLogPath.string() + '\n'};

    if (log_file_alt.open(QFile::WriteOnly | QFile::Truncate))
    {
      msg += "Using alternative:\n" + alt_path.string() ;
      log_stream.setDevice(&log_file_alt);
    }
    else
      msg += "Alternative failed: " + alt_path.string();

    QMessageBox::warning(parent, "Log", QString{msg.c_str()});
  }
  
  // custom log handler and formatter, logging to file
  qSetMessagePattern(log_format);
  qInstallMessageHandler(log_handler);
}


bool check_commands_exist ()
{
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
      qCritical() << "Command does not exist: " << cmd ;
      return false;
    }
  }
  
  return true;
}


bool check_platform_size ()
{
  bool valid = false;

  PlatformSize ps;
  if (const auto size = ps.get_size() ; size == 0)    
  {
    qCritical() << "Could not determine platform size";
    if (!ps.platform_file_exist())
      qCritical() << "You may have booted in BIOS or CSM mode";
  }    
  else if (size == 64)
    valid = true;
  else
    qCritical() << "Only 64bit supported";  

  return valid;
}


bool get_keymap (std::vector<std::string>& keys)
{
  // NOTE: this command is ran by the StartWidget, can probably remove it from here
  std::vector<std::string> list;

  KeyMaps cmd;
  cmd.get_list(list);
  return !list.empty();
}


bool check_cpu_vendor ()
{
  CpuVendor cmd;
  return cmd.get_vendor() != CpuVendor::Vendor::None;
}


bool check_connection()
{
  bool good = false;

  if (int sockfd = socket(AF_INET, SOCK_STREAM, 0); sockfd > -1)
  {
    const timeval timeout { .tv_sec = 5,
                            .tv_usec = 0};

    const sockaddr_in addr = {.sin_family = AF_INET,
                              .sin_port = htons(443),
                              .sin_addr = inet_addr("8.8.8.8")};
    
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    good = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == 0;
    
    if (good)
      close(sockfd);
  }
	
  if (!good)
    qCritical() << "Network connection failed";

  return good;
}


bool sync_system_clock()
{
  SysClockSync cmd;
  return cmd.execute() == CmdSuccess;
}


static std::tuple<bool, std::string> startup_checks()
{
  // auto check = []<typename F, typename... Args>(F f, Args... args)  -> bool
  //   requires (std::same_as<bool, std::invoke_result_t<Args...>>)
  // {
  //   return f(std::forward<Args>(args)...);
  // };

  auto fail = [](const std::string_view err = ""){ return std::make_tuple(false, std::string{err}); };

  auto check = []<typename F>(F f) -> bool requires (std::same_as<bool, std::invoke_result_t<F>>)
  {
    return f();
  };

  if (!check(check_cpu_vendor))
    return fail("CPU vendor not Intel/AMD, or not found");
  else if (!check(check_commands_exist))
    return fail("Not all required commands exist");
  else if (!check(check_platform_size))
    return fail("Platform size not found or not 64bit");
  else if (!check(check_connection))
    return fail("No active internet connection");
  else if (!check(sync_system_clock))
    return fail("Failed to call timedatectl or it failed");
  else
    return {true, ""};
}


struct NavTree : public QTreeView
{
public:

  explicit NavTree(QLayout * const centre_layout) : m_centre_layout(centre_layout)
  {
    m_model = new QStandardItemModel{0,1};

    for (const auto item : Widgets::all())
      m_model->appendRow(new QStandardItem(item->get_nav_name()));

    setModel(m_model);
    setHeaderHidden(true);
    setMinimumWidth(100);
    setMaximumWidth(150);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    setFrameStyle(QFrame::NoFrame);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection& selected, const QItemSelection& deselected)
    {
      if (const QModelIndexList selectedIndexes = selected.indexes() ; !selectedIndexes.isEmpty())
      {
        const QModelIndex index = selectedIndexes.first(); 
        const QVariant data = m_model->data(index, Qt::DisplayRole);         

        if (index.row() >= 0 && index.row() < std::ssize(Widgets::all()))
        {
          auto next = Widgets::all()[index.row()];

          if (m_current)
          {
            m_current->hide();
            m_centre_layout->replaceWidget(m_current, next, Qt::FindDirectChildrenOnly);
          }
          else
          {
            m_centre_layout->addWidget(next);
          }

          m_current = next;
          m_current->show();

          if (m_current->objectName() == Widgets::install()->get_nav_name())
          {
            // setFocus() so it triggers a validation check
            m_current->setFocus(Qt::FocusReason::ActiveWindowFocusReason);
          }
        }
      }
    });
  }

  ~NavTree()
  {
    for(auto& item : Widgets::all())
      delete item;
  }

  void show_welcome()
  {
    setCurrentIndex(model()->index(0,0));
  }

private:
  QLayout * m_centre_layout;
  QWidget * m_current{nullptr};
  QStandardItemModel * m_model;
  QModelIndex m_welcome_index;
};



int main (int argc, char ** argv)
{
  #if !defined(ALI_PROD) && !defined(ALI_DEV)
    static_assert(false, "ALI_PROD or ALI_DEV must be defined");  
  #endif
  
  QApplication app(argc, argv);
  QMainWindow window;

  // do this ASAP
  configure_log_file(&window);

  
  if (QStyleFactory::keys().contains("Fusion"))
  {
    // TODO Styling may not be worth the effort for some colors, except
    //      for those who require high contrast
    //  https://stackoverflow.com/questions/48256772/dark-theme-for-qt-widgets
    //  https://github.com/Alexhuszagh/BreezeStyleSheets
    QApplication::setStyle(QStyleFactory::create("Fusion"));
  }

  // main window contains a central widget which has a horizontal layout,
  // to which the Nav is added first. The layout is passed to the 
  // navigation tree so it can add/remove widgets as nav items are 
  // selected. 
  QHBoxLayout * centre_layout = new QHBoxLayout;
  NavTree * nav_tree = new NavTree(centre_layout);

  centre_layout->setContentsMargins(0,0,0,0);
  centre_layout->addWidget(nav_tree);

  QWidget * centre_widget = new QWidget;
  centre_widget->setContentsMargins(0,0,0,0);
  centre_widget->setLayout(centre_layout);

  
  window.setWindowTitle("ali");
  window.resize(800, 600);  // TODO may be unsuitable
  window.setCentralWidget(centre_widget);

  // always show Welcome initially
  nav_tree->show_welcome();

  window.show();

  // perform startup checks
  if (const auto [ok, err] = startup_checks(); !ok)
  {
    qCritical() << err;
    QMessageBox::critical(&window, "Cannot Install", QString{err.c_str()});
  }
  else
  {
    return app.exec();
  }
}


