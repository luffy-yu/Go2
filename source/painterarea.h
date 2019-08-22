#ifndef PAINTERAREA_H
#define PAINTERAREA_H

#include <QWidget>
#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

#include <sys/time.h>
#include <windows.h>
#include <QProcess>
#include <QApplication>
#include "go2algorithm.h"

class PainterArea : public QWidget
{
    Q_OBJECT
public:
    explicit PainterArea(QWidget *parent = 0);
    //清除绘制的图形
    void clearAllPath();
    //显示线段和圆的统计数据并绘制图形
    void showLinesAndCirclesReport();
    //修改need2Close为true，关闭窗口。
    void closeWindow();
private:
    //计算圆弧上的点的距3点钟方向的角度
    double getAngle(CirclePoint *cl, Point *p1);
    //计算圆的外接矩形
    QRectF getCircleRect(Circle *cl);
    //计算圆的外接矩形
    QRectF getCircleRect(int x,int y,int r);
    //计算绘制圆弧的起始角度和跨越的角度
    int *get2Angles(CirclePoint *cl,Point *p1,Point *p2);
    //查询内存消耗
    QString getUsedMemory(DWORD pid);
    //生成线段的统计数据文本
    QString generateLineStatisticText();
    //生成圆的统计数据文本
    QString generateCircleStatisticText();

public slots:
    //完整显示 - 裁剪显示的切换
    void switchShowMode();

protected:
    //绘制图形
    void paintEvent(QPaintEvent *event);

private:
    //映射的绘图区域大小
    QRect rect;
    //裁剪显示 - 完整显示按钮
    QPushButton *switchBtn;
    //显示文字
    QLabel *textLabel;
    //是否只显示背景
    bool onlyShowBackGround;
    //是否显示完整的图形
    bool showOrigin;
    //备份显示的文字
    QString backupText;
    //是否需要关闭窗口，如果true则停止绘制，关闭窗口。volatile可保证变量值为最新。
    volatile bool need2Close;
};

#endif // PAINTERAREA_H
