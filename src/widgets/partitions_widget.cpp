#include <ali/widgets/partitions_widget.hpp>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QTableWidgetItem>


static const QString waffle_title_have_parts = R"!(## Partitions
This table only shows **GPT** partitions and **not mounted**.

If your partitions are not showing, return to the terminal:
- `lsblk -o PATH,PTTYPE,MOUNTPOINT <dev>`
- `findmnt`

---

`/`:
- Existing filesystem (if any) is always wiped and the new filesystem created

<br/>

`/efi`:
- If using an existing EFI partition, set 'Create Filesystem' empty
- Otherwise, set `Create Filesystem` to `vfat` (FAT32)

<br/>

`/home`:
- Either same as `/`, or a dedicated partition
- If mounting to existing partition, leave `Create Filesystem` empty
- Otherwise set `Create Filesystem` as desired
)!";


static const QString waffle_title_no_parts = R"!(## Partitions
No eligible partitions found. This table only shows **GPT** partitions and **not mounted**.

<br/>

Return to the terminal:
- `lsblk -o PATH,PTTYPE,MOUNTPOINT <dev>`
- `findmnt`

<br/>

PTTYPE must be `gpt` and MOUNTPOINT empty.

- To ummount: `umount <mount_point>`

You need a GPT label on the device, with at least two partitions (`/` and `/boot`).

<br/>

### Create GPT Label
- `fdisk <dev>`
- Enter `g`, press enter, then `w` and press Enter

<br/>

### Create Partitions
Use `fdisk <dev>` or `cfdisk <dev>`.
- Boot partition should be **at least** 512MB, preferably 1GB
- Root partition should be **at least** 8GB, preferably 32GB
)!";


static const QStringList SupportedRootFs = 
{
  "ext4"
};

static const QStringList SupportedHomeFs = 
{
  "ext4"
};

static const QStringList SupportedEfiFs = 
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
    
    // efi
    m_efi_dev = new QComboBox;
    m_efi_dev->setMaximumWidth(200);
    m_efi_fs = new QComboBox;
    m_efi_fs->setMaximumWidth(200);

    // home
    m_home_dev = new QComboBox;
    m_home_dev->setMaximumWidth(200);
    m_home_fs = new QComboBox;
    m_home_fs->setMaximumWidth(200);

    m_home_to_root = new QCheckBox("Mount /home to root partition");
    
    auto root_layout = new QHBoxLayout;
    root_layout->addWidget(m_root_dev);
    root_layout->addWidget(m_root_fs);

    auto efi_layout = new QHBoxLayout;
    efi_layout->addWidget(m_efi_dev);
    efi_layout->addWidget(m_efi_fs);

    auto home_layout = new QHBoxLayout;
    home_layout->addWidget(m_home_dev);
    home_layout->addWidget(m_home_fs);

    m_root_fs->addItem("");
    m_root_fs->addItems(SupportedRootFs);
    m_efi_fs->addItem("");
    m_efi_fs->addItems(SupportedEfiFs);
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
    mounts_layout->addRow("/efi", efi_layout);
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
        // triggers update_mount_data()
        m_home_dev->setCurrentText(m_root_dev->currentText());
        m_home_fs->setCurrentText(m_root_fs->currentText());

        m_home_dev->setEnabled(false);
        m_home_fs->setEnabled(false);
      }
      else
      {
        m_home_dev->setEnabled(true);
        m_home_fs->setEnabled(true);
      
        update_mount_data();
      }      
    });

    // root
    connect(m_root_dev, &QComboBox::currentTextChanged, [this](const QString& value)
    {
      if (m_home_to_root->checkState() == Qt::Checked)
      {
        m_home_dev->setCurrentText(value);
        m_home_fs->setCurrentText(m_root_fs->currentText());
      }

      update_mount_data();
    });


    connect(m_root_fs, &QComboBox::currentTextChanged, this, [this](const QString val)
    {
      if (m_home_to_root->checkState() == Qt::Checked)
      {
        m_home_fs->setCurrentText(m_root_fs->currentText());
        update_mount_data();
      }
    });
    
    // boot
    connect(m_efi_dev, &QComboBox::currentTextChanged, this, [this](const QString val)
    {
      update_mount_data();
    });

    connect(m_efi_fs, &QComboBox::currentTextChanged, this, [this](const QString val)
    {
      update_mount_data();
    });

    // home
    connect(m_home_dev, &QComboBox::currentTextChanged, this, [this](const QString val)
    {
      update_mount_data();
    });

    connect(m_home_fs, &QComboBox::currentTextChanged, this, [this](const QString val)
    {
      update_mount_data();
    });

    
    m_home_to_root->setChecked(true);

    update_mount_data();
  }


  void update_mount_data(const bool summary = true)
  {
    m_mounts.root.dev = m_root_dev->currentText().toStdString();
    m_mounts.root.create_fs = true;//m_root_fs->currentIndex () != 0;
    m_mounts.root.fs = m_mounts.root.create_fs ?  m_root_fs->currentText().toStdString() :
                                                  PartitionUtils::get_partition_fs(m_mounts.root.dev);

    m_mounts.efi.dev = m_efi_dev->currentText().toStdString();
    m_mounts.efi.create_fs = m_efi_fs->currentIndex () != 0;
    m_mounts.efi.fs = m_mounts.efi.create_fs ?  m_efi_fs->currentText().toStdString() :
                                                PartitionUtils::get_partition_fs(m_mounts.efi.dev);

    
    if (m_home_to_root->isChecked())
    {
      m_mounts.home.dev = m_mounts.root.dev;
      m_mounts.home.fs = m_mounts.root.fs;
      m_mounts.home.create_fs = false;
    }
    else
    {
      m_mounts.home.dev = m_home_dev->currentText().toStdString();
      m_mounts.home.create_fs = m_home_fs->currentIndex () != 0;
      m_mounts.home.fs = m_mounts.home.create_fs ?  m_home_fs->currentText().toStdString() :
                                                    PartitionUtils::get_partition_fs(m_mounts.home.dev);
    }
    
    if (summary)
      update_summary();
  }


  void update_summary()
  {
    const auto root_dev = QString::fromStdString(m_mounts.root.dev);
    const auto root_fs = m_mounts.root.fs.empty() ? "None" : QString::fromStdString(m_mounts.root.fs);
    const auto root_create_fs = m_mounts.root.create_fs ? "Yes" : "No";

    const auto efi_dev = QString::fromStdString(m_mounts.efi.dev);
    const auto efi_fs = m_mounts.efi.fs.empty() ? "None" : QString::fromStdString(m_mounts.efi.fs);
    const auto efi_create_fs = m_mounts.efi.create_fs ? "Yes" : "No";

    const auto home_dev = QString::fromStdString(m_mounts.home.dev);
    const auto home_fs = m_mounts.home.fs.empty() ? "None" : QString::fromStdString(m_mounts.home.fs);
    const auto home_create_fs = m_mounts.home.create_fs ? "Yes" : "No";
    
    m_summary_text.clear();

    QTextStream ss{&m_summary_text};

    ss << "|Mount|Device|Filesystem|Create|\n";
    ss << "|:---|:---|:---:|:---:|\n";
    ss << "| / |"     << root_dev << "|"<< root_fs << "|" << root_create_fs << "|\n";
    ss << "| /efi |"  << efi_dev << "|" << efi_fs << "|"  << efi_create_fs << "|\n";
    ss << "| /home |" << home_dev << "|"<< home_fs << "|" << home_create_fs << "|\n";

    if (root_dev == efi_dev)
      ss << "<span style=\"color:red;\">/ and /boot cannot be the same partition</span>\n";
    else if (home_dev == efi_dev)
      ss << "<span style=\"color:red;\">/home and /boot cannot be the same partition</span>\n";
    else
    {
      if (root_fs == "None")
        ss << "<span style=\"color:red;\">/ has no filesystem</span>\n";

      if (efi_fs == "None")
        ss << "<span style=\"color:red;\">/boot has no filesystem</span>\n";
      else if (efi_fs != "vfat")
        ss << "<span style=\"color:red;\">/boot must be vfat</span>\n";
    }
    
    if (home_fs != root_fs && home_fs.isEmpty())
      ss << "<span style=\"color:red;\">/home has no filesystem</span>\n";

    m_summary->setText(m_summary_text);
  }


  void add_efi(const QString& dev)
  {
    m_efi_dev->addItem(dev);
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
  QComboBox * m_efi_dev, * m_root_dev, * m_home_dev;
  QComboBox * m_efi_fs, * m_root_fs, * m_home_fs;
  QCheckBox * m_home_to_root;
  QLabel * m_summary; 
  MountData m_mounts;
  QString m_summary_text;
};


PartitionsWidget::PartitionsWidget() : ContentWidget("Mounts")
{
  QVBoxLayout * layout = new QVBoxLayout;
  setLayout(layout);
  
  QTextEdit * lbl_title = new QTextEdit;
  lbl_title->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);  
  lbl_title->setReadOnly(true);
  
  layout->setAlignment(Qt::AlignTop);
  layout->addWidget(lbl_title);
    
  PartitionUtils::probe_for_install();

  if (PartitionUtils::have_partitions())
  { 
    lbl_title->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    lbl_title->setMarkdown(waffle_title_have_parts);

    m_mounts_widget = new SelectMounts;

    layout->addWidget(create_table());
    layout->addWidget(m_mounts_widget);    
    layout->addStretch(1);
  }
  else
  {
    lbl_title->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    lbl_title->setFixedHeight(600);
    lbl_title->setMarkdown(waffle_title_no_parts);
  }
}


QTableWidget * PartitionsWidget::create_table()
{
  static const int COL_DEV      = 0;
  static const int COL_FS_CURR  = 1;
  static const int COL_EFI      = 2;
  static const int COL_SIZE     = 3;

  auto table = new QTableWidget(PartitionUtils::num_partitions(), 4);
  table->verticalHeader()->hide();
  table->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  table->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
  table->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
  table->setMinimumHeight(100);
  table->setMaximumHeight(200);
  table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  table->setHorizontalHeaderItem(COL_DEV,     new QTableWidgetItem("Device"));
  table->setHorizontalHeaderItem(COL_FS_CURR, new QTableWidgetItem("Existing Filesystem"));
  table->setHorizontalHeaderItem(COL_EFI,     new QTableWidgetItem("EFI"));
  table->setHorizontalHeaderItem(COL_SIZE,    new QTableWidgetItem("Size"));

  int row = 0;
  for(const auto& part : PartitionUtils::partitions())
  {
    const auto path = QString::fromStdString(part.dev);
    const auto fs = part.is_fat32 ? "vfat (FAT32)" : part.fs_type;

    auto item_dev = new QTableWidgetItem(path);
    auto item_fs_curr = new QTableWidgetItem(QString::fromStdString(fs));
    auto item_efi = new QTableWidgetItem(QString::fromStdString(part.is_efi ? "True" : "False"));
    auto item_size = new QTableWidgetItem(QString::fromStdString(format_size(part.size)));
    
    item_fs_curr->setTextAlignment(Qt::AlignHCenter);
    item_efi->setTextAlignment(Qt::AlignHCenter);
    
    table->setItem(row, COL_DEV, item_dev);
    table->setItem(row, COL_FS_CURR, item_fs_curr);
    table->setItem(row, COL_EFI, item_efi);
    table->setItem(row, COL_SIZE, item_size);
    ++row;

    m_mounts_widget->add_efi(path);
    m_mounts_widget->add_home(path);
    m_mounts_widget->add_root(path);
  }

  table->setColumnWidth(COL_DEV,200);
  table->setColumnWidth(COL_FS_CURR,150);
  table->setColumnWidth(COL_EFI,100);
  table->horizontalHeader()->setStretchLastSection(true);
  table->resizeRowsToContents();

  return table;
}


bool PartitionsWidget::is_valid()
{
  if (!m_mounts_widget)
    return false;
    
  const MountData mounts = m_mounts_widget->get_data();
  const bool root_ok = !mounts.root.dev.empty() && !mounts.root.fs.empty();
  const bool efi_ok = !mounts.efi.dev.empty() && !mounts.efi.fs.empty();
  const bool home_ok = !mounts.home.dev.empty() && !mounts.home.fs.empty();
  const bool efi_root_same = mounts.root.dev == mounts.efi.dev;
  
  return root_ok && efi_ok && home_ok && !efi_root_same;
}


std::pair<bool, MountData> PartitionsWidget::get_data()
{
  if (!m_mounts_widget || !is_valid())
    return {false, {}};
  else
    return {true, m_mounts_widget->get_data()};    
}

