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

#include <iostream> // temp



enum class Type { Kernel, Firmware, Required };


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

  CpuVendor cpu_vendor_cmd;

  const CpuVendor::Vendor cpu_vendor = cpu_vendor_cmd.get_vendor();
  const QString cpu_ucode = cpu_vendor == CpuVendor::Vendor::Amd ? "amd-ucode" : "intel-ucode";

  {
    m_required = SelectPackagesWidget::all_required({"base", cpu_ucode, "sudo", "iwd"}, Type::Required);
    
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

  // TODO removed: replace with user-typed package names
  // { 
  //   m_recommended = SelectPackagesWidget::none_required({{"nano",true}, {"git",false}});
    
  //   QGroupBox * group_recommended = new QGroupBox("Recommended");
  //   group_recommended->setLayout(m_recommended->layout());    

  //   layout->addWidget(group_recommended);
  // }
  
  setLayout(layout);
}


bool PackagesWidget::is_valid()
{
  return Packages::have_kernel() && Packages::have_required();
}

