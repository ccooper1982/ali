#include <ali/widgets/packages_widget.hpp>
#include <ali/commands.hpp>
#include <ali/packages.hpp>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QCheckBox>
#include <QBitArray>
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


enum class Type { Kernel, Firmware, Required, Important, Shell };


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
  
  setLayout(layout);
}


bool PackagesWidget::is_valid()
{
  return Packages::have_kernel() && Packages::have_required();
}

