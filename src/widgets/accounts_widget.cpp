#include <ali/widgets/accounts_widget.hpp>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>


AccountsWidget::AccountsWidget() : ContentWidget("Accounts")
{
  QVBoxLayout * layout = new QVBoxLayout;
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(50);
  
  {
    QFormLayout * root_layout = new QFormLayout;
    m_root_pass = new QLineEdit("arch");
    m_root_pass->setMaximumWidth(150);
    root_layout->addRow("Root Password", m_root_pass);

    QGroupBox * root_group = new QGroupBox{"Root"};
    root_group->setLayout(root_layout);
    layout->addWidget(root_group);
  }

  {
    QFormLayout * user_layout = new QFormLayout;

    m_user_username = new QLineEdit;
    m_user_username->setMaximumWidth(150);

    m_user_pass = new QLineEdit;
    m_user_pass->setMaximumWidth(150);

    user_layout->addRow("Username", m_user_username);
    user_layout->addRow("Password", m_user_pass);

    QGroupBox * user_group = new QGroupBox{"User"};
    user_group->setLayout(user_layout);
    layout->addWidget(user_group);
  }

  setLayout(layout);
}


bool AccountsWidget::is_valid()
{
  // NOTE: don't force a non-privileged account
  return !m_root_pass->text().isEmpty();
}