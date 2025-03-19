
#include <iostream>
#include <qt/QtWidgets/QApplication>
#include <qt/QtWidgets/QLabel>
#include <qt/QtWidgets/QFormLayout>
#include <qt/QtWidgets/QHBoxLayout>
#include <qt/QtWidgets/QVBoxLayout>
#include <qt/QtWidgets/QLineEdit>
#include <qt/QtWidgets/QPushButton>
#include <qt/QtWidgets/QTreeView>
#include <QStandardItemModel>
#include <qt/QtWidgets/QMainWindow>


struct NavTree : public QTreeView
{
  NavTree()
  {
    m_model = new QStandardItemModel{0,0};

    QStandardItem * item_locale = new QStandardItem("Locale");
    QStandardItem * item_partitions = new QStandardItem("Partitions");
    QStandardItem * item_kernels = new QStandardItem("Kernels");
    QStandardItem * item_network = new QStandardItem("Network");
    QStandardItem * item_accounts = new QStandardItem("Accounts");
    QStandardItem * item_video = new QStandardItem("Video");
    QStandardItem * item_audio = new QStandardItem("Audio");
    QStandardItem * item_profiles = new QStandardItem("Profiles");
    QStandardItem * item_packages = new QStandardItem("Packages");


    m_model->appendRow(item_locale);      // keymap, timezone, NTP
    m_model->appendRow(item_partitions);  // /boot, /, /home, swap
    m_model->appendRow(item_kernels);     // linux, linux-lts, linux-zen
    m_model->appendRow(item_accounts);    // set root passwd, create user account
    m_model->appendRow(item_network);     // hostname, copy config to install
    m_model->appendRow(item_video);
    m_model->appendRow(item_audio);
    m_model->appendRow(item_profiles);    // no desktop, with desktop+select WM and DE
    m_model->appendRow(item_packages);    // additional packages (standard: base, linux-firmware, <kernels>, <gpu>)

    setModel(m_model);
    setHeaderHidden(true);
    setMaximumWidth(200);

    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection& selected, const QItemSelection& deselected)
    {
      QModelIndexList selectedIndexes = selected.indexes(); 
      if (!selectedIndexes.isEmpty())
      {
          QModelIndex index = selectedIndexes.first(); 
          QVariant data = m_model->data(index, Qt::DisplayRole); 
          qDebug() << "Selected item:" << data.toString(); 
      }
    });
  }

  private:
    QStandardItemModel * m_model;
};


QTreeView * create_nav_tree()
{
  NavTree * tree = new NavTree;
  
  return tree;
}


int main (int argc, char ** argv)
{
  #ifdef ALI_PROD
    std::cout << "Production Mode\n";
  #elif defined(ALI_DEV)
    std::cout << "Dev Mode\n";
  #else
    static_assert(false, "ALI_PROD or ALI_DEV must be defined");
  #endif

  QApplication app(argc, argv);
  
  QHBoxLayout * central_layout = new QHBoxLayout;
  QVBoxLayout * nav_layout = new QVBoxLayout;
  QWidget * wgt_content = new QWidget;
 

  central_layout->addLayout(nav_layout);
  central_layout->addWidget(wgt_content, 1);
  
  
  QWidget * wgt_central = new QWidget;
  wgt_central->setLayout(central_layout);

  QMainWindow window;
  window.setWindowTitle("Arch Linux Installer");
  window.resize(800, 600);
  window.setCentralWidget(wgt_central);

  // nav
  // QPalette pal; // no affect
  // pal.setColor(QPalette::ColorRole::Window, Qt::white);
  // wgt_content->setAutoFillBackground(true);
  // wgt_content->setPalette(pal);

  nav_layout->addWidget(create_nav_tree());

  // QPushButton * btn_partitions = new QPushButton("Partitions");
  // QPushButton * btn_network = new QPushButton("Network");
  // nav_layout->addWidget(btn_partitions, Qt::AlignHCenter);
  // nav_layout->addWidget(btn_network, Qt::AlignHCenter);

  // content
  central_layout->addWidget(wgt_content);

  //
  QFormLayout * network_layout = new QFormLayout;
  QLineEdit * ledit_hostname = new QLineEdit;
  network_layout->addRow("Hostname: ", ledit_hostname);
  
  //
  central_layout->addLayout(network_layout);

  window.show();
  return app.exec();
}
