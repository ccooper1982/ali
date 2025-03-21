
#include <iostream>
#include <fstream>
#include <map>
#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QLabel>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QStandardItemModel>
#include <QMainWindow>
#include <QStyleFactory>
#include <ali/widgets/widgets.hpp>
#include <ali/common.hpp>


static const QString log_format{"%{type} - %{message}"};
static QFile log_file{InstallLogPath};
static QFile log_file_alt{"./" / InstallLogPath.filename()};
static QTextStream log_stream;


void log_handler(const QtMsgType type, const QMessageLogContext& ctx, const QString& m)
{
  log_stream << qFormatLogMessage(type, ctx, m) << '\n';
  log_stream.flush();
}


struct NavTree : public QTreeView
{
public:

  explicit NavTree(QLayout * const centre_layout) : m_centre_layout(centre_layout)
  {
    m_model = new QStandardItemModel{0,1};

    for (const auto item : Widgets::all())
      m_model->appendRow(new QStandardItem(item->get_nav_name()));

    setModel(m_model);
    setHeaderHidden(true);
    setMinimumWidth(100);
    setMaximumWidth(150);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    setFrameStyle(QFrame::NoFrame);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection& selected, const QItemSelection& deselected)
    {
      if (const QModelIndexList selectedIndexes = selected.indexes() ; !selectedIndexes.isEmpty())
      {
        const QModelIndex index = selectedIndexes.first(); 
        const QVariant data = m_model->data(index, Qt::DisplayRole);         

        if (index.row() >= 0 && index.row() < std::ssize(Widgets::all()))
        {
          auto next = Widgets::all()[index.row()];

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
          // need setFocus(), so the InstallWidget triggers a validation check
          // can't do in the InstallWidget::InstallWidget() because it creates
          // a recursive call stack 
          m_current->setFocus(Qt::FocusReason::ActiveWindowFocusReason);
        }
      }
    });
  }

  ~NavTree()
  {
    for(auto& item : Widgets::all())
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
  
  if (QStyleFactory::keys().contains("Fusion"))
  {
    // TODO 
    //  https://stackoverflow.com/questions/48256772/dark-theme-for-qt-widgets
    //  https://github.com/Alexhuszagh/BreezeStyleSheets
    QApplication::setStyle(QStyleFactory::create("Fusion"));
  }

  // main window contains a central widget which has a horizontal layout,
  // to which the Nav is added first. The layout is passed to the 
  // navigation tree so it can add/remove widgets as nav items are 
  // selected. 
  QHBoxLayout * centre_layout = new QHBoxLayout;
  NavTree * nav_tree = new NavTree(centre_layout);

  centre_layout->setContentsMargins(0,0,0,0);
  centre_layout->addWidget(nav_tree);

  QWidget * centre_widget = new QWidget;
  centre_widget->setContentsMargins(0,0,0,0);
  centre_widget->setLayout(centre_layout);

  QMainWindow window;
  window.setWindowTitle("ali");
  window.resize(800, 600);  // TODO may be unsuitable
  window.setCentralWidget(centre_widget);

  // always show Welcome initially
  nav_tree->show_welcome();

  window.show();

  if (log_file.open(QFile::WriteOnly | QFile::Truncate))
  {
    log_stream.setDevice(&log_file);
  }
  else
  {
    const fs::path alt_path{fs::current_path() / InstallLogPath.filename()};

    std::string msg{"Cannot open preferred log file for writing:\n" + InstallLogPath.string() + '\n'};

    if (log_file_alt.open(QFile::WriteOnly | QFile::Truncate))
    {
      msg += "Using alternative:\n" + alt_path.string() ;
      log_stream.setDevice(&log_file_alt);
    }
    else
      msg += "Alternative failed: " + alt_path.string();

    QMessageBox::warning(&window, "Log", QString{msg.c_str()});
  }

  qSetMessagePattern(log_format);
  qInstallMessageHandler(log_handler);

  return app.exec();
}
