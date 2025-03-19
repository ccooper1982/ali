
#include <iostream>
#include <qt/QtWidgets/QApplication>
#include <qt/QtWidgets/QLabel>
#include <qt/QtWidgets/QMainWindow>

// sudo mount -t tmpfs -o size=10M tmpfs /mnt/mytmpfs

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
  
  QMainWindow window;
  window.setWindowTitle("Arch Linux Installer");
  window.resize(400, 300);
  window.show();
  return app.exec();
}
