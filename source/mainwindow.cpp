#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    //初始化菜单和命令
    functionTestMenu = new QMenu(tr("功能测试(&F)"),this);
    efficiencyTestMenu = new QMenu(tr("效率测试(&E)"),this);
    aboutMenu = new QMenu(tr("关于(&A)"),this);
    test1Action = new QAction(tr("测试1(&1)"),this);
    test2Action = new QAction(tr("测试2(&2)"),this);
    test3Action = new QAction(tr("测试3(&3)"),this);
    test4Action = new QAction(tr("测试4(&4)"),this);
    test5Action = new QAction(tr("测试5(&5)"),this);
    aboutAction = new QAction(tr("关于Go2(&G)"),this);
    aboutQt = new QAction(tr("关于Qt(&Q)"),this);

    //添加命令
    functionTestMenu->addAction(test1Action);
    functionTestMenu->addAction(test2Action);
    functionTestMenu->addAction(test3Action);
    functionTestMenu->addAction(test4Action);
    efficiencyTestMenu->addAction(test5Action);
    aboutMenu->addAction(aboutAction);
    aboutMenu->addAction(aboutQt);

    //添加菜单
    this->menuBar()->addMenu(functionTestMenu);
    this->menuBar()->addMenu(efficiencyTestMenu);
    this->menuBar()->addMenu(aboutMenu);

    //关联信号和槽
    connect(test1Action,SIGNAL(triggered()),this,SLOT(test1Slot()));
    connect(test2Action,SIGNAL(triggered()),this,SLOT(test2Slot()));
    connect(test3Action,SIGNAL(triggered()),this,SLOT(test3Slot()));
    connect(test4Action,SIGNAL(triggered()),this,SLOT(test4Slot()));
    connect(test5Action,SIGNAL(triggered()),this,SLOT(test5Slot()));
    connect(aboutAction,SIGNAL(triggered()),this,SLOT(aboutSlot()));
    connect(aboutQt,SIGNAL(triggered()),qApp,SLOT(aboutQt()));

    //初始化绘图区域
    painterArea = new PainterArea;
    //设置中心Widget
    this->setCentralWidget(painterArea);

    //测试文件名
    file1 = "TestData1.xml";
    file2 = "TestData2.xml";
}

MainWindow::~MainWindow()
{

}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 得到应用程序当前文件夹测试文件的完整路径
  */
QString MainWindow::getTestFilePath(QString filename)
{
    // 当前路径 + '\\' + filename
    return QString(QApplication::applicationDirPath() + "\\" + QString(filename));
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 点击"测试1"后执行的语句
  */
void MainWindow::test1Slot()
{
    setWindowTitle("Go2");
    //初始化
    clearFormerState();
    //清除绘图区域图形
    painterArea->clearAllPath();
    //读取文件，成功后统计数据。
    if(readXmlFileFiltedByID(getTestFilePath(file1),1)){
#ifdef QT_DEBUG
        qDebug()<<"read file finished";
#endif
        setWindowTitle(tr("Go2 - 功能测试1"));
        painterArea->showLinesAndCirclesReport();
    }
}

void MainWindow::test2Slot()
{
    setWindowTitle("Go2");
    clearFormerState();
    painterArea->clearAllPath();
    if(readXmlFileFiltedByID(getTestFilePath(file1),2)){
        setWindowTitle(tr("Go2 - 功能测试2"));
        painterArea->showLinesAndCirclesReport();
    }
}

void MainWindow::test3Slot()
{
    setWindowTitle("Go2");
    clearFormerState();
    painterArea->clearAllPath();
    if(readXmlFileFiltedByID(getTestFilePath(file1),3)){
        setWindowTitle(tr("Go2 - 功能测试3"));
        painterArea->showLinesAndCirclesReport();
    }
}

void MainWindow::test4Slot()
{
    clearFormerState();
    painterArea->clearAllPath();
    if(readXmlFileFiltedByID(getTestFilePath(file1),4)){
        setWindowTitle(tr("Go2 - 功能测试4"));
        painterArea->showLinesAndCirclesReport();
    }
}

void MainWindow::test5Slot()
{
    setWindowTitle("Go2");
    clearFormerState();
    painterArea->clearAllPath();
    if(readXmlFileAll(getTestFilePath(file2))){
        setWindowTitle(tr("Go2 - 效率测试5"));
        painterArea->showLinesAndCirclesReport();
    }
}

void MainWindow::aboutSlot()
{
    QMessageBox::about(this,tr("关于Go2"),tr("<h3>作品名:Go2<br>"
                                           "赛题组类:A类赛题<br>"
                                           "赛题名称:矢量图形（line和circle）在非自交多边形边界中的裁剪显示<br>"
                                           "作者:于留传 隋 燕<br>"
                                           "指导老师:江 涛<br>"
                                           "Email:ylch_top@sina.com<br>"
                                           "<font color = red>From:山东科技大学测绘学院遥感系</font></h3>"));
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 关闭软件前清理资源
  */
void MainWindow::closeEvent(QCloseEvent */*event*/)
{
    //painterArea停止绘制
    painterArea->closeWindow();
    if(clearFormerState()){
        close();
    }
}
