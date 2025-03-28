#ifndef ALI_PACKAGESWIDGET_H
#define ALI_PACKAGESWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QSet>
#include <QString>


struct SelectPackagesWidget;

struct PackageData
{
  // required: 'base' and 'amd-ucode'/'intel-ucode'
  //           kernels are selected separately
  QSet<QString> required; 
  QSet<QString> kernels;
  QSet<QString> firmware;
  QSet<QString> recommended;
};

struct PackagesWidget : public ContentWidget
{
  PackagesWidget();
  virtual ~PackagesWidget() = default;

  virtual bool is_valid() override;

  PackageData get_data();

private:
  
  SelectPackagesWidget * m_required;
  SelectPackagesWidget * m_kernels;
  SelectPackagesWidget * m_firmware;
  SelectPackagesWidget * m_recommended;
};


#endif
