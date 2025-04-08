#include <ali/widgets/video_widget.hpp>
#include <ali/commands.hpp>
#include <ali/packages.hpp>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>


// TODO read these from files
const static QStringList NvidiaNouveau = {"libva-mesa-driver", "mesa", "xf86-video-nouveau", "xorg-server", "xorg-xinit"};
const static QStringList NvidiaOpen = {"dkms", "nvidia-open", "nvidia-open-dkms", "xorg-server", "xorg-xinit"};
const static QStringList NvidiaClosed = {"dkms","nvidia-dkms", "xorg-server", "xorg-xinit"};

const static QStringList AmdOpen = {"libva-mesa-driver", "mesa", "vulkan-radeon", "xf86-video-amdgpu", "xf86-video-ati", "xorg-server", "xorg-xinit"};
const static QStringList IntelOpen = {/*"libva-mesa-driver",*/ "mesa", "intel-media-driver", "vulkan-intel", "xorg-server", "xorg-xinit"};
const static QStringList VmOpen = {"mesa", "xf86-video-vmware", "xorg-server", "xorg-xinit"};


struct Drivers
{
  QMap<QString, QStringList> drivers;
};


const static QMap<GpuVendor, Drivers> SourceToDrivers = 
{
  {
    GpuVendor::Nvidia, Drivers {.drivers =
                                  {
                                    {"Open Source", NvidiaNouveau},
                                    {"Nvidia Open", NvidiaOpen},
                                    {"Nvidia Propriatory", NvidiaClosed}
                                  }
                               }
  },
  {
    GpuVendor::Amd, Drivers {.drivers = { {"Open Source", AmdOpen} } }
  },
  {
    GpuVendor::Intel, Drivers {.drivers = { {"Open Source", IntelOpen} } }
  },
  {
    GpuVendor::VM, Drivers {.drivers = { {"Open Source", VmOpen} } }
  }
};


const static QMap<GpuVendor, QString> VendorToName =
{
    {GpuVendor::Amd,     "AMD"},
    {GpuVendor::Nvidia,  "Nvidia"},
    {GpuVendor::Intel,   "Intel"},
    {GpuVendor::VM,      "VM"},
    {GpuVendor::Unknown, "Unknown"}
};

const static QMap<QString, GpuVendor> NameToVendor =
{
    {"AMD",     GpuVendor::Amd},
    {"Nvidia",  GpuVendor::Nvidia},
    {"Intel",   GpuVendor::Intel},
    {"VM",      GpuVendor::VM},
    {"Unknown", GpuVendor::Unknown}
};



VideoWidget::VideoWidget() : ContentWidget("Video")
{
  QFormLayout * layout = new QFormLayout;
  
  m_vendor = new QComboBox;
  m_vendor->addItems({"", "Nvidia", "AMD", "Intel", "VM"});
  m_vendor->setMaximumWidth(175);

  m_source_model = new QComboBox;
  m_source_model->setMaximumWidth(175);

  m_packages = new QListWidget;
  m_packages->setMaximumHeight(150);
  m_packages->setMaximumWidth(175);
  m_packages->setEnabled(false);
  m_packages->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);


  connect(m_vendor, &QComboBox::currentTextChanged, this, [this](const QString& vendor)
  { 
    vendor_changed(vendor);
  });

  connect(m_source_model, &QComboBox::currentTextChanged, this, [this](const QString& model)
  {     
    const auto packages = SourceToDrivers[m_video_vendor].drivers[model];

    m_packages->clear();
    m_packages->insertItems(0, packages);

    Packages::set_video_packages(packages);
  });


  layout->addRow("Vendor", m_vendor);
  layout->addRow("Source Model", m_source_model);
  layout->addRow("Packages", m_packages);
  
  get_vendor();

  setLayout(layout);
}


void VideoWidget::get_vendor()
{
  VideoVendor video_cmd;
  m_video_vendor = video_cmd.get_vendor();

  qDebug() << "GPU vendor: " << VendorToName[m_video_vendor];

  m_vendor->setCurrentText(VendorToName[m_video_vendor]);

  // force update of UI
  vendor_changed(m_vendor->currentText());
}


void VideoWidget::vendor_changed(const QString& vendor)
{
  m_source_model->clear();

  if (!vendor.isEmpty())
  {
    m_video_vendor = NameToVendor[vendor];

    if (vendor == "Nvidia")
      m_source_model->addItems({"Open Source", "Nvidia Open", "Nvidia Propriatory"});
    else if (vendor == "AMD")
      m_source_model->addItems({"Open Source"});
      else if (vendor == "Intel")
      m_source_model->addItems({"Open Source"});
    else if (vendor == "VM")
      m_source_model->addItems({"Open Source"});
  }
}


bool VideoWidget::is_valid()
{
  return true;
}