#ifndef ALI_NETWORKWIDGET_H
#define ALI_NETWORKWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>


struct NetworkWidget : public ContentWidget
{
  struct NetworkData
  {
    std::string hostname;
    bool ntp{false};
    bool copy_config{true};
  };

  NetworkWidget() : ContentWidget("Network")
  {
    QFormLayout * layout = new QFormLayout;

    m_hostname = new QLineEdit("archlinux");
    m_hostname->setMaximumWidth(200);

    m_ntp = new QCheckBox();
    m_ntp->setChecked(true);

    m_copy_config = new QCheckBox("Use the current network config on the installed system.");
    m_copy_config->setChecked(true);

    layout->addRow("Hostname", m_hostname);
    layout->addRow("NTP", m_ntp);
    layout->addRow("Copy Config", m_copy_config);

    setLayout(layout);
  }

  virtual bool is_valid() override
  {
    return !m_hostname->text().isEmpty();
  }

  NetworkData get_data() const
  {
    return NetworkData { .hostname = m_hostname->text().toStdString(),
                         .ntp = m_ntp->isChecked(),
                         .copy_config = m_copy_config->isChecked()
                        };
  }

private:
  QLineEdit * m_hostname;
  QCheckBox * m_ntp;
  QCheckBox * m_copy_config;
};


#endif
