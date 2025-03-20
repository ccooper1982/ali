#ifndef ALI_WELCOMEWIDGET_H
#define ALI_WELCOMEWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <ali/widgets/content_widget.hpp>

struct WelcomeWidget : public ContentWidget
{
  WelcomeWidget(const QString& nav_name) : ContentWidget(nav_name)
  {
    QVBoxLayout * layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QLabel * label = new QLabel("Welcome");
    layout->addWidget(label);

    setLayout(layout);
  }
};




#endif
