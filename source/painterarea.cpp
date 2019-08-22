#include "painterarea.h"

extern vector<Point> lineEntityPoints;//线段
extern vector<Point> boundaryPoints;//裁剪窗口(边界)
extern vector<Circle> circleEntitys;//圆

extern LineClipStatistic lineClipStatistic;//线段的统计数据
extern CirCleClipStatistic circleClipStatistic;//圆的统计数据

extern vector<LineFlag> linesFlags;//标记线段

//extern vector<Point> clipLinePoints;//裁剪线段的点
extern vector<Point> clipedLinePoints;//裁剪后的线段

extern vector<CircleFlag> circleFlags;//标记圆

//extern vector<CirclePoint> clipCirclePoints;//裁剪圆的点
extern vector<CirclePoint> clipedCircleArcs;//裁剪后的圆弧

PainterArea::PainterArea(QWidget *parent) :
    QWidget(parent)
{
    //初始化
    QHBoxLayout *mainLayout = new QHBoxLayout;

    QVBoxLayout *layoutL = new QVBoxLayout;
    switchBtn = new QPushButton(tr("裁剪显示"),this);
    switchBtn->setFixedWidth(100);

    connect(switchBtn,SIGNAL(clicked()),this,SLOT(switchShowMode()));

    layoutL->addWidget(switchBtn,0,Qt::AlignBottom);
    mainLayout->addLayout(layoutL);

    textLabel = new QLabel(tr(""),this);
    //设置字体颜色为红色
    textLabel->setStyleSheet("color:red");

    QVBoxLayout *layoutR = new QVBoxLayout;
    layoutR->addWidget(textLabel,0,Qt::AlignTop | Qt::AlignRight);
    mainLayout->addLayout(layoutR);

    setLayout(mainLayout);

    rect.setLeft(0);
    rect.setTop(0);
    rect.setWidth(1440);
    rect.setHeight(900);

    showOrigin = true;
    //初始化为false
    need2Close = false;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 因为绘图的零度是三点钟方向，
  * 所以用余弦定理计算圆弧上某点的角度，
  * acos()返回值为[0,PI]。
  * 如果点在圆心的上方，则用360减去返回值。
  * 由于物理坐标是以左上角为原点，
  * 逻辑坐标要以左下角为原点，
  * 所以需要进行变换。
  * 即 x1 = x0，y1 = height - y0。
  */
double PainterArea::getAngle(CirclePoint *cl, Point *p1)
{
    int centerX = cl->x;
    int centerY = rect.height() - cl->y;
    int radius = cl->r;

    double px = p1->x;
    double py = rect.height() - p1->y;

    //3点钟方向
    double p3x = centerX + radius;
    double p3y = centerY;

    double a2 = (px - p3x) * (px - p3x) + (py - p3y) * (py - p3y);
    double b2 = radius * radius;
    //double c2 = b2;

    double cosa = (b2 * 2 - a2) / (2 * b2);

    //任意角度的余弦值为[-1.0,1.0];
    //上式可简化为1 - a2 / (2 * b2)，
    //所以如果值小于-1.0，则应取-1.0。
    if(cosa < - 1.0){
        cosa = -1.0;
    }
    //换算成角度
    double angle =  acos(cosa) / 3.1415926 * 180;
#ifdef QT_DEBUG
    qDebug()<<"angle:"<<angle;
#endif
    if(py < centerY){
        return (360 - angle);
    }
    return angle;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算变换后的圆的外接矩形
  */
QRectF PainterArea::getCircleRect(Circle *cl)
{
    return QRectF(cl->x - cl->r,rect.height() - cl->y - cl->r,2 * cl->r,2 * cl->r);
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算变换后的圆的外接矩形
  */
QRectF PainterArea::getCircleRect(int x, int y, int r)
{
    return QRectF(x - r,rect.height() - y - r,2 * r,2 *r);
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算绘制圆弧时的起始角度和跨越角度。
  */
int *PainterArea::get2Angles(CirclePoint *cl, Point *p1, Point *p2)
{
    int *angles = new int[2];
    angles[0] = 0;
    angles[1] = 0;

    angles[0] = int(getAngle(cl,p1));
    angles[1] = int(getAngle(cl,p2));

#ifdef QT_DEBUG
    qDebug()<<"angle1:"<<p1->x<<p1->y<<angles[0];
    qDebug()<<"angle2:"<<p2->x<<p2->y<<angles[1];
#endif
    angles[0] = angles[0];
    //如果末点角度小于起点角度，则加上360度后与起点角度求差，得到跨越角度
    angles[1] = angles[1] < angles[0] ? (angles[1] + 360 - angles[0]) : (angles[1] - angles[0]);

    //在QT中，一度用16表示，
    //正数代表逆时针，负数代表逆时针。
    angles[0] *= (-16);
    angles[1] *= (-16);

    return angles;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 查询程序占用内存。
  * 思路：通过调用外部命令'tasklist /FI "PID EQ pid"'。
  * 将返回的字符串首先替换掉','，
  * 然后用正则表达式匹配已KB为单位表示内存的字符串，
  * 最后换算为MB为单位返回。
  */
QString PainterArea::getUsedMemory(DWORD pid)
{
    char pidChar[25];
    //将DWORD类型转换为10进制的char*类型
    _ultoa(pid,pidChar,10);

    //调用外部命令
    QProcess p;
    p.start("tasklist /FI \"PID EQ " + QString(pidChar) + " \"");
    p.waitForFinished();
    //得到返回结果
    QString result = QString::fromLocal8Bit(p.readAllStandardOutput());
    //关闭外部命令
    p.close();

    //替换掉","
    result = result.replace(",","");
    //匹配 '数字+空格+K'部分。
    QRegExp rx("(\\d+)(\\s)(K)");
    //初始化结果
    QString usedMem("");
    if(rx.indexIn(result) != -1){
        //匹配成功
        usedMem = rx.cap(0);
    }
    //截取K前面的字符串，转换为数字，供换算单位使用。
    usedMem = usedMem.left(usedMem.length() - 1);
    //换算为MB的单位
    return QString::number(usedMem.toDouble() / 1024) + " MB";
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 生成线段的统计数据文字
  */
QString PainterArea::generateLineStatisticText()
{
    QString str("");
    if(lineClipStatistic.total == 0)
        return str;
    str.append(tr("<h2>直线总数: %1<br>").arg(lineClipStatistic.total));
    str.append(tr("在内部: %1<br>").arg(lineClipStatistic.inCount));
    str.append(tr("在外部: %1<br>").arg(lineClipStatistic.outCount));
    str.append(tr("相交: %1<br>").arg(lineClipStatistic.crossCount));
    str.append(tr("交点数: %1<br></h2>").arg(lineClipStatistic.crossPointCount));

    return str;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 生成圆的统计数据文字
  */
QString PainterArea::generateCircleStatisticText()
{
    QString str("");
    if(circleClipStatistic.total == 0)
        return str;
    str.append(tr("<h2>圆总数: %1<br>").arg(circleClipStatistic.total));
    str.append(tr("在内部: %1<br>").arg(circleClipStatistic.inCount));
    str.append(tr("在外部: %1<br>").arg(circleClipStatistic.outCount));
    str.append(tr("相交: %1<br>").arg(circleClipStatistic.crossCount));
    str.append(tr("交点数: %1<br></h2>").arg(circleClipStatistic.crossPointCount));

    return str;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 设置只显示背景，重绘，以清除绘制的图形。
  */
void PainterArea::clearAllPath()
{
    onlyShowBackGround = true;
    textLabel->setText("");
    update();
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 显示线段和圆的统计报告，并绘制图形。
  */
void PainterArea::showLinesAndCirclesReport()
{
    //处理耗时任务时，防止界面假死。
    qApp->processEvents(QEventLoop::AllEvents);

    onlyShowBackGround = false;
    //统计线段的数据
    calculateLinesStatistic(&lineEntityPoints,&boundaryPoints);
    //统计圆的数据
    calculateCirclesStatistic(&circleEntitys,&boundaryPoints);

#ifdef QT_DEBUG
    qDebug()<<"calculate finished.";
#endif
    //更改按钮显示文字
    switchBtn->setText(tr("裁剪显示"));

    showOrigin = true;
    //显示统计数据
    textLabel->setText(generateLineStatisticText() + generateCircleStatisticText());
#ifdef QT_DEBUG
    qDebug()<<generateLineStatisticText() + generateCircleStatisticText();
#endif
    //绘制图形
    update();
}

void PainterArea::closeWindow()
{
    need2Close = true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 点击 裁剪显示 - 完整显示 后执行的语句
  */
void PainterArea::switchShowMode()
{
    //设置按钮不可用
    switchBtn->setDisabled(true);
    //处理耗时任务时，防止界面假死。
    qApp->processEvents(QEventLoop::AllEvents);
    showOrigin = !showOrigin;
    //计时
    timeval tStart;
    timeval tEnd;

    double usedTime = 0.0;
    //开始计时
    gettimeofday(&tStart,NULL);

    //如果已经执行过裁剪，则不再裁剪。
    if((lineClipStatistic.crossCount > 0) && (clipedLinePoints.size() == 0)){
        calculateWithinLines(&boundaryPoints);
    }
    if((circleClipStatistic.crossCount > 0) && (clipedCircleArcs.size() == 0)){
        calculateWithinCircleArcs(&boundaryPoints);
    }
    //绘制图形
    update();
    //结束计时
    gettimeofday(&tEnd,NULL);
    //用时
    usedTime = (1000000 * (tEnd.tv_sec - tStart.tv_sec) + tEnd.tv_usec - tStart.tv_usec) / 1000.0;

    //设置按钮可用
    switchBtn->setEnabled(true);
    if(!showOrigin){
        switchBtn->setText(tr("完整显示"));
        //备份当前显示
        backupText = textLabel->text();

        textLabel->setText(tr("<h2>裁剪完成!<br>用时: %1 ms<br>占用内存: %2</h2>").arg(usedTime).arg(getUsedMemory(GetCurrentProcessId())));
    }
    else{
        textLabel->setText(backupText);

        switchBtn->setText(tr("裁剪显示"));
    }
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 绘制图形
  */
void PainterArea::paintEvent(QPaintEvent */*event*/)
{
    //如果需要关闭，则关闭，不再绘制
    if(need2Close)
        close();

    QPainter painter(this);
    //抗锯齿
    painter.setRenderHints(QPainter::Antialiasing, true);
    //映射窗口
    //左上角物理坐标(0,0) --> 逻辑坐标(0,0)
    //右下角物理坐标(width,height) --> 逻辑坐标(rect.width,rect.height)
    painter.setWindow(rect);
    //填充黑色
    painter.fillRect(rect,QBrush(Qt::black));
    //设置画笔颜色
    painter.setPen(Qt::white);
    //设置字体
    painter.setFont(QFont("Times",15,QFont::Bold));

    //绘制坐标轴
    int w = rect.width();
    int h = rect.height();

    int left = 100;
    int bottom = 100;

    float deltaX = w / 14.0;
    float deltaY = h / 9.0;

    //X轴
    painter.drawLine(left,h - bottom,w - left + 50,h - bottom);
    for(int i = 2;i < 14;i++){
        painter.drawLine(deltaX * i,h - bottom - 5,deltaX * i,h - bottom + 5);
        painter.drawText(deltaX * i - 15,h - bottom + 30,tr("%1").arg(i * 100));
    }
    //Y轴
    painter.drawLine(left,bottom - 50,left,h - bottom);
    for(int i = 1;i < 9;i++){
        painter.drawLine(left - 5,deltaY * i,left + 5,deltaY * i);
        painter.drawText(left - 45,h - deltaY * i + 10,tr("%1").arg(i * 100));
    }
    //绘制图形
    if(!onlyShowBackGround){
        //绘制边界
        painter.setPen(Qt::red);

        Point *p1 = NULL;
        Point *p2 = NULL;

        int size = 0;
        size = boundaryPoints.size();
        for(int i = 0;i < size - 1;i++){
            p1 = &(boundaryPoints.at(i));
            p2 = &(boundaryPoints.at(i + 1));
            painter.drawLine(p1->x,rect.height() - p1->y,p2->x,rect.height() - p2->y);
        }
        p1 = NULL;
        p2 = NULL;
        Circle *cl = NULL;
        if(showOrigin){
            //绘制所有圆
            painter.setPen(Qt::blue);

            size = lineClipStatistic.total;
            for(int i = 0;i < size;i++){
                p1 = &(lineEntityPoints.at(2 * i));
                p2 = &(lineEntityPoints.at(2 * i + 1));
                painter.drawLine(p1->x,rect.height() - p1->y,p2->x,rect.height() - p2->y);
            }
            //绘制所有线段
            p1 = NULL;
            p2 = NULL;

            size = circleClipStatistic.total;

            for(int i = 0;i < size;i++){
                cl = &(circleEntitys.at(i));
                painter.drawEllipse(getCircleRect(cl));
            }

            cl = NULL;

        }else{
            LineFlag lf;
            CircleFlag cf;
            //绘制在多边形边界内部的线段
            painter.setPen(Qt::blue);
            size = lineClipStatistic.total;
            for(int i = 0;i < size;i++){
                lf = linesFlags.at(i);

                if(lf != WithinLine)
                    continue;

                p1 = &(lineEntityPoints.at(2 * i));
                p2 = &(lineEntityPoints.at(2 * i + 1));
                painter.drawLine(p1->x,rect.height() - p1->y,p2->x,rect.height() - p2->y);
            }
            //绘制裁剪后的线段
            size = clipedLinePoints.size() / 2;
            for(int i = 0;i < size;i++){
                p1 = &(clipedLinePoints.at(2 * i));
                p2 = &(clipedLinePoints.at(2 * i + 1));
                painter.drawLine(p1->x,rect.height() - p1->y,p2->x,rect.height() - p2->y);
            }
            p1 = NULL;
            p2 = NULL;
            //绘制在多边形边界内部的圆
            size = circleClipStatistic.total;
            for(int i = 0;i < size;i++){

                cf = circleFlags.at(i);

                if(cf != WithinCircle)
                    continue;

                cl = &(circleEntitys.at(i));
                painter.drawEllipse(getCircleRect(cl));
            }
            cl = NULL;

            //绘制裁剪后的圆弧
            CirclePoint *cp = NULL;

            int *angles = NULL;

            int total = 0;

            size = clipedCircleArcs.size();

            for(int i = 0;i < size;i++){

                cp = &(clipedCircleArcs.at(i));

                total = cp->points.size() / 2;

                for(int k = 0;k < total;k++){
                    p1 = &(cp->points.at(2 * k));
                    p2 = &(cp->points.at(2 * k + 1));
#ifdef QT_DEBUG
                    qDebug()<<"draw arc:"<<p1->x<<":"<<p1->y<<"-"<<p2->x<<":"<<p2->y;
#endif
                    angles = get2Angles(cp,p1,p2);

                    painter.drawArc(getCircleRect(cp->x,cp->y,cp->r),angles[0],angles[1]);
                    //释放内存
                    delete []angles;
                    angles = NULL;
                }
            }
            cp = NULL;
            p1 = NULL;
            p2 = NULL;
        }
    }
}
