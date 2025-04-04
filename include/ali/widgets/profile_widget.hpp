#ifndef ALI_PROFILEWIDGET_H
#define ALI_PROFILEWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <ali/profiles.hpp>


struct ProfileSelect;


struct ProfileWidget : public ContentWidget
{
  ProfileWidget();
  virtual ~ProfileWidget() = default;

  virtual bool is_valid() override;

  Profile get_data() const;

private:

  void profile_type_changed(const QString& name);

private:
  QVBoxLayout * m_layout;  
  QComboBox * m_profile_type;
  ProfileSelect * m_profile_select;
  
};


#endif
