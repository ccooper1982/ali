#ifndef ALI_SWAPWIDGET_H
#define ALI_SWAPWIDGET_H

#include <ali/widgets/content_widget.hpp>
#include <QFormLayout>
#include <QCheckBox>


struct SwapWidget : public ContentWidget
{
  struct SwapData
  {
    bool zram_enabled{true};
  };

  SwapWidget() : ContentWidget("Swap")
  {
    QFormLayout * layout = new QFormLayout;

    m_zram = new QCheckBox("RAM disk with filesystem that compresses data");
    m_zram->setChecked(true);

    layout->addRow("Use zram", m_zram);

    setLayout(layout);
  }

  virtual bool is_valid() override
  {
    return true;
  }

  SwapData get_data() const
  {
    return SwapData { .zram_enabled = m_zram->isChecked()};
  }

private:
  QCheckBox * m_zram;
};


#endif
