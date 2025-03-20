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
    root_pass = new QLineEdit("arch");
    root_pass->setMaximumWidth(150);
    root_layout->addRow("Root Password", root_pass);

    QGroupBox * root_group = new QGroupBox{"Root"};
    root_group->setLayout(root_layout);
    layout->addWidget(root_group);
  }

  {
    QFormLayout * user_layout = new QFormLayout;

    user_username = new QLineEdit;
    user_username->setMaximumWidth(150);

    user_pass = new QLineEdit;
    user_pass->setMaximumWidth(150);

    user_layout->addRow("Username", user_username);
    user_layout->addRow("Password", user_pass);

    QGroupBox * user_group = new QGroupBox{"User"};
    user_group->setLayout(user_layout);
    layout->addWidget(user_group);
  }

  setLayout(layout);
}