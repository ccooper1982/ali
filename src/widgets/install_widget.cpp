#include <ali/widgets/install_widget.hpp>
#include <ali/widgets/widgets.hpp>


static const QString waffle = R"!(
### Check
- Confirm the mount paths
- If you have a dedicated partition for `/home`, bigger is better

### Log
- The commands and steps executed are shown below
- There is a log file created, the path for which is below

---

)!";


struct LogWidget : public QPlainTextEdit
{
  LogWidget()
  {
    setReadOnly(true);
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
  qDebug() << "validate()";

  bool valid {false};

  // ask each widget if it's valid, except ourselves
  for(const auto& widget : Widgets::all())
  {
    if (!widget->is_install_widget())
    {
      if (valid = widget->is_valid(); !valid)
      {
        qDebug() << "invalid: " << widget->get_nav_name();
        break;
      }
    }
  }
  
  m_btn_install->setEnabled(valid);
}


#ifdef ALI_PROD
void InstallWidget::install()
{
  qDebug() << "Installing";
}
#endif