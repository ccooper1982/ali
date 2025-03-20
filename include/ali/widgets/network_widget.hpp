#ifndef ALI_NETWORKWIDGET_H
#define ALI_NETWORKWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>


struct NetworkWidget : public ContentWidget
{
  NetworkWidget() : ContentWidget("Network")
  {
    QFormLayout * layout = new QFormLayout;

    edit_hostname = new QLineEdit("archlinux");
    edit_hostname->setMaximumWidth(200);

    chkb_ntp = new QCheckBox();
    chkb_ntp->setChecked(true);

    layout->addRow("Hostname", edit_hostname);
    layout->addRow("NTP", chkb_ntp);

    setLayout(layout);
  }

private:
  QLineEdit * edit_hostname;
  QCheckBox * chkb_ntp;
};


#endif
