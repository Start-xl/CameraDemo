#include "MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // QFile styleFile(":/style/style.qss");
    // if (styleFile.open(QFile::ReadOnly)) {
    //     QString styleSheet = QString(styleFile.readAll());
    //     a.setStyleSheet(styleSheet); // 应用样式表
    //     styleFile.close();
    // } else {
    //     qWarning() << "Failed to load styleSheet:" << styleFile.errorString();
    // }

    MainWindow w;
    w.show();
    return a.exec();
}
