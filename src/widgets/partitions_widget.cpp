#include <ali/widgets/partitions_widget.hpp>
#include <ali/util.hpp>
#include <QVBoxLayout>
#include <QTreeView>


// struct DeviceTree : public QTreeView
// {
//   DeviceTree()
//   {
//     m_model = new QStandardItemModel{0,1};

//     const DiskTree diskTree = create_disk_tree();

//     for (const auto& [disk_name, parts] : diskTree.get_disks())
//     {
//       auto disk_item = new QStandardItem{QString::fromStdString(disk_name)};

//       for (const auto& part : parts)
//         disk_item->appendRow(new QStandardItem{QString::fromStdString(part.path)});
    
//       m_model->appendRow(disk_item);
//     }

//     setModel(m_model);
//     setHeaderHidden(true);
//   }

// private:
//   QStandardItemModel * m_model{nullptr};
// };


PartitionsWidget::PartitionsWidget(const QString& nav_name) : ContentWidget(nav_name)
{
  QVBoxLayout * layout = new QVBoxLayout;
  setLayout(layout);
  
  //DeviceTree * tree = new DeviceTree;

  layout->addWidget(new QLabel("Devices"));

  Partitions partitions = get_partitions();

  for(const auto& part : partitions)
    std::cout << part.path << ' ' << part.size << '\n';

  //layout->addWidget(tree);
}