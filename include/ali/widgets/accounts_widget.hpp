#ifndef ALI_ACCOUNTSWIDGET_H
#define ALI_ACCOUNTSWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QLineEdit>


struct AccountsWidget : public ContentWidget
{
  AccountsWidget();
  virtual ~AccountsWidget() = default;  

  virtual bool is_valid() override;

  std::string root_password() const
  {
    return m_root_pass->text().toStdString();
  }

  std::string user_username() const
  {
    return m_user_username->text().toStdString();
  }

  std::string user_password() const
  {
    return m_user_pass->text().toStdString();
  }

  bool user_is_sudo() const
  {
    return m_user_sudo->isChecked();
  }

private:
  QLineEdit * m_root_pass;
  QLineEdit * m_user_username;
  QLineEdit * m_user_pass;
  QCheckBox * m_user_sudo;
};


#endif
