#ifndef ALI_WELCOMEWIDGET_H
#define ALI_WELCOMEWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <ali/widgets/content_widget.hpp>


inline static const QString intro = R"!(
# Arch Linux Install

---

This tool follows the Arch guide, which contains everything required
for install.

No changes are made until the final step when 'Install' is pressed.

)!";


struct WelcomeWidget : public ContentWidget
{
  WelcomeWidget() : ContentWidget("Start")
  {
    QLabel * label = new QLabel(intro);
    label->setContentsMargins(0,0,0,10);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    label->setWordWrap(true);
    label->setTextFormat(Qt::TextFormat::MarkdownText);
    
    QVBoxLayout * layout = new QVBoxLayout;
    layout->setContentsMargins(10,10,10,10);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addWidget(label);

    layout->addStretch(1);
    setLayout(layout);
  }

  virtual ~WelcomeWidget() = default;
};




#endif
