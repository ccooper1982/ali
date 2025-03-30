#ifndef ALI_PARTITIONSWIDGET_H
#define ALI_PARTITIONSWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <ali/disk_utils.hpp>
#include <QString>


struct MountData
{
  struct Mount
  {
    std::string dev;
    std::string fs;
    bool create_fs{false};
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

  std::pair<bool, MountData> get_data() ;

private:
  std::pair<bool, std::string> get_fs_from_path(const std::string& path);
  QTableWidget *  create_table();

private:
  SelectMounts * m_mounts_widget{nullptr};
};


#endif
