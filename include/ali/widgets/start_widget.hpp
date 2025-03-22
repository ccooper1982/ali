#ifndef ALI_STARTWIDGET_H
#define ALI_STARTWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <ali/widgets/content_widget.hpp>


struct StartWidget : public ContentWidget
{
  StartWidget() ;

  virtual ~StartWidget() = default;

  virtual bool is_valid() override { return true; }

private:
  bool keymaps();
  bool language();

private:
  std::vector<std::string> m_keymaps;
  QComboBox * m_combo_keymaps;
  QComboBox * m_combo_country;
};




#endif
