
#include <iostream>
#include <map>
#include <ali/widgets/welcome_widget.hpp>
#include <ali/widgets/content_widget.hpp>
#include <ali/widgets/partitions_widget.hpp>
#include <QApplication>
#include <QLabel>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QStandardItemModel>
#include <QMainWindow>


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
    new PartitionsWidget{"Partitions"},
    new NetworkWidget{"Network"}
    // Locales, Bootloader, Accounts, Profile, Audio, Video, Packages
  };

public:

  explicit NavTree(QLayout * const centre_layout) : m_centre_layout(centre_layout)
  {
    m_model = new QStandardItemModel{0,1};

    for (const auto item : NavItemToWidget)
      m_model->appendRow(new QStandardItem(item->get_nav_name()));

    setModel(m_model);
    setHeaderHidden(true);
    setMaximumWidth(200);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    setFrameStyle(QFrame::NoFrame);

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
            m_centre_layout->replaceWidget(m_current, next, Qt::FindDirectChildrenOnly);
          }
          else
          {
            m_centre_layout->addWidget(next);
          }

          m_current = next;
          m_current->show();
        }
      }
    });
  }

  ~NavTree()
  {
    for(auto& item : NavItemToWidget)
      delete item;
  }

  void show_welcome()
  {
    setCurrentIndex(model()->index(0,0));
  }

private:
  QLayout * m_centre_layout;
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
  
  // main window contains a widget which has a horizontal layout,
  // to which the Nav is added first. The layout is passed to the 
  // navigation tree so it can add/remove widgets as nav items are 
  // selected. 

  QHBoxLayout * centre_layout = new QHBoxLayout;
  NavTree * nav_tree = new NavTree(centre_layout);

  centre_layout->setContentsMargins(0,5,0,0);
  centre_layout->addWidget(nav_tree);

  QWidget * centre_widget = new QWidget;
  centre_widget->setContentsMargins(0,0,0,0);
  centre_widget->setLayout(centre_layout);

  QMainWindow window;
  window.setWindowTitle("ali");
  window.resize(800, 600);  // TODO may not be suitable
  window.setCentralWidget(centre_widget);

  // nav
  // QPalette pal; // no affect
  // pal.setColor(QPalette::ColorRole::Window, Qt::white);
  // wgt_content->setAutoFillBackground(true);
  // wgt_content->setPalette(pal);

  // always show Welcome initially
  nav_tree->show_welcome();

  window.show();

  return app.exec();
}
