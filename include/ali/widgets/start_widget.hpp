#ifndef ALI_STARTWIDGET_H
#define ALI_STARTWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <vector>
#include <string>
#include <ali/widgets/content_widget.hpp>


struct StartWidget : public ContentWidget
{
  struct Locale
  {
    std::string keymap;
    QStringList locales;
  };

  StartWidget() ;

  virtual ~StartWidget() = default;

  virtual bool is_valid() override { return true; }

  Locale get_data();


private:
  bool get_keymaps();
  bool get_locales();

private:
  std::vector<std::string> m_keymaps;
  QComboBox * m_combo_keymaps;
  QComboBox * m_combo_locales;
};




#endif
