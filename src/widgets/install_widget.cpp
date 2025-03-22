#include <ali/widgets/install_widget.hpp>
#include <ali/widgets/widgets.hpp>
#include <ali/install.hpp>


static const QString waffle = R"!(
### Check
- Confirm the mount paths
- If you have a dedicated partition for `/home`, bigger is better

### Log
- A log file is created in `/var/log/ali/install.log`
- The log below does not show all output (such as from pacstrap), but
  the log file contains all output

---

)!";


struct LogWidget : public QPlainTextEdit
{
  LogWidget()
  {
    setReadOnly(true);
    setWordWrapMode(QTextOption::WrapMode::NoWrap);
    setMinimumHeight(400);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  }

private:
};


InstallWidget::InstallWidget() : ContentWidget("Install")
{
  QVBoxLayout * layout = new QVBoxLayout;
  layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  auto lbl_waffle = new QLabel(waffle);
  lbl_waffle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  lbl_waffle->setWordWrap(true);
  lbl_waffle->setTextFormat(Qt::TextFormat::MarkdownText);

  layout->addWidget(lbl_waffle);

  m_btn_install = new QPushButton("Install");
  m_btn_install->setMaximumWidth(100);
  
  #ifdef ALI_PROD
    connect(m_btn_install, &QPushButton::clicked, this, &InstallWidget::install);
  #endif

  layout->addWidget(m_btn_install, 0, Qt::AlignHCenter);

  m_log_widget = new LogWidget;
  m_log_widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    
  layout->addWidget(m_log_widget);
  layout->addStretch(1);

  setLayout(layout);
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

  // offload the install to a dedicated class, passing
  // everything it needs, keeping the UI somewhat separate
  // from the core work
  
  try
  {
    m_btn_install->setEnabled(false);

    Install installer {[this](const std::string_view& m)
    {
      m_log_widget->appendPlainText(QString::fromLocal8Bit(m.data(), m.size())); 
    }};

    installer.install();
  }
  catch(const std::exception& e)
  {
    qCritical() << e.what();
  }

  m_btn_install->setEnabled(true);  
}
#endif