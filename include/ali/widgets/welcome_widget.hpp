#ifndef ALI_WELCOMEWIDGET_H
#define ALI_WELCOMEWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <ali/widgets/content_widget.hpp>

struct WelcomeWidget : public ContentWidget
{
  WelcomeWidget() : ContentWidget("Start")
  {
    QVBoxLayout * layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QLabel * label = new QLabel("Arch Linux Installer");
    layout->addWidget(label);

    setLayout(layout);
  }

  virtual ~WelcomeWidget() = default;
};




#endif
