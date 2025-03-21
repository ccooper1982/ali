#ifndef ALI_PACKAGESWIDGET_H
#define ALI_PACKAGESWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QSet>

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
};


#endif
