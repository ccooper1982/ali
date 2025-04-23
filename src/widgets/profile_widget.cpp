#include <ali/widgets/profile_widget.hpp>
#include <ali/profiles.hpp>
#include <ali/packages.hpp>


struct ProfileSelect : public QWidget
{
  ProfileSelect(QWidget * parent = nullptr) : QWidget(parent)
  {
    m_tty_profiles = Profiles::get_tty_profile_names();
    m_desktop_profiles = Profiles::get_desktop_profile_names();
    m_desktop_greeters = Profiles::get_greeter_names(false);

    m_layout = new QFormLayout;
    
    m_packages = new QPlainTextEdit;
    m_packages->setMaximumWidth(450);
    m_packages->setMaximumHeight(175);
    m_packages->setReadOnly(true);
    m_packages->setWordWrapMode(QTextOption::NoWrap);
    m_packages->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_packages->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_commands = new QPlainTextEdit;
    m_commands->setMaximumWidth(450);
    m_commands->setMaximumHeight(125);
    m_commands->setReadOnly(true);
    m_commands->setWordWrapMode(QTextOption::NoWrap);
    m_commands->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_commands->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_profile = new QComboBox;
    m_profile->setFixedWidth(200);
    
    m_greeter = new QComboBox;
    m_greeter->setFixedWidth(200);
    //m_greeter->addItems({"None (tty)", "sddm", "gdm"});

    m_layout->addRow("Profile", m_profile);
    m_layout->addRow("Greeter", m_greeter);
    m_layout->addRow("Packages", m_packages);
    m_layout->addRow("Commands", m_commands);
    m_layout->setContentsMargins(0,10,10,10);
    
    set_tty();
    
    connect(m_profile, &QComboBox::currentTextChanged, this, [this](const QString& name)
    {
      profile_selection_changed(name);
    });

    connect(m_greeter, &QComboBox::currentTextChanged, this, [this](const QString&)
    {
      const QString name = get_greeter_name();
      
      if (name.isEmpty())
        Packages::set_greeter_packages({}); // None
      else
        Packages::set_greeter_packages(Profiles::get_greeter(name).packages);
    });
    
    setLayout(m_layout);
  }


  void set_tty()
  {
    m_profile->clear();
    m_profile->addItems(m_tty_profiles);
    m_profile->setCurrentIndex(0);
    m_layout->setRowVisible(1, false); // hide greeter for tty
    m_tty = true;

    profile_selection_changed(m_profile->currentText());
  }


  void set_desktop()
  {
    m_profile->clear();
    m_profile->addItems(m_desktop_profiles);
    m_profile->setCurrentIndex(0);
    m_layout->setRowVisible(1, true);
    m_tty = false;

    profile_selection_changed(m_profile->currentText());
  }

  
  QString get_profile_name() const
  {
    return m_profile->currentText();
  }

  QString get_greeter_name() const
  {
    // index is "none" which isn't in greeters.json, so return empty
    return m_greeter->currentIndex() == 0 ? "" : m_greeter->currentText();
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

      const auto& profile = Profiles::get_profile(name);
      
      Packages::set_profile_packages(profile.packages);

      m_packages->setPlainText(profile.packages.join('\n'));
      m_greeter->clear();
      
      m_greeter->addItem("None"); // for "None" option
      m_greeter->addItems(Profiles::get_greeter_names(m_tty));

      m_commands->clear();

      if (!profile.system_commands.empty())
      {
        m_commands->appendPlainText("-- System Commands");
        m_commands->appendPlainText(profile.system_commands.join('\n'));
      }

      if (!profile.user_commands.empty())
      {
        m_commands->appendPlainText("-- User Commands");
        m_commands->appendPlainText(profile.user_commands.join('\n'));
      }
      
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
  QComboBox * m_profile, * m_greeter;
  QFormLayout * m_layout;
  QPlainTextEdit * m_packages, * m_commands;
  QStringList m_tty_profiles;
  QStringList m_desktop_profiles;
  QStringList m_desktop_greeters;
  bool m_tty{true};
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


ProfileData ProfileWidget::get_data() const
{
  return {.profile_name = m_profile_select->get_profile_name(), .greeter_name = m_profile_select->get_greeter_name() };
}


bool ProfileWidget::is_valid()
{
  return m_profile_select && !m_profile_select->get_profile_name().isEmpty();
}