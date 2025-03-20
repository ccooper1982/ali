#ifndef ALI_ACCOUNTSWIDGET_H
#define ALI_ACCOUNTSWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QLineEdit>


struct AccountsWidget : public ContentWidget
{
  AccountsWidget();
  virtual ~AccountsWidget() = default;

private:
  QLineEdit * root_pass;
  QLineEdit * user_username;
  QLineEdit * user_pass;
};


#endif
