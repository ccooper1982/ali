#ifndef ALI_PROFILEWIDGET_H
#define ALI_PROFILEWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <ali/profiles.hpp>


struct ProfileSelect;

struct ProfileData
{
  QString profile_name;
  QString greeter_name;
};


struct ProfileWidget : public ContentWidget
{
  ProfileWidget();
  virtual ~ProfileWidget() = default;

  virtual bool is_valid() override;

  ProfileData get_data() const;


private:
  QVBoxLayout * m_layout;  
  QComboBox * m_profile_type;
  ProfileSelect * m_profile_select;
  
};


#endif
