#ifndef ALI_PACKAGESWIDGET_H
#define ALI_PACKAGESWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QSet>
#include <QString>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>


struct SelectPackagesWidget;
struct AdditionalPackagesWidget;


struct Searcher : public QObject
{
  Q_OBJECT

public:
  inline static const char * base_url {"https://www.archlinux.org/packages/search/json/?arch=x86_64&name="};

  Searcher(Searcher&&) noexcept = default;

  Searcher(std::shared_ptr<QNetworkAccessManager> network_manager, const QString& name) noexcept
  {
    const auto query = std::format("{}{}", base_url, name.toStdString());

    qDebug() << "Sending query: " << query;

    QNetworkRequest request {QUrl{QString::fromStdString(query)}};
    request.setTransferTimeout(std::chrono::seconds{5});

    m_reply.reset(network_manager->get(request));

    connect(m_reply.get(), &QNetworkReply::finished, [this, name]()
    {
      qDebug() << "Have response for: " << name;

      bool found{false};

      const auto data = m_reply->readAll();

      if (const auto doc = QJsonDocument::fromJson(data); doc.isNull())
        qCritical() << "Package searched returned invalid JSON";
      else if (!doc.isObject())
        qCritical() << "Package search JSON root is not an object";
      else if (const auto root = doc.object(); !(root.contains("results") && root["results"].isArray()))
        qCritical() << "JSON does not contian 'results' or it is not an array";
      else
        found = !root["results"].toArray().isEmpty(); 

      emit on_finish(name, found);
    });
  }
  
  signals:
    void on_finish(const QString& name, const bool found);

  private:
    std::unique_ptr<QNetworkReply> m_reply;
};


struct PackageSearch : public QObject
{
  Q_OBJECT

public:

  PackageSearch() : m_network_manager(new QNetworkAccessManager)
  {

  }

  void search (const QString& packages_string)
  {
    m_exist.clear();
    m_invalid.clear();
    m_reply_count = 0;

    auto packages = packages_string.tokenize(u' ', Qt::SkipEmptyParts).toContainer();

    if (packages.isEmpty())
    {
      emit on_complete(m_exist, m_invalid);
      return;
    }

    m_expected_reply_count = packages.size();

    for (const auto& name : packages)
    {
      if (name.empty())
        continue;

      qDebug() << "Create searcher for " << name;

      auto searcher = std::make_shared<Searcher>(m_network_manager, name.toString());
      m_searchers.push_back(searcher);

      connect(searcher.get(), &Searcher::on_finish, this, [this](const QString& name, const bool found)
      {
        ++m_reply_count;

        if (found)
          m_exist.append(name);
        else
          m_invalid.append(name);

        if (m_reply_count == m_expected_reply_count)
        {
          m_searchers.clear();
          emit on_complete(m_exist, m_invalid);
        }
      });
    }
  }


  signals:
    void on_complete(const QStringList exist, const QStringList invalid);

private:
  QStringList m_exist;
  QStringList m_invalid;
  QString m_err;
  qsizetype m_reply_count{0}, m_expected_reply_count{0};
  std::vector<std::shared_ptr<Searcher>> m_searchers;
  std::shared_ptr<QNetworkAccessManager> m_network_manager;
};


struct PackagesWidget : public ContentWidget
{
  PackagesWidget();
  virtual ~PackagesWidget() = default;

  virtual bool is_valid() override;

private:
  SelectPackagesWidget * m_required;
  SelectPackagesWidget * m_kernels;
  SelectPackagesWidget * m_firmware;  
  SelectPackagesWidget * m_important;
  SelectPackagesWidget * m_shell;
  AdditionalPackagesWidget * m_additional;
};


#endif
