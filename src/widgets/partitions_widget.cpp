#include <ali/widgets/partitions_widget.hpp>
#include <ali/util.hpp>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QTableWidgetItem>


struct HorizonalLine : public QFrame
{
  HorizonalLine()
  {
    setFrameShape(QFrame::HLine);
  }
};


struct SelectMounts : public QWidget
{
  SelectMounts(QWidget * parent = nullptr) : QWidget(parent)
  {
    QFormLayout * layout = new QFormLayout;
    setLayout(layout);
    
    m_boot = new QComboBox;
    m_root = new QComboBox;
    m_home = new QComboBox;
    m_home_to_root = new QCheckBox("Mount /home to root partition");
        
    m_root->setMaximumWidth(200);
    m_boot->setMaximumWidth(200);
    m_home->setMaximumWidth(200);


    layout->addRow("/", m_root);
    layout->addRow("/boot", m_boot);
    layout->addRow("/home", m_home);
    layout->addRow("", m_home_to_root);

    m_home_to_root->connect(m_home_to_root, &QCheckBox::checkStateChanged, [this](const Qt::CheckState state)
    {
      if (state == Qt::Checked)
      {
        m_home->setCurrentText(get_root());
        m_home->setEnabled(false);
      }
      else
      {
        m_home->setEnabled(true);
      }
    });

    m_root->connect(m_root, &QComboBox::currentTextChanged, [this](const QString& value)
    {
      if (m_home_to_root->checkState() == Qt::Checked)
        m_home->setCurrentText(value);
    });


    m_home_to_root->setChecked(true);
    m_home_to_root->setEnabled(false);
  }

  void add_boot(const QString& dev)
  {
    m_boot->addItem(dev);
  }

  void add_root(const QString& dev)
  {
    m_root->addItem(dev);
  }

  void add_home(const QString& dev)
  {
    m_home->addItem(dev);
  }

  QString get_root () const
  {
    return m_root->itemText(m_root->currentIndex());
  }

  QString get_boot () const
  {
    return m_boot->itemText(m_boot->currentIndex());
  }

  QString get_home () const
  {
      return m_home->itemText(m_home->currentIndex());
  }

private:
  QComboBox * m_boot, * m_root, * m_home;
  QCheckBox * m_home_to_root;
};


static std::string format_size(const int64_t size)
{
  static const char *sizeNames[] = {"B", "KB", "MB", "GB", "TB", "PB"};

  std::string result;

  const uint64_t i = (uint64_t) std::floor(std::log(size) / std::log(1024));
  const auto display_size = size / std::pow(1024, i);
  
  return std::format("{:.1f} {}", display_size, sizeNames[i]);
}


PartitionsWidget::PartitionsWidget() : ContentWidget("Mounts")
{
  static const int COL_DEV  = 0;
  static const int COL_FS   = 1;
  static const int COL_EFI  = 2;
  static const int COL_SIZE = 3;

  QVBoxLayout * layout = new QVBoxLayout;
  setLayout(layout);
  
  layout->setAlignment(Qt::AlignTop);
  layout->addWidget(new QLabel("Partitions"));
  

  m_mounts_widget = new SelectMounts;
  m_partitions = get_partitions();

  auto table = new QTableWidget(m_partitions.size(), 4);
  table->verticalHeader()->hide();
  table->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  table->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);  
  table->setMinimumHeight(100);
  table->setMaximumHeight(200);
  table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  table->setHorizontalHeaderItem(COL_DEV,   new QTableWidgetItem("Device"));
  table->setHorizontalHeaderItem(COL_FS,    new QTableWidgetItem("Filesystem"));
  table->setHorizontalHeaderItem(COL_EFI,   new QTableWidgetItem("EFI"));
  table->setHorizontalHeaderItem(COL_SIZE,  new QTableWidgetItem("Size"));
  

  if (m_partitions.empty())
  {
    qCritical() << "No partitions found";
  }

  int row = 0;
  for(const auto& part : m_partitions)
  {
    const auto path = QString::fromStdString(part.path);

    auto item_dev = new QTableWidgetItem(path);
    auto item_type = new QTableWidgetItem(QString::fromStdString(part.is_fat32 ? "vfat (FAT32)" : part.fs_type));
    auto item_efi = new QTableWidgetItem(QString::fromStdString(part.is_efi ? "True" : "False"));
    auto item_size = new QTableWidgetItem(QString::fromStdString(format_size(part.size)));
    
    item_type->setTextAlignment(Qt::AlignHCenter);
    item_efi->setTextAlignment(Qt::AlignHCenter);

    table->setItem(row, COL_DEV, item_dev) ;
    table->setItem(row, COL_FS, item_type) ;
    table->setItem(row, COL_EFI, item_efi) ;
    table->setItem(row, COL_SIZE, item_size) ;
    ++row;

    if (part.is_efi)
      m_mounts_widget->add_boot(path);
    else
    {
      m_mounts_widget->add_home(path);
      m_mounts_widget->add_root(path);
    }
  }

  table->setColumnWidth(COL_DEV,200);
  table->setColumnWidth(COL_FS,100);
  table->setColumnWidth(COL_EFI,100);
  table->horizontalHeader()->setStretchLastSection(true);
  table->resizeRowsToContents();

  layout->addWidget(table);
  layout->addWidget(m_mounts_widget);
}


bool PartitionsWidget::is_valid()
{
  // widget only allows appropriate partitions
  // for the mount points.
  return true;
}


std::pair<bool, PartitionData> PartitionsWidget::get_data()
{
  const auto root_part = m_mounts_widget->get_root().toStdString();
  const auto boot_part = m_mounts_widget->get_boot().toStdString();
  const auto home_part = m_mounts_widget->get_home().toStdString();

  const auto[have_root, root_fs] = get_fs_from_path(root_part);
  const auto[have_boot, boot_fs] = get_fs_from_path(boot_part);
  const auto[have_home, home_fs] = get_fs_from_path(home_part);
  
  if (have_root && have_boot && have_home)
  {
    return {true, PartitionData {
                                  .root = {.path = root_part, .fs = root_fs},
                                  .boot = {.path = boot_part, .fs = boot_fs},
                                  .home = {.path = home_part, .fs = home_fs}
                                }};
  }
  else
    return {false, {}};
}


std::pair<bool, std::string> PartitionsWidget::get_fs_from_path(const std::string& path)
{
  const auto it = std::find_if(std::cbegin(m_partitions), std::cend(m_partitions), [&path](const Partition& part)
  {
    return part.path == path;
  });

  if (it == std::cend(m_partitions))
    return {false, ""};
  else
    return {true, it->fs_type};
}
