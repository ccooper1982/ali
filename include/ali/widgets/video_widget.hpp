#ifndef ALI_VIDEOWIDGET_H
#define ALI_VIDEOWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <ali/common.hpp>



struct VideoWidget : public ContentWidget
{
  VideoWidget();
  virtual ~VideoWidget() = default;

  virtual bool is_valid() override;

private:
  void get_vendor();
  void vendor_changed(const QString& vendor);

private:
  QComboBox * m_vendor;
  QComboBox * m_source_model;
  QListWidget * m_packages;
  GpuVendor m_video_vendor{GpuVendor::Unknown};
};


#endif
