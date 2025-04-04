#include <ali/widgets/accounts_widget.hpp>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>


AccountsWidget::AccountsWidget() : ContentWidget("Accounts")
{
  QVBoxLayout * layout = new QVBoxLayout;
  layout->setAlignment(Qt::AlignTop);
    
  {
    QFormLayout * root_layout = new QFormLayout;
    QGroupBox * root_group = new QGroupBox{"Root"};

    m_root_pass = new QLineEdit("arch");
    m_root_pass->setMaximumWidth(150);
    root_layout->addRow("Root Password", m_root_pass);
    
    root_group->setLayout(root_layout);
    layout->addWidget(root_group);
  }
  
  {
    QFormLayout * user_layout = new QFormLayout;
    QGroupBox * user_group = new QGroupBox{"User"};

    m_user_username = new QLineEdit;
    m_user_username->setMaximumWidth(150);

    m_user_pass = new QLineEdit;
    m_user_pass->setMaximumWidth(150);

    m_user_sudo = new QCheckBox("Add the user to sudoers");
    m_user_sudo->setChecked(true);

    user_layout->addRow("Username", m_user_username);
    user_layout->addRow("Password", m_user_pass);
    user_layout->addRow("Permit sudo", m_user_sudo);
    
    user_group->setLayout(user_layout);
    layout->addWidget(user_group);
  }

  layout->addStretch();
  setLayout(layout);
}


bool AccountsWidget::is_valid()
{
  //don't force a user account
  const bool root_pass = !m_root_pass->text().isEmpty();
  const bool user_name = !m_user_username->text().isEmpty();
  const bool user_pass = !m_user_pass->text().isEmpty();

  // root_pass is set AND username is set if it's not empty
  return root_pass && ((user_name && user_pass) || (!user_name));
}