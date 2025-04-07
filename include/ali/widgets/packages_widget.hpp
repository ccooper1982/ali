#ifndef ALI_PACKAGESWIDGET_H
#define ALI_PACKAGESWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QSet>
#include <QString>


struct SelectPackagesWidget;


struct PackagesWidget : public ContentWidget
{
  PackagesWidget();
  virtual ~PackagesWidget() = default;

  virtual bool is_valid() override;

private:
  SelectPackagesWidget * m_required;
  SelectPackagesWidget * m_kernels;
  SelectPackagesWidget * m_firmware;  
  SelectPackagesWidget * m_important;
  SelectPackagesWidget * m_shell;
};


#endif
