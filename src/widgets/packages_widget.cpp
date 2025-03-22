#include <ali/widgets/packages_widget.hpp>
#include <ali/commands.hpp>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QCheckBox>
#include <QBitArray>
#include <QtTypes>

#include <iostream> // temp



struct SelectPackagesWidget : public QWidget
{
  enum class Options { All, One, None };

  static SelectPackagesWidget * all_required(const QStringList& names)
  {
    return new SelectPackagesWidget(names, Options::All, -1);
  }

  static SelectPackagesWidget * one_required(const QStringList& names, const qsizetype required_index = 0)
  {
    return new SelectPackagesWidget(names, Options::One, required_index);
  }

  static SelectPackagesWidget * none_required(const QStringList& names)
  {
    return new SelectPackagesWidget(names, Options::None, -1);
  }

  
  SelectPackagesWidget(const QStringList& names, const Options opt, const qsizetype req_index)
  {
    auto selected = [this](const bool checked, const QString& val)
    {
      if (checked)
        m_selected.insert(val);
      else
        m_selected.remove(val);
    };

    QHBoxLayout * layout = new QHBoxLayout;
    
    m_buttons = new QButtonGroup{this};

    // ButtonGroup exclusive by default, so set false before adding buttons
    // otherwise only one will be checked
    if (opt != Options::One)
      m_buttons->setExclusive(false);


    qsizetype i = 0;
    for (const auto& name : names)
    {
      QCheckBox * chk = new QCheckBox(name);
      m_buttons->addButton(chk);

      chk->connect(chk, &QCheckBox::checkStateChanged, this, [this, chk, selected](const Qt::CheckState state)
      {        
        selected(chk->isChecked(), chk->text());
      });
      
      if (opt == Options::All)
        chk->setChecked(true);
      else if (opt == Options::One && i++ == req_index)
        chk->setChecked(true);

      layout->addWidget(chk);
    }
    
    // now set exclusive, there will be one checked in the loop
    m_buttons->setExclusive(opt == Options::One);

    setLayout(layout);
  }

  const QSet<QString>& get_selected() const 
  {
    return m_selected;
  }

  void disable_box (const QString& name)
  {
    for(auto btn : m_buttons->buttons())
    {
      if (btn->text() == name)
        btn->setDisabled(true);
    }
  }

private:
  QSet<QString> m_selected;
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
    m_required = SelectPackagesWidget::all_required({"base", cpu_ucode});
    
    QGroupBox * group_required = new QGroupBox("Required");
    group_required->setLayout(m_required->layout());    
    group_required->setEnabled(false);

    layout->addWidget(group_required);
  }
  
  {
    // https://wiki.archlinux.org/title/Kernel#Officially_supported_kernels
    m_kernels =  SelectPackagesWidget::one_required({"linux"});
    
    QGroupBox * group_kernels = new QGroupBox("Kernels");
    group_kernels->setLayout(m_kernels->layout());    

    layout->addWidget(group_kernels);
  }

  {
    m_firmware = SelectPackagesWidget::none_required({"linux-firmware"});
    
    QGroupBox * group_firmware = new QGroupBox("Firmware");
    group_firmware->setLayout(m_firmware->layout());    

    layout->addWidget(group_firmware);
  }
  
  setLayout(layout);
}


bool PackagesWidget::is_valid()
{
  const bool valid = !m_required->get_selected().empty() &&
                     !m_kernels->get_selected().empty(); 
  return valid;
}


PackageData PackagesWidget::get_data()
{
  // caller of this only deals with std types
  return PackageData {.required = m_required->get_selected(),
                      .kernels = m_kernels->get_selected(),
                      .firmware = m_firmware->get_selected()};
}