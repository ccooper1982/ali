#include <ali/widgets/profile_widget.hpp>
#include <ali/profiles.hpp>


struct ProfileSelect : public QWidget
{
  ProfileSelect(QWidget * parent = nullptr) : QWidget(parent)
  {
    m_layout = new QFormLayout;
    
    m_packages = new QPlainTextEdit;
    m_packages->setMaximumWidth(400);
    m_packages->setMaximumHeight(150);
    m_packages->setReadOnly(true);
    m_packages->setWordWrapMode(QTextOption::NoWrap);
    m_packages->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_packages->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_commands = new QPlainTextEdit;
    m_commands->setMaximumWidth(450);
    m_commands->setMaximumHeight(75);
    m_commands->setReadOnly(true);
    m_commands->setWordWrapMode(QTextOption::NoWrap);
    m_commands->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_commands->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_info = new QTextEdit;
    m_info->setReadOnly(true);
    m_info->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_info->setMinimumHeight(400);
    m_info->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        
    
    m_profile = new QComboBox;
    m_profile->setFixedWidth(200);
    
    connect(m_profile, &QComboBox::currentTextChanged, this, [this](const QString& name)
    {
      profile_selection_changed(name);
    });

    m_layout->addRow("Profile", m_profile);
    m_layout->addRow("Packages", m_packages);
    m_layout->addRow("Commands", m_commands);
    m_layout->addRow("Info", m_info);
    
    m_layout->setContentsMargins(0,10,10,10);
    

    m_tty_profiles = Profiles::get_tty_profile_names();
    m_desktop_profiles = Profiles::get_desktop_profile_names();

    setLayout(m_layout);
  }


  void set_tty()
  {
    m_profile->clear();
    m_profile->addItems(m_tty_profiles);
  }


  void set_desktop()
  {
    m_profile->clear();
    m_profile->addItems(m_desktop_profiles);
  }

  
  QString get_profile_name() const
  {
    return m_profile->currentText();
  }


private:
  void profile_selection_changed(const QString& name)
  {
    try
    {
      m_packages->clear();
      m_commands->clear();
      m_info->clear();

      const auto& profile = Profiles::get_profile(name);
      m_packages->appendPlainText(profile.packages.join('\n'));
      m_commands->appendPlainText(profile.commands.join('\n'));
      m_info->setMarkdown(profile.info);

      m_packages->verticalScrollBar()->setSliderPosition(0);
      m_commands->verticalScrollBar()->setSliderPosition(0);
    }
    catch(const std::exception& e)
    {
      qCritical() << e.what();
    }
  }


protected:
  QComboBox * m_profile;
  QFormLayout * m_layout;
  QPlainTextEdit * m_packages, * m_commands;
  QTextEdit * m_info;
  QStringList m_tty_profiles;
  QStringList m_desktop_profiles;
};



ProfileWidget::ProfileWidget() : ContentWidget("Profiles")
{
  m_layout = new QVBoxLayout;

  m_profile_type = new QComboBox;
  m_profile_type->addItems({"TTY", "Desktop"});
  m_profile_type->setFixedWidth(200);

  connect(m_profile_type, &QComboBox::currentTextChanged, this, [this](const QString& item)
  {
    if (item == "TTY")
      m_profile_select->set_tty();
    else
      m_profile_select->set_desktop();
  });


  if (!Profiles::read())
  {
    m_profile_type->setEnabled(false);
    QMessageBox::critical(this, "Profiles", "Failed to read profile data");
  } 

  m_profile_select = new ProfileSelect;
  
  QFormLayout * type_layout = new QFormLayout;
  type_layout->addRow("Type", m_profile_type);

  m_layout->setAlignment(Qt::AlignTop);
  m_layout->setContentsMargins(0,10,0,0);
  m_layout->addLayout(type_layout);  
  m_layout->addWidget(m_profile_select);
  m_layout->addStretch(1);

  m_profile_select->set_tty();

  setLayout(m_layout);
}


void ProfileWidget::profile_type_changed(const QString& name)
{
  // QWidget * curr, * next;

  // if (name == "Desktop")
  // {
  //   curr = m_tty_profiles;
  //   next = m_desktop_profiles;
  // }
  // else
  // {
  //   curr = m_desktop_profiles;
  //   next = m_tty_profiles;
  // }

  // if (curr && next)
  // {
  //   m_layout->replaceWidget(curr, next);
  //   curr->hide();
  //   next->show();
  // }
}


Profile ProfileWidget::get_data() const
{
  return Profiles::get_profile(m_profile_select->get_profile_name());
}


bool ProfileWidget::is_valid()
{
  return m_profile_select && !m_profile_select->get_profile_name().isEmpty();
}