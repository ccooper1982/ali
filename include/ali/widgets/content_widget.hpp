#ifndef ALI_CONTENTWIDGET_H
#define ALI_CONTENTWIDGET_H

#include <QtWidgets>


struct ContentWidget : public QWidget
{
  ContentWidget(const QString& nav_name) : m_nav_name(nav_name)
  {
    
  }

  
  virtual ~ContentWidget() = default;


  virtual const QString get_nav_name() const
  {
    return m_nav_name;
  }


private:
  QString m_nav_name;
};


#endif
