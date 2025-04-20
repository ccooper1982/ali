#include <ali/widgets/packages_widget.hpp>
#include <ali/commands.hpp>
#include <ali/packages.hpp>
#include <barrier>
#include <QtTypes>


static const QStringList Required =
{
  "base",
  "usb_modeswitch", // for usb devices that can switch modes (recharging/something else)
  "usbmuxd",
  "usbutils",
  "reflector",      // pacman mirrors list
  "dmidecode",      // BIOS / hardware info, may not actually be useful for most users
  "e2fsprogs",      // useful
  "gpm",            // laptop touchpad support
  "less"            // useful
};


// TODO this could be moved to Packages.hpp and renamed to PackageType
enum class Type { Kernel, Firmware, Required, Important, Shell };


struct AdditionalPackagesWidget : public QWidget
{
  AdditionalPackagesWidget (QWidget* parent = nullptr) : QWidget(parent)
  {
    auto layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignTop);

    auto form_layout = new QFormLayout;
    form_layout->setFormAlignment(Qt::AlignTop);
    form_layout->setAlignment(Qt::AlignTop);
    form_layout->setVerticalSpacing(10);
        
    form_layout->addRow(new QLabel{"Type package names separated by a space, i.e. 'git firefox'. If any do not exist they remain in the text box."});

    {
      auto * search_pkg_layout = new QHBoxLayout;
      search_pkg_layout->setAlignment(Qt::AlignTop);
      search_pkg_layout->setSpacing(10);

      m_package_names = new QLineEdit;
      m_package_names->setFixedWidth(300);

      m_search_btn = new QPushButton{"Search"};
      m_search_btn->setFixedWidth(130);

      connect(&package_search, &PackageSearch::on_complete, this, [this](const QStringList exist, const QStringList invalid)
      {
        m_package_names->setText(invalid.join(' '));
                
        Packages::add_additional(exist);
        
        refresh_to_install();

        m_search_btn->setEnabled(true);
        m_rmv_btn->setEnabled(true);
      });
      
      connect(m_search_btn, &QPushButton::clicked, this, &AdditionalPackagesWidget::do_search);
      connect(m_package_names, &QLineEdit::returnPressed, this, &AdditionalPackagesWidget::do_search);


      search_pkg_layout->addWidget(m_package_names);
      search_pkg_layout->addWidget(m_search_btn);

      form_layout->addRow("Packages", search_pkg_layout);
    }

    {
      auto * confirmed_layout = new QHBoxLayout;
      confirmed_layout->setAlignment(Qt::AlignTop);
      confirmed_layout->setSpacing(10);
      
      m_confirmed_packages = new QListWidget;
      m_confirmed_packages->setFixedWidth(200);
      m_confirmed_packages->setMaximumHeight(200);
      m_confirmed_packages->setSelectionBehavior(QAbstractItemView::SelectRows);
      m_confirmed_packages->setSelectionMode(QAbstractItemView::MultiSelection);

      m_rmv_btn = new QPushButton{"Remove Selected"};
      m_rmv_btn->setFixedWidth(130);

      connect(m_rmv_btn, &QPushButton::clicked, this, [this]()
      {
        QStringList remove;

        for (const auto item : m_confirmed_packages->selectedItems())
          remove.append(item->text());

        if (!remove.isEmpty())
        {
          Packages::remove_additional(remove);
          refresh_to_install();
        }
      });

      confirmed_layout->addWidget(m_confirmed_packages);
      confirmed_layout->addWidget(m_rmv_btn, 0, Qt::AlignTop);

      form_layout->addRow("To Install", confirmed_layout);
    }
        
    layout->addLayout(form_layout);
    layout->addStretch(1);

    setLayout(layout);
  }


private:
  void do_search()
  {
    if (m_package_names->text().isEmpty()) 
      return;

    m_rmv_btn->setEnabled(false);
    m_search_btn->setEnabled(false);

    try
    {
      package_search.search(m_package_names->text());
    }
    catch(const std::exception& e)
    {
      qCritical() << e.what();
    }
  }
  

  void refresh_to_install()
  {
    // clear the list, and add again reading from Packages.
    // Packages uses a set so we avoid duplicates and the UI will reflect
    // exactly what will be installed
    m_confirmed_packages->clear();
    for (const auto& pkg : Packages::additional())
      m_confirmed_packages->addItem(pkg.name);
  }


private:  
  QLineEdit * m_package_names;
  QListWidget * m_confirmed_packages;
  PackageSearch package_search;
  QPushButton * m_search_btn, * m_rmv_btn;
};


struct SelectPackagesWidget : public QWidget
{  
  enum class Options { All, One, None };
  

  static SelectPackagesWidget * all_required(const QStringList& names, const Type type)
  {
    return new SelectPackagesWidget(names, Options::All, type);
  }

  static SelectPackagesWidget * one_required(const QStringList& names, const Type type)
  {
    return new SelectPackagesWidget(names, Options::One, type);
  }

  static SelectPackagesWidget * none_required(const QStringList& names, const Type type)
  {
    return new SelectPackagesWidget(names, Options::None, type);
  }

  static SelectPackagesWidget * none_required(const QMap<QString, bool>& names, const Type type)
  {
    auto * widget = new SelectPackagesWidget(names.keys(), Options::None, type);

    for (const auto& package : names.keys(true))
    {
      if (auto * btn = widget->findChild<QCheckBox*>(package); btn)
        btn->setChecked(true);
    }

    return widget;
  }


  SelectPackagesWidget(const QStringList& names, const Options opt, const Type type)
  {
    auto selected = [this, type](const bool checked, const QString& val)
    {
      switch (type)
      {
        using enum Type;

        case Kernel:
          checked ? Packages::add_kernel(val) : Packages::remove_kernel(val);
        break;

        case Required:
          checked ? Packages::add_required(val) : Packages::remove_required(val);
        break;

        case Firmware:
          checked ? Packages::add_firmware(val) : Packages::remove_firmware(val);
        break;

        case Important:
          checked ? Packages::add_important({val}) : Packages::remove_important({val});
        break;

        case Shell:
          Packages::set_shell(val);
        break;

        default:
          // n/a
        break;
      }
    };

    QHBoxLayout * layout = new QHBoxLayout;
    
    m_buttons = new QButtonGroup{this};

    // ButtonGroup is exclusive by default, so set false before adding buttons
    // otherwise only one will be checked
    if (opt != Options::One)
      m_buttons->setExclusive(false);


    qsizetype i = 0;
    for (const auto& name : names)
    {
      QCheckBox * chk = new QCheckBox(name);
      chk->setObjectName(name);
      m_buttons->addButton(chk);

      chk->connect(chk, &QCheckBox::checkStateChanged, this, [this, chk, selected](const Qt::CheckState state)
      {        
        selected(chk->isChecked(), chk->text());
      });
      
      if (opt == Options::All)
        chk->setChecked(true);
      else if (opt == Options::One && i++ == 0) // required is always the first package
        chk->setChecked(true);

      layout->addWidget(chk);
    }
    
    // now set exclusive, there will be one checked in the loop
    m_buttons->setExclusive(opt == Options::One);

    setLayout(layout);
  }

    
  void select (const QStringList& packages)
  {
    for (const auto& package : packages)
    {
      if (auto * btn = m_buttons->findChild<QCheckBox*>(package); btn)
        btn->setChecked(true);
    }
  }


private:
  QButtonGroup * m_buttons;
};


PackagesWidget::PackagesWidget() : ContentWidget("Packages")
{
  QVBoxLayout * layout = new QVBoxLayout;
  layout->setAlignment(Qt::AlignTop);


  {
    m_required = SelectPackagesWidget::all_required(Required, Type::Required);
    
    QGroupBox * group_required = new QGroupBox("Required");
    group_required->setLayout(m_required->layout());    
    group_required->setEnabled(false);

    layout->addWidget(group_required);
  }
  
  {
    // TODO https://wiki.archlinux.org/title/Kernel#Officially_supported_kernels
    m_kernels =  SelectPackagesWidget::one_required({"linux"}, Type::Kernel);
    
    QGroupBox * group_kernels = new QGroupBox("Kernels");
    group_kernels->setLayout(m_kernels->layout());    

    layout->addWidget(group_kernels);
  }

  {
    m_firmware = SelectPackagesWidget::none_required({"linux-firmware", "linux-firmware-marvell"}, Type::Firmware);
    
    QGroupBox * group_firmware = new QGroupBox("Firmware");
    group_firmware->setLayout(m_firmware->layout());    

    layout->addWidget(group_firmware);
  }
  
  { 
    GetCpuVendor cpu_vendor_cmd;
    const CpuVendor cpu_vendor = cpu_vendor_cmd.get_vendor();
    const QString cpu_ucode = cpu_vendor == CpuVendor::Amd ? "amd-ucode" : "intel-ucode";
    
    m_important = SelectPackagesWidget::none_required({{"sudo",true}, {cpu_ucode, true}, {"nano", true}, {"wireless-regdb", true}, {"openssh", true}}, Type::Important);
    
    QGroupBox * group_important = new QGroupBox("Important");
    group_important->setLayout(m_important->layout());    

    layout->addWidget(group_important);
  }

  {
    m_shell = SelectPackagesWidget::one_required({"bash", "zsh", "ksh", "fish", "nushell"}, Type::Shell);
    
    QGroupBox * group_shell = new QGroupBox("Shell");
    group_shell->setLayout(m_shell->layout());

    layout->addWidget(group_shell);
  }

  {
    m_additional = new AdditionalPackagesWidget;
    
    QGroupBox * group_additional = new QGroupBox("Additional");
    group_additional->setLayout(m_additional->layout());

    layout->addWidget(group_additional);
  }
  
  setLayout(layout);
}


bool PackagesWidget::is_valid()
{
  return Packages::have_kernel() && Packages::have_required();
}

