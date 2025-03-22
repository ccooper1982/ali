#ifndef ALI_PARTITIONSWIDGET_H
#define ALI_PARTITIONSWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QString>


struct PartitionData
{
  std::string root;
  std::string boot;
  std::string home;
};


struct SelectMounts;

struct PartitionsWidget : public ContentWidget
{
  PartitionsWidget();
  virtual ~PartitionsWidget() = default;

  virtual bool is_valid() override;

  PartitionData get_data() ;

private:
  SelectMounts * m_mounts_widget;
};


#endif
