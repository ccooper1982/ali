#include <ali/widgets/install_widget.hpp>

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

  #ifdef ALI_PROD
    QPushButton * btn_install = new QPushButton("Install");
    btn_install->setMaximumWidth(100);
    connect(btn_install, &QPushButton::clicked, this, &InstallWidget::install);

    layout->addWidget(btn_install, 0, Qt::AlignHCenter);
  #endif

  m_log_widget = new LogWidget;
  m_log_widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    
  layout->addWidget(m_log_widget);
  layout->addStretch(1);

  setLayout(layout);

  validate();
}

void InstallWidget::validate()
{
  // ask each widget if it's valid
}

#ifdef ALI_PROD
void InstallWidget::install()
{
  
}
#endif