#include <ali/widgets/start_widget.hpp>
#include <ali/commands.hpp>


static const QString intro = R"!(
# Arch Linux Install

---

This tool follows the Arch guide, which contains everything required
for troubleshooting if the install fails.

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

  m_combo_tz = new QComboBox;
  m_combo_tz->setMaximumWidth(200);

  settings_layout->addRow("Keyboard", m_combo_keymaps);
  settings_layout->addRow("Locale", m_combo_locales);
  settings_layout->addRow("Timezone", m_combo_tz);

  
  if (!get_keymaps())
    QMessageBox::warning(this, "Keymap", "Could not get keymaps");

  if (!get_locales())
    QMessageBox::warning(this, "Locale", "Could not get locales");

  if (!get_timezones())
    QMessageBox::warning(this, "Timezone", "Could not get timezones");


  layout->addWidget(lbl_intro);
  layout->addLayout(settings_layout);
  layout->addStretch(1);

  setLayout(layout);
}


bool StartWidget::get_keymaps()
{
  if (!LocaleUtils::read_keymaps())
  {
    m_combo_keymaps->addItem("us");
    m_combo_keymaps->setEnabled(false);
    return false;
  }
  else
  {
    m_combo_keymaps->addItems(LocaleUtils::get_keymaps());
    return true;
  }
}


bool StartWidget::get_locales()
{
  if (!LocaleUtils::read_locales())
  {
    m_combo_locales->addItem("en_US.UTF-8");
    m_combo_locales->setEnabled(false);
    return false;
  }
  else
  {
    m_combo_locales->addItems(LocaleUtils::get_locales());
    return true;
  }
}


bool StartWidget::get_timezones()
{
  if (!LocaleUtils::read_timezones())
  {
    m_combo_tz->addItem("Europe/London");
    m_combo_tz->setEnabled(false);
    return false;
  }
  else
  {
    m_combo_tz->addItems(LocaleUtils::get_timezones());
    return true;
  }
} 


StartWidget::Locale StartWidget::get_data()
{
  return Locale { .keymap = m_combo_keymaps->currentText().toStdString(),
                  .locales = QStringList{m_combo_locales->currentText()},
                  .timezone = m_combo_tz->currentText().toStdString()};
}