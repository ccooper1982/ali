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

private:
  void validate();

  #ifdef ALI_PROD
    void install();
  #else
    void install(){};
  #endif

private:
  LogWidget * m_log_widget;
  QPushButton * m_btn_install{nullptr};
};


#endif
