#include <ali/widgets/install_widget.hpp>
#include <ali/widgets/widgets.hpp>


static const QString waffle_preinstall = R"!(
### Install
- A log file is created in `/var/log/ali/install.log`
- The log below only shows minimal information
- There is no progress indicator, this may take a few minutes

---

)!";


static const QString waffle_postinstall_ok = R"!(
# Complete

Full details:
- `/var/log/ali/install.log`

<br/>

## You can reboot now

---

)!";


static const QString waffle_postinstall_fail = R"!(
# Fail

Full details:
- `/var/log/ali/install.log`
 
If the error is produced during any of:
- mount
- pacstrap
- fstab
- arch-choot
- bootloader

Rebooting to the installed system will likely fail, otherwise
it is usually safe to reboot to the installed system. 

To fix the issue in the __live ISO__, you can return to the terminal.

In the terminal, only chroot if the "arch-chroot" step succeeded
during install (see log below):

`arch-chroot /mnt`

See Arch Guide to troubleshoot.
  
---
  
)!";


struct LogWidget : public QPlainTextEdit
{
  LogWidget() : QPlainTextEdit()
  {
    setReadOnly(true);
    setOverwriteMode(false);
    setWordWrapMode(QTextOption::WrapMode::NoWrap);
    setMinimumHeight(400);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  }
};


InstallWidget::InstallWidget() : ContentWidget("Install")
{
  QVBoxLayout * layout = new QVBoxLayout;
  layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  m_lbl_waffle = new QLabel(waffle_preinstall);
  m_lbl_waffle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  m_lbl_waffle->setWordWrap(true);
  m_lbl_waffle->setTextFormat(Qt::TextFormat::MarkdownText);

  m_btn_install = new QPushButton("Install");
  m_btn_install->setMaximumWidth(100);
  
  QHBoxLayout * install_icon_layout = new QHBoxLayout;
  install_icon_layout->setAlignment(Qt::AlignHCenter);

  m_lbl_busy = new QLabel;
  m_lbl_busy->setFixedSize(50,50);
  m_lbl_busy->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  m_lbl_busy->setAutoFillBackground(true);
  m_lbl_busy->setStyleSheet("QLabel { background-color: green; }");
  
  install_icon_layout->addWidget(m_btn_install, 0, Qt::AlignHCenter);

  m_log_widget = new LogWidget;
  m_log_widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
  
  layout->addWidget(m_lbl_waffle);
  layout->addLayout(install_icon_layout);
  layout->addWidget(m_log_widget);
  layout->addStretch();

  setLayout(layout);

  connect(&m_installer, &Install::on_stage_start, this, [this](const QString msg)
  {
    m_log_widget->appendHtml("<b>" + msg + "</b>");
  });

  connect(&m_installer, &Install::on_log, this, [this](const QString msg)
  {
    m_log_widget->appendPlainText(msg);
  });

  connect(&m_installer, &Install::on_stage_end, this, [this](const QString msg)
  {
    m_log_widget->appendHtml("<b>" + msg + "</b>");
  });

  connect(&m_installer, &Install::on_complete, this, [this](const bool success)
  {
    if (success)
    {
      m_btn_install->hide();
      m_lbl_waffle->setText(waffle_postinstall_ok);
    }
    else
    {
      m_btn_install->setEnabled(true);
      m_btn_install->setText("Install");
      m_lbl_waffle->setText(waffle_postinstall_fail);
    }
  });

  #ifdef ALI_PROD
    connect(m_btn_install, &QPushButton::clicked, this, &InstallWidget::install);
  #endif
}


InstallWidget::~InstallWidget()
{
  // TODO don't need to join install thread, but may need
  //      a way to cancel Install::install()
}


void InstallWidget::focusInEvent(QFocusEvent *event)
{
  validate();
}


void InstallWidget::validate()
{
  bool valid {false};

  // ask each widget, except ourselves, if it's valid
  for(const auto& widget : Widgets::all())
  {
    if (!widget->is_install_widget())
    {
      if (valid = widget->is_valid(); !valid)
      {
        qWarning() << "Invalid: " << widget->get_nav_name();
        break;
      }
    }
  }
  
  if (valid)
  {
    qInfo() << "Validation passed";
  }
  
  m_btn_install->setEnabled(valid);
}


#ifdef ALI_PROD
void InstallWidget::install()
{
  qInfo() << "Installing";

  // offload the install to a dedicated class in a separate thread
  try
  {
    m_btn_install->setText("Installing ...");
    m_btn_install->setEnabled(false);
    
    m_install_thread = std::move(std::jthread([this](){ m_installer.install(); }));
  }
  catch(const std::exception& e)
  {
    m_log_widget->appendPlainText(QString::fromStdString(std::format("ERROR: exception: {}", e.what())));
    qCritical() << e.what();
  }
}
#endif