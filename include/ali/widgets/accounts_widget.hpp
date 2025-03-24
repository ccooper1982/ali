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

private:
  QLineEdit * m_root_pass;
  QLineEdit * m_user_username;
  QLineEdit * m_user_pass;
  QCheckBox * m_user_sudo;
};


#endif
