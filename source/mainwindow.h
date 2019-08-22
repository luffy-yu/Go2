#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include "painterarea.h"

#ifdef QT_DEBUG
#include <QDebug>
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void test1Slot();//点击命令后执行的语句
    void test2Slot();
    void test3Slot();
    void test4Slot();
    void test5Slot();
    void aboutSlot();

protected:
    void closeEvent(QCloseEvent */*event*/);
private:
    QMenu *functionTestMenu;//功能测试菜单
    QAction *test1Action;//功能测试1
    QAction *test2Action;//功能测试2
    QAction *test3Action;//功能测试3
    QAction *test4Action;//功能测试4
    QMenu *efficiencyTestMenu;//效率测试菜单
    QAction *test5Action;//测试5

    QMenu *aboutMenu;//关于菜单
    QAction *aboutAction;//关于
    QAction *aboutQt;//关于Qt

    PainterArea *painterArea;//绘图区域

    QString file1;//功能测试文件名
    QString file2;//效率测试文件名
    QString getTestFilePath(QString filename);//得到文件的完整路径
};

#endif // MAINWINDOW_H
