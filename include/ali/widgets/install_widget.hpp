#ifndef ALI_INSTALLWIDGET_H
#define ALI_INSTALLWIDGET_H

#include <QFileSystemWatcher>
#include <thread>
#include <ali/common.hpp>
#include <ali/widgets/content_widget.hpp>
#include <ali/install.hpp>

struct LogWidget;


struct InstallWidget : public ContentWidget
{
  Q_OBJECT 

public:
  InstallWidget() ;

  virtual ~InstallWidget();

signals:
  void on_install_begin();
  void on_install_end();

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
  QLabel * m_lbl_waffle;
  QLabel * m_lbl_busy;
  std::jthread m_install_thread;
  Install m_installer;
};


#endif
