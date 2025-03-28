#include <ali/widgets/start_widget.hpp>
#include <ali/commands.hpp>


static const QString intro = R"!(
# Arch Linux Install

---

This tool follows the Arch guide, which contains everything required
for install and troubleshooting if the install fails.

Only required packages, additional packages and config files are
placed onto the installed filesystem.

---

)!";


StartWidget::StartWidget() : ContentWidget("Start")
{
  QVBoxLayout * layout = new QVBoxLayout;
  QFormLayout * settings_layout = new QFormLayout;

  layout->setContentsMargins(10,10,10,10);
  layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  
  QLabel * lbl_intro = new QLabel(intro);
  lbl_intro->setContentsMargins(0,0,0,10);
  lbl_intro->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  lbl_intro->setWordWrap(true);
  lbl_intro->setTextFormat(Qt::TextFormat::MarkdownText);
  
  m_combo_keymaps = new QComboBox;
  m_combo_keymaps->setMaximumWidth(200);

  m_combo_locales = new QComboBox;
  m_combo_locales->setMaximumWidth(200);

  settings_layout->addRow("Keyboard", m_combo_keymaps);
  settings_layout->addRow("Locale", m_combo_locales);
  
  if (!get_keymaps())
  {
    QMessageBox::warning(this, "Keymaps", "Could not get keymaps");
    m_combo_keymaps->addItem("US");
    m_combo_keymaps->setEnabled(false);
  }

  if (!get_locales())
  {
    QMessageBox::warning(this, "Locales", "Could not get locales");
    m_combo_locales->addItem("en_US.UTF-8");
    m_combo_locales->setEnabled(false);
  }


  layout->addWidget(lbl_intro);
  layout->addLayout(settings_layout);
  layout->addStretch(1);

  setLayout(layout);
}


bool StartWidget::get_keymaps()
{
  if (KeyMaps cmd; cmd.get_list(m_keymaps))
  {
    for (const auto& name : m_keymaps)
      m_combo_keymaps->addItem(QString::fromStdString(name));
    
    return true;
  }
  else
  {
    qWarning() << "Could not get keymaps";
    return false;
  }
}


bool StartWidget::get_locales()
{
  if (!LocaleUtils::read())
    return false;
  
  m_combo_locales->addItems(LocaleUtils::get());
  return true;
}


StartWidget::Locale StartWidget::get_data()
{
  return Locale { .keymap = m_combo_keymaps->currentText().toStdString(),
                  .locales = QStringList{m_combo_locales->currentText()}
                };
}