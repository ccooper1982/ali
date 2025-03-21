#ifndef ALI_INSTALLWIDGET_H
#define ALI_INSTALLWIDGET_H

#include <QFileSystemWatcher>
#include <ali/widgets/content_widget.hpp>
#include <ali/common.hpp>

struct LogWidget;


struct InstallWidget : public ContentWidget
{
  InstallWidget() ;

  virtual ~InstallWidget() = default;

protected:
  virtual void focusInEvent(QFocusEvent *event) override;
  
private:
  void validate();

  virtual bool is_install_widget() const override
  {
    return true;
  }

  #ifdef ALI_PROD
    void install();
    virtual bool is_valid() override { return true; }
  #else
    void install(){};
    virtual bool is_valid() override { return false; }
  #endif

private:
  LogWidget * m_log_widget;
  QPushButton * m_btn_install{nullptr};
};


#endif
