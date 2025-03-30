#include <ali/widgets/partitions_widget.hpp>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QTableWidgetItem>


static const QString waffle_title_have_parts = R"!(## Partitions
This table only shows partitions that are within a **GPT**
and **not mounted**.

If your partitions are not showing, return to the terminal:
- `lsblk -o PTTYPE,MOUNTPOINT <device>`


To keep an existing filesystem, leave the "Create Filesystem" option
blank, otherwise set the desired filesystem.


`/boot`
- Must be `FAT32`. Created filesystem is always FAT32 
- Recommended size: 512MB - 1GB
)!";

static const QString waffle_title_no_parts = R"!( ## Partitions
No eligible partitions found:
- Device must have a GPT (partition table)
- Partition must **not** be mounted

<br/>

Return to the terminal, then:
- `lsblk -o PATH,PTTYPE,MOUNTPOINT <device>`

<br/>

PTTYPE must be `gpt` and MOUNTPOINT empty.

- If required, unmount with: `umount <mount_point>`
- Remove all partition tables: `wipefs -a -f <dev>`
  - Take care: assume data is unrecoverable
- Create GPT:
  - `fdisk <dev>`
  - Enter `g`, press enter, then `w` and press Enter

<br/>

Then use `fdisk <dev>` or `cfdisk <dev>` to create partitions as required.
- Boot partition should be at least 512MB
- Root partition should be at least 8GB (minimal) or 16GB (desktop)
)!";


static const QStringList SupportedRootFs = 
{
  "ext4"
};

static const QStringList SupportedHomeFs = 
{
  "ext4"
};

static const QStringList SupportedBootFs = 
{
  "vfat"  // FAT32
};



static std::string format_size(const int64_t size)
{
  static const char *sizeNames[] = {"B", "KB", "MB", "GB", "TB", "PB"};

  std::string result;

  const uint64_t i = (uint64_t) std::floor(std::log(size) / std::log(1024));
  const auto display_size = size / std::pow(1024, i);
  
  return std::format("{:.1f} {}", display_size, sizeNames[i]);
}


struct SelectMounts : public QWidget
{
  SelectMounts(QWidget * parent = nullptr) : QWidget(parent)
  {
    m_summary = new QLabel;

    QVBoxLayout * layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignTop | Qt::AlignCenter);


    QFormLayout * mounts_layout = new QFormLayout;

    auto header_layout = new QHBoxLayout;
    header_layout->addWidget(new QLabel("Partition"), 0, Qt::AlignCenter);
    header_layout->addWidget(new QLabel("Create Filesystem"), 0, Qt::AlignCenter);
    
    // root
    m_root_dev = new QComboBox;
    m_root_dev->setMaximumWidth(200);
    m_root_fs = new QComboBox();
    m_root_fs->setMaximumWidth(200);
    
    // boot
    m_boot_dev = new QComboBox;
    m_boot_dev->setMaximumWidth(200);
    m_boot_fs = new QComboBox;
    m_boot_fs->setMaximumWidth(200);

    // home
    m_home_dev = new QComboBox;
    m_home_dev->setMaximumWidth(200);
    m_home_fs = new QComboBox;
    m_home_fs->setMaximumWidth(200);

    m_home_to_root = new QCheckBox("Mount /home to root partition");
    
    auto root_layout = new QHBoxLayout;
    root_layout->addWidget(m_root_dev);
    root_layout->addWidget(m_root_fs);

    auto boot_layout = new QHBoxLayout;
    boot_layout->addWidget(m_boot_dev);
    boot_layout->addWidget(m_boot_fs);

    auto home_layout = new QHBoxLayout;
    home_layout->addWidget(m_home_dev);
    home_layout->addWidget(m_home_fs);

    m_root_fs->addItem("");
    m_root_fs->addItems(SupportedRootFs);
    m_boot_fs->addItem("");
    m_boot_fs->addItems(SupportedBootFs);
    m_home_fs->addItem("");
    m_home_fs->addItems(SupportedHomeFs);
    
    m_summary->setContentsMargins(10,0,10,0);
    m_summary->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_summary->setWordWrap(true);
    m_summary->setTextFormat(Qt::TextFormat::MarkdownText);
    m_summary->setMinimumWidth(400);
    m_summary->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);

    mounts_layout->addRow("Mount", header_layout);
    mounts_layout->addRow("/", root_layout);
    mounts_layout->addRow("/boot", boot_layout);
    mounts_layout->addRow("/home", home_layout);
    mounts_layout->addRow("", m_home_to_root);
        
    layout->addLayout(mounts_layout);
    layout->addSpacing(50);
    layout->addWidget(m_summary, 0, Qt::AlignCenter);
    layout->addStretch(1);

    setLayout(layout);

    connect(m_home_to_root, &QCheckBox::checkStateChanged, [this](const Qt::CheckState state)
    {
      if (state == Qt::Checked)
      {
        m_home_dev->setCurrentText(m_root_dev->currentText()); // triggers update_mount_data()
        m_home_dev->setEnabled(false);
        m_home_fs->setEnabled(false);
      }
      else
      {
        m_home_dev->setEnabled(true);
        m_home_fs->setEnabled(true);
      }
    });

    connect(m_root_dev, &QComboBox::currentTextChanged, [this](const QString& value)
    {
      if (m_home_to_root->checkState() == Qt::Checked)
      {
        m_home_dev->setCurrentText(value);
        m_home_fs->setCurrentText(m_root_fs->currentText());
      
        update_mount_data();
      }
    });

    connect(m_root_fs, &QComboBox::currentTextChanged, this, [this](const QString val)
    {
      if (m_home_to_root->checkState() == Qt::Checked)
      {
        m_home_fs->setCurrentText(m_root_fs->currentText());
        update_mount_data();
      }
    });
    
    
    connect(m_boot_dev, &QComboBox::currentTextChanged, this, [this](const QString val)
    {
      update_mount_data();
    });

    connect(m_boot_fs, &QComboBox::currentTextChanged, this, [this](const QString val)
    {
      update_mount_data();
    });


    // enforce and disable until implemented
    m_home_to_root->setChecked(true);
    m_home_to_root->setEnabled(false);

    update_mount_data();
  }


  void update_mount_data(const bool summary = true)
  {
    m_mounts.root.dev = m_root_dev->currentText().toStdString();
    m_mounts.root.create_fs = m_root_fs->currentIndex () != 0;
    m_mounts.root.fs = m_root_fs->currentText().toStdString();

    m_mounts.boot.dev = m_boot_dev->currentText().toStdString();
    m_mounts.boot.create_fs = m_boot_fs->currentIndex () != 0;
    m_mounts.boot.fs = m_boot_fs->currentText().toStdString();

    m_mounts.home.dev = m_home_dev->currentText().toStdString();
    m_mounts.home.create_fs = m_home_fs->currentIndex () != 0;
    m_mounts.home.fs = m_home_fs->currentText().toStdString();

    if (!m_mounts.root.create_fs)
      m_mounts.root.fs = PartitionUtils::get_partition_fs(m_mounts.root.dev);
    
    if (!m_mounts.boot.create_fs)
      m_mounts.boot.fs = PartitionUtils::get_partition_fs(m_mounts.boot.dev);

    if (!m_mounts.home.create_fs)
      m_mounts.home.fs = PartitionUtils::get_partition_fs(m_mounts.home.dev);

    if (summary)
      update_summary();
  }


  void update_summary()
  {
    const auto root_dev = QString::fromStdString(m_mounts.root.dev);
    const auto root_fs = QString::fromStdString(m_mounts.root.fs);
    const auto root_create_fs = m_mounts.root.create_fs ? "Yes" : "No";

    const auto boot_dev = QString::fromStdString(m_mounts.boot.dev);
    const auto boot_fs = QString::fromStdString(m_mounts.boot.fs);
    const auto boot_create_fs = m_mounts.boot.create_fs ? "Yes" : "No";

    const auto home_dev = QString::fromStdString(m_mounts.home.dev);
    const auto home_fs = QString::fromStdString(m_mounts.home.fs);
    const auto home_create_fs = m_mounts.home.create_fs ? "Yes" : "No";
    
    m_summary_text.clear();

    QTextStream ss{&m_summary_text};

    ss << "|Mount|Device|Filesystem|Create|\n";
    ss << "|:---|:---|:---:|:---:|\n";
    ss << "| / |"     << root_dev << "|"<< root_fs << "|" << root_create_fs << "|\n";
    ss << "| /boot |" << boot_dev << "|"<< boot_fs << "|" << boot_create_fs << "|\n";
    ss << "| /home |" << home_dev << "|"<< home_fs << "|" << home_create_fs << "|\n";

    if (root_dev == boot_dev)
      ss << "<span style=\"color:red;\">/ and /boot cannot be the same partition</span>\n";
    else
    {
      if (root_fs.isEmpty())
      {
        ss << "<span style=\"color:red;\">/ has no filesystem</span>\n";
      }

      if (boot_fs.isEmpty())  
        ss << "<span style=\"color:red;\">/boot has no filesystem</span>\n";
      else if (boot_fs != "vfat")
        ss << "<span style=\"color:red;\">/boot must be vfat</span>\n";
    }
    
    if (home_fs != root_fs && home_fs.isEmpty())
      ss << "<span style=\"color:red;\">/home has no filesystem</span>\n";

    m_summary->setText(m_summary_text);
  }


  void add_boot(const QString& dev)
  {
    m_boot_dev->addItem(dev);
  }


  void add_root(const QString& dev)
  {
    m_root_dev->addItem(dev);
  }


  void add_home(const QString& dev)
  {
    m_home_dev->addItem(dev);
  }


  MountData get_data() const
  {
    return m_mounts;
  }

private:
  QComboBox * m_boot_dev, * m_root_dev, * m_home_dev;
  QComboBox * m_boot_fs, * m_root_fs, * m_home_fs;
  QCheckBox * m_home_to_root;
  QLabel * m_summary; 
  MountData m_mounts;
  QString m_summary_text;
};


PartitionsWidget::PartitionsWidget() : ContentWidget("Mounts")
{
  QVBoxLayout * layout = new QVBoxLayout;
  setLayout(layout);
  
  QLabel * lbl_title = new QLabel;
  lbl_title->setTextFormat(Qt::MarkdownText);
  lbl_title->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  lbl_title->setWordWrap(true);
  
  layout->setAlignment(Qt::AlignTop);
  layout->addWidget(lbl_title);
    
  PartitionUtils::probe(ProbeOpts::UnMounted);

  if (PartitionUtils::have_partitions())
  { 
    lbl_title->setText(waffle_title_have_parts);

    m_mounts_widget = new SelectMounts;

    layout->addWidget(create_table());
    layout->addWidget(m_mounts_widget);    
  }
  else
  {
    lbl_title->setText(waffle_title_no_parts);
  }

  layout->addStretch(1);
}


QTableWidget * PartitionsWidget::create_table()
{
  static const int COL_DEV  = 0;
  static const int COL_FS   = 1;
  static const int COL_EFI  = 2;
  static const int COL_SIZE = 3;

  auto table = new QTableWidget(PartitionUtils::num_partitions(), 4);
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

  int row = 0;
  for(const auto& part : PartitionUtils::partitions())
  {
    const auto path = QString::fromStdString(part.dev);

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

    m_mounts_widget->add_boot(path);
    m_mounts_widget->add_home(path);
    m_mounts_widget->add_root(path);
  }

  table->setColumnWidth(COL_DEV,200);
  table->setColumnWidth(COL_FS,100);
  table->setColumnWidth(COL_EFI,100);
  table->horizontalHeader()->setStretchLastSection(true);
  table->resizeRowsToContents();

  return table;
}


bool PartitionsWidget::is_valid()
{
  const MountData mounts = m_mounts_widget->get_data();
  const bool root_ok = !mounts.root.dev.empty() && !mounts.root.fs.empty();
  const bool boot_ok = !mounts.boot.dev.empty() && !mounts.boot.fs.empty();
  const bool home_ok = !mounts.home.dev.empty() && !mounts.home.fs.empty();
  const bool boot_root_same = mounts.root.dev == mounts.boot.dev;
  
  return root_ok && boot_ok && home_ok && !boot_root_same;
}


std::pair<bool, MountData> PartitionsWidget::get_data()
{
  if (!m_mounts_widget || !is_valid())
    return {false, {}};
  else
    return {true, m_mounts_widget->get_data()};    
}

