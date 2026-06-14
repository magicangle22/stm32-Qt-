#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle("STM32 OLED Image Sender");
    window.resize(700, 550);
    window.show();

    return app.exec();
}
