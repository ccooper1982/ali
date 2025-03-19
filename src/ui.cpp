
#include <iostream>
#include <map>
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


struct ContentWidget : public QWidget
{
  ContentWidget(const QString& nav_name) : m_nav_name(nav_name)
  {
    
  }


  virtual const QString get_nav_name() const
  {
    return m_nav_name;
  }


private:
  QString m_nav_name;
};


struct WelcomeWidget : public ContentWidget
{
  WelcomeWidget(const QString& nav_name) : ContentWidget(nav_name)
  {
    QVBoxLayout * layout = new QVBoxLayout;
    layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QLabel * label = new QLabel("Welcome");
    layout->addWidget(label);

    setLayout(layout);
  }
};


struct NetworkWidget : public ContentWidget
{
  NetworkWidget(const QString& nav_name) : ContentWidget(nav_name)
  {
    QFormLayout * network_layout = new QFormLayout;
    QLineEdit * ledit_hostname = new QLineEdit;
    network_layout->addRow("Hostname: ", ledit_hostname);

    setLayout(network_layout);
  }
};


struct NavTree : public QTreeView
{
private:
  const std::vector<ContentWidget*> NavItemToWidget = 
  {
    new WelcomeWidget{"Welcome"},
    new NetworkWidget{"Network"}
    // Locales, Partitions, Bootloader, Accounts, Profile, Audio, Video, Packages
  };

public:

  explicit NavTree(QLayout * const central_layout) : m_central_layout(central_layout)
  {
    m_model = new QStandardItemModel{0,0};

    for (const auto item : NavItemToWidget)
      m_model->appendRow(new QStandardItem(item->get_nav_name()));

    setModel(m_model);
    setHeaderHidden(true);
    setMaximumWidth(200);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection& selected, const QItemSelection& deselected)
    {
      if (const QModelIndexList selectedIndexes = selected.indexes() ; !selectedIndexes.isEmpty())
      {
        const QModelIndex index = selectedIndexes.first(); 
        const QVariant data = m_model->data(index, Qt::DisplayRole);         

        if (index.row() >= 0 && index.row() < std::ssize(NavItemToWidget))
        {
          auto next = NavItemToWidget[index.row()];

          if (m_current)
          {
            m_current->hide();
            m_central_layout->replaceWidget(m_current, next, Qt::FindDirectChildrenOnly);
          }
          else
          {
            m_central_layout->addWidget(next);
          }

          m_current = next;
          m_current->show();
        }
      }
    });
  }

  ContentWidget * get_welcome_widget()
  {
    return NavItemToWidget[0];
  }

  void show_welcome()
  {
    //QItemSelection s(1);
    //selectionModel()->select(s, QItemSelectionModel::ClearAndSelect);
    setCurrentIndex(model()->index(0,0));
  }

private:
  QLayout * m_central_layout;
  QWidget * m_current{nullptr};
  QStandardItemModel * m_model;
  QModelIndex m_welcome_index;
};




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

  NavTree * nav_tree = new NavTree(central_layout);
  
  central_layout->addLayout(nav_layout);
  //central_layout->addWidget(content_widget, 1); 
  
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

  nav_layout->addWidget(nav_tree);

  // content
  //central_layout->addWidget(wgt_content);

  // always show Welcome initially
  nav_tree->show_welcome();

  window.show();
  return app.exec();
}
