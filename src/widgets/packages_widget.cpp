#include <ali/widgets/packages_widget.hpp>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QCheckBox>
#include <QBitArray>
#include <QtTypes>


struct SelectPackagesWidget : public QWidget
{
  SelectPackagesWidget(const QStringList& names, const bool required)
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
    m_buttons->setExclusive(required);

    qsizetype i = 0;
    for (const auto& name : names)
    {
      QCheckBox * chk = new QCheckBox(name);
      m_buttons->addButton(chk);

      chk->connect(chk, &QCheckBox::checkStateChanged, this, [this, chk, selected](const Qt::CheckState state)
      {        
        selected(chk->isChecked(), chk->text());
        qDebug() << m_selected;
      });
      
      if (i++ == 0 && required)
        chk->setChecked(true);

      layout->addWidget(chk);
    }
    
    setLayout(layout);
  }


  const QSet<QString>& get_selected() const 
  {
    return m_selected;
  }

private:
  QSet<QString> m_selected;
  QButtonGroup * m_buttons;
};


PackagesWidget::PackagesWidget() : ContentWidget("Packages")
{
  QVBoxLayout * layout = new QVBoxLayout;
  layout->setAlignment(Qt::AlignTop);

  {
    m_required = new SelectPackagesWidget{{"base"}, true};
    
    QGroupBox * group_required = new QGroupBox("Required");
    group_required->setLayout(m_required->layout());    
    group_required->setEnabled(false);

    layout->addWidget(group_required);
  }
  
  {
    // https://wiki.archlinux.org/title/Kernel#Officially_supported_kernels
    m_kernels = new SelectPackagesWidget{{"linux"}, true};
    
    QGroupBox * group_kernels = new QGroupBox("Kernels");
    group_kernels->setLayout(m_kernels->layout());    

    layout->addWidget(group_kernels);
  }

  {
    m_firmware = new SelectPackagesWidget{{"linux-firmware"}, false};
    
    QGroupBox * group_firmware = new QGroupBox("Firmware");
    group_firmware->setLayout(m_firmware->layout());    

    layout->addWidget(group_firmware);
  }
  
  setLayout(layout);
}