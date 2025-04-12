#include <ali/widgets/profile_widget.hpp>
#include <ali/profiles.hpp>
#include <ali/packages.hpp>


struct ProfileSelect : public QWidget
{
  ProfileSelect(QWidget * parent = nullptr) : QWidget(parent)
  {
    m_tty_profiles = Profiles::get_tty_profile_names();
    m_desktop_profiles = Profiles::get_desktop_profile_names();

    m_layout = new QFormLayout;
    
    m_packages = new QPlainTextEdit;
    m_packages->setMaximumWidth(450);
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
    set_tty();
    
    connect(m_profile, &QComboBox::currentTextChanged, this, [this](const QString& name)
    {
      qDebug() << "slot";
      profile_selection_changed(name);
    });

    m_layout->addRow("Profile", m_profile);
    m_layout->addRow("Packages", m_packages);
    m_layout->addRow("Commands", m_commands);
    m_layout->addRow("Info", m_info);
    
    m_layout->setContentsMargins(0,10,10,10);
    
    setLayout(m_layout);
  }


  void set_tty()
  {
    m_profile->clear();
    m_profile->addItems(m_tty_profiles);
    m_profile->setCurrentIndex(0);

    profile_selection_changed(m_profile->currentText());
  }


  void set_desktop()
  {
    m_profile->clear();
    m_profile->addItems(m_desktop_profiles);
    m_profile->setCurrentIndex(0);

    profile_selection_changed(m_profile->currentText());
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
      // empty when we set_tty() or set_desktop(), clear and
      // then add. Not an error but get_profile() throws
      if (name.isEmpty())
        return;

      m_info->clear();

      const auto& profile = Profiles::get_profile(name);
      
      Packages::set_profile_packages(profile.packages);

      m_packages->setPlainText(profile.packages.join('\n'));

      m_commands->setPlainText("-- System Commands");
      m_commands->appendPlainText(profile.system_commands.join('\n'));
      m_commands->appendPlainText("-- User Commands");
      m_commands->appendPlainText(profile.user_commands.join('\n'));
      
      m_info->setMarkdown(profile.info);

      m_packages->verticalScrollBar()->setSliderPosition(0);
      m_commands->verticalScrollBar()->setSliderPosition(0);
    }
    catch(const std::exception& e)
    {
      qCritical() << "exception when profile selection: " << name;
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
  if (!Profiles::read())
  {
    QMessageBox::critical(this, "Profiles", "Failed to read profile data");
  }

  m_layout = new QVBoxLayout;
  
  m_profile_type = new QComboBox;  
  m_profile_type->setFixedWidth(200);
  m_profile_type->addItems({"TTY", "Desktop"});

  
  connect(m_profile_type, &QComboBox::currentTextChanged, this, [this](const QString& item)
  {
    if (item == "TTY")
      m_profile_select->set_tty();
    else
      m_profile_select->set_desktop();
  });


  m_profile_select = new ProfileSelect;

  QFormLayout * type_layout = new QFormLayout;
  type_layout->addRow("Type", m_profile_type);

  m_layout->setAlignment(Qt::AlignTop);
  m_layout->setContentsMargins(0,10,0,0);
  m_layout->addLayout(type_layout);  
  m_layout->addWidget(m_profile_select);
  m_layout->addStretch(1);

  setLayout(m_layout);
}


QString ProfileWidget::get_data() const
{
  return m_profile_select->get_profile_name();
}


bool ProfileWidget::is_valid()
{
  return m_profile_select && !m_profile_select->get_profile_name().isEmpty();
}