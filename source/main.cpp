#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //设置支持中文
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    MainWindow w;
    //固定大小(800,600)
    w.setFixedSize(800,600);
    w.show();
    
    return a.exec();
}
