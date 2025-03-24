#ifndef ALI_PARTITIONSWIDGET_H
#define ALI_PARTITIONSWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <ali/util.hpp>
#include <QString>


struct PartitionData
{
  struct Mount
  {
    std::string path;
    std::string fs;
  };

  Mount root;
  Mount boot;
  Mount home;
};


struct SelectMounts;

struct PartitionsWidget : public ContentWidget
{
  PartitionsWidget();
  virtual ~PartitionsWidget() = default;

  virtual bool is_valid() override;

  std::pair<bool, PartitionData> get_data() ;

private:
  std::pair<bool, std::string> get_fs_from_path(const std::string& path);

private:
  SelectMounts * m_mounts_widget;
  Partitions m_partitions;
};


#endif
