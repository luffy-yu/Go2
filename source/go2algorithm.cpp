#include "go2algorithm.h"

vector<Point> lineEntityPoints;//线段
vector<Point> boundaryPoints;//裁剪窗口(边界)
vector<Circle> circleEntitys;//圆

LineClipStatistic lineClipStatistic;//线段的统计数据
CirCleClipStatistic circleClipStatistic;//圆的统计数据

vector<LineFlag> linesFlags;//标记线段

vector<Point> clipLinePoints;//裁剪线段的点
vector<Point> clipedLinePoints;//裁剪后的线段

vector<CircleFlag> circleFlags;//标记圆

vector<CirclePoint> clipCirclePoints;//裁剪圆的点
vector<CirclePoint> clipedCircleArcs;//裁剪后的圆弧

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 通过内存映射文件的方式打开xml文件。
  * 处理TestData2.xml文件时间 < 2s(注：CPU Celeron(R) Dual-Core T3000 @1.80GHz 1.80GHz)
  * 通过对xml文件的观察。
  * 用'D'定位ID属性，用'B'定位多边形，
  * 用'E'和'L'定位线段，用'E'和'C'定位圆。
  * 遍历文件：
  * 将对多边形顶点存入boundaryPoints；
  * 将线段顶点存入lineEntityPoints；
  * 将圆存入circleEntitys。
  */
bool readXmlFileFiltedByID(QString filename, int filterId)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Unbuffered)){
#ifdef QT_DEBUG
        qDebug()<<"open error";
#endif
        return false;
    }
#ifdef QT_DEBUG
    //用于计时
    QTime time;
    time.start();
#endif
    //建立内存映射
    uchar *fpr = file.map(0,file.size());
    if(!fpr){
#ifdef QT_DEBUG
        qDebug()<<"map error";
#endif
        return false;
    }
    //文件大小
    qint64 fileSize = file.size();
#ifdef QT_DEBUG
    qDebug()<<"file size:"<<fileSize;
#endif
    qint64 offset = 0;//偏移，用于跳过已读取的数据

    qint64 i = 0;//用于遍历文件

    bool passID = false;//判断是不是指定ID的TestCase
    for(;i < fileSize;i++){
        i += offset;
        char ch = char(fpr[i]);
        /**
          * 判断ID
          * 以 ID="1" 为例：
          * 'D'后面的第三个字符之后到第二个'"'之间的字符为ID
          * 所以如果遇到'D'字符，则记录'D'后面最近的一对双引号之间的字符，
          * 然后将其转换为整形，如果与指定ID相等，则读取数据。
          * 否则passID为true，将读取文件下一个字符，直到再次遇到'D'字符，
          * 当passID为false时，才开始读取数据。当再次遇到'D'字符，ID变化，
          * passID再次变为true，不再读取数据。循环直至文件结束。
          */
        if(ch == 'D'){
            passID = false;
            int idi = 2;
            char id[5] = "....";//用于存放表示ID的字符，最大为4位。
            int idPos = 0;
            bool startId = false;
            while(true){
                idi ++;
                if(char(fpr[i + idi]) == '"'){
                    if(startId){
                        break;
                    }
                    startId = true;
                    continue;
                }
                id[idPos++] = char(fpr[i + idi]);
            }
#ifdef QT_DEBUG
            qDebug()<<"id:"<<atoi(id);
#endif
            if(filterId != atoi(id)){//aoti()将char类型转换为int类型
                passID = true;
            }
        }
        if(passID)
            continue;

        offset = 0;//重置为0
        /**
          * 如果当前字符为'B'，并且之后第15个字符为'C'则说明是多边形。
          * B oundary Type="C
          * 如果再次遇到带有'B'字符的是结束标签。
          * 判断字符是不是数字，即 48 <= ch <= 57。
          * 首先是x坐标，遇到','则开始读取y坐标。
          * 如果遇到'/'字符，y坐标读取完毕。
          * 继续读取后面的xy坐标，直到遇到结束标签，"</Boundary>"。
          */
        if(ch == 'B'){
            if(char(fpr[i + 15]) == 'C'){
                int j = 15;
                int sumij = 0;
                int number = 0;
                int number2 = 0;
                char ch4[5] = "....";
                int pos = 0;

                Point *p = NULL;//保存点的指针
                while(true){
                    j++;
                    sumij = i + j;
                    //number >=0 并且 number2 <= 0 才是数字
                    number = fpr[sumij] - 48;//char(48) = '0'，
                    number2 = fpr[sumij] - 57;//char(57) = '9'
                    // '/'后面第11个字符若是'B'，则说明是结束标签，(/ Vertex> </B)
                    // 跳出循环。
                    if(char(fpr[sumij + 11]) == 'B')
                        break;
                    //如果后面一个字符是'/'，则说明当前字符为'<'。
                    //所以y坐标已经读取完毕，使用atoi()转换即可得到y坐标。
                    if(char(fpr[sumij + 1]) == '/'){

                        p->y = atoi(ch4);
                        p->type = LinePoint;
                        //x、y坐标读取完毕，存入boundaryPoints。
                        boundaryPoints.push_back(*p);

                        //释放内存
                        delete p;
                        p = NULL;

                        //准备下次读入，从ch4[0]开始。
                        //因为坐标最少有1位数字，
                        //所以须将ch4的第2、3、4位置上的数字清除，
                        //此处采用'.'，有小数点的意思，在atoi()转换中，
                        //只会将小数点前面的数字转换为整型。
                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';
                        continue;
                    }
                    if(number == -4){//char(44) = ','
                        Point *pp = new Point;
                        pp->x = atoi(ch4);

                        p = pp;

                        //准备读入y坐标，从ch4[0]开始。
                        //因为坐标最少有1位数字，
                        //所以须将ch4的第2、3、4位置上的数字清除，
                        //此处采用'.'，有小数点的意思，在atoi()转换中，
                        //只会将小数点前面的数字转换为整型。
                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';

                        continue;
                    }
                    //如果不是数字就跳过。
                    if(number < 0)
                        continue;
                    if(number2 > 0)
                        continue;

                    ch4[pos++] = char(fpr[sumij]);
                }//Boundary坐标读取完毕。

                //'/'后面第11个字符是'B',
                //再后面第9个字符是'>'即结束标签的结尾尖括号。(B oundary>)
                //所以是 + 11 + 9。
                offset = j + 11 + 9;
            }
            continue;
        }
        /**
          * 如果当前字符为'E'，并且之后第13个字符为'L'则说明是线段。
          * E ntity Type="L
          * 如果再次遇到带有'y'字符的是结束标签。
          * 因为"EndPoint"中带有'E'字符，所以不用'E'判断是否结束。
          * 判断字符是不是数字，即 48 <= ch <= 57。
          * 首先是起点x坐标，遇到','则开始读取y坐标。
          * 如果遇到'/'字符，y坐标读取完毕。
          * 继续读取终点的xy坐标，直到遇到结束标签，"</Entity>"。
          */
        if(ch == 'E'){
            if(char(fpr[i + 13]) == 'L'){

                int j = 13;
                int sumij = 0;
                int number = 0;
                int number2 = 0;

                char ch4[5] = "....";
                int pos = 0;
                Point *p = NULL;//保存点的指针
                while(true){
                    j++;
                    sumij = i + j;
                    number = fpr[sumij] - 48;
                    number2 = fpr[sumij] - 57;
                    //'/'后面第18个字符若是'y',则说明是结束标签，(/ EndPoint> </Entity)
                    if(char(fpr[sumij + 18]) == 'y')
                        break;
                    if(char(fpr[sumij + 1]) == '/'){
                        p->y = atoi(ch4);
                        lineEntityPoints.push_back(*p);

                        delete p;
                        p = NULL;

                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';
                        continue;
                    }
                    if(number == -4){
                        Point *pp = new Point;
                        pp->x = atoi(ch4);
                        p = pp;

                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';
                        continue;
                    }
                    if(number < 0)
                        continue;
                    if(number2 > 0)
                        continue;
                    ch4[pos++] = char(fpr[sumij]);
                }//Line坐标读取完毕。
                //'/'后面第18个字符是'y',
                //再后面第2个字符是'>'即结束标签的结尾尖括号。(</Entit y>)
                //所以是 + 18 + 2。
                offset = j + 18 + 2;

                continue;
            }
            /**
              * 如果当前字符为'E'，并且之后第13个字符为'C'则说明是圆。
              * E ntity Type="C
              * 如果再次遇到带有'y'字符的是结束标签。
              * 判断字符是不是数字，即 48 <= ch <= 57。
              * 首先是x坐标，遇到','则开始读取y坐标。
              * 如果遇到'/'字符，y坐标读取完毕。
              * 继续读取半径，直到遇到结束标签，"</Entity>"。
              */
            if(char(fpr[i + 13] == 'C')){

                int j = 13;
                int sumij = 0;
                int number = 0;
                int number2 = 0;
                char ch4[5] = "....";
                int pos = 0;

                Circle *cl = NULL;//保存圆的指针

                bool startRadius = false;

                while(true){
                    j++;
                    sumij = i + j;

                    number = fpr[sumij] - 48;
                    number2 = fpr[sumij] - 57;
                    //'/'后面第16个字符若是'y',则说明是结束标签，(/ Radius> </Entity)
                    if(char(fpr[sumij + 16]) == 'y')
                        break;
                    if(char(fpr[sumij + 1]) == '/'){
                        //Y
                        if(startRadius){
                            cl->r = atoi(ch4);
                            circleEntitys.push_back(*cl);

                            delete cl;
                            cl = NULL;

                            startRadius = false;

                            continue;
                        }

                        cl->y = atoi(ch4);

                        startRadius = true;

                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';

                        continue;
                    }

                    if(number == -4){
                        //startY = true;
                        //X

                        Circle *cc = new Circle;
                        cc->x = atoi(ch4);

                        cl = cc;

                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';

                        continue;
                    }

                    if(number < 0)
                        continue;
                    if(number2 > 0)
                        continue;
                    ch4[pos++] = char(fpr[sumij]);
                }//Circle读取完毕。
                //'/'后面第16个字符是'y',
                //再后面第2个字符是'>'即结束标签的结尾尖括号。(</Entit y>)
                //所以是 + 16 + 2。
                offset = j + 16 + 2;

                continue;
            }
            continue;
        }
    }
    //移除内存映射
    file.unmap(fpr);
    //关闭文件
    file.close();
#ifdef QT_DEBUG
    //耗时
    qDebug()<<"time:"<<time.elapsed() << " ms";

    qDebug()<<"Circles:"<< circleEntitys.size();
    qDebug()<<"Lines:"<< lineEntityPoints.size() / 2;

    qDebug()<<"Boundarys vertex:"<<boundaryPoints.size();
#endif
    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 通过内存映射文件的方式打开xml文件。
  * 处理TestData2.xml文件时间 < 2s(注：CPU Celeron(R) Dual-Core T3000 @1.80GHz 1.80GHz)
  * 通过对xml文件的观察。
  * 用'D'定位ID属性，用'B'定位多边形，
  * 用'E'和'L'定位线段，用'E'和'C'定位圆。
  * 遍历文件：
  * 将对多边形顶点存入boundaryPoints；
  * 将线段顶点存入lineEntityPoints；
  * 将圆存入circleEntitys。
  */
bool readXmlFileAll(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Unbuffered)){
#ifdef QT_DEBUG
        qDebug()<<"open error";
#endif
        return false;
    }
#ifdef QT_DEBUG
    //用于计时
    QTime time;
    time.start();
#endif
    //建立内存映射
    uchar *fpr = file.map(0,file.size());
    if(!fpr){
#ifdef QT_DEBUG
        qDebug()<<"map error";
#endif
        return false;
    }
    //文件大小
    qint64 fileSize = file.size();
#ifdef QT_DEBUG
    qDebug()<<"file size:"<<fileSize;
#endif
    qint64 offset = 0;//偏移，用于跳过已读取的数据

    qint64 i = 0;//用于遍历文件

    for(;i < fileSize;i++){
        i += offset;
        char ch = char(fpr[i]);

        offset = 0;//重置为0
        /**
          * 如果当前字符为'B'，并且之后第15个字符为'C'则说明是多边形。
          * B oundary Type="C
          * 如果再次遇到带有'B'字符的是结束标签。
          * 判断字符是不是数字，即 48 <= ch <= 57。
          * 首先是x坐标，遇到','则开始读取y坐标。
          * 如果遇到'/'字符，y坐标读取完毕。
          * 继续读取后面的xy坐标，直到遇到结束标签，"</Boundary>"。
          */
        if(ch == 'B'){
            if(char(fpr[i + 15]) == 'C'){
                int j = 15;
                int sumij = 0;
                int number = 0;
                int number2 = 0;
                char ch4[5] = "....";
                int pos = 0;

                Point *p = NULL;//保存点的指针
                while(true){
                    j++;
                    sumij = i + j;
                    //number >=0 并且 number2 <= 0 才是数字
                    number = fpr[sumij] - 48;//char(48) = '0'，
                    number2 = fpr[sumij] - 57;//char(57) = '9'
                    // '/'后面第11个字符若是'B'，则说明是结束标签，(/ Vertex> </B)
                    // 跳出循环。
                    if(char(fpr[sumij + 11]) == 'B')
                        break;
                    //如果后面一个字符是'/'，则说明当前字符为'<'。
                    //所以y坐标已经读取完毕，使用atoi()转换即可得到y坐标。
                    if(char(fpr[sumij + 1]) == '/'){

                        p->y = atoi(ch4);
                        p->type = LinePoint;
                        //x、y坐标读取完毕，存入boundaryPoints。
                        boundaryPoints.push_back(*p);

                        //释放内存
                        delete p;
                        p = NULL;

                        //准备下次读入，从ch4[0]开始。
                        //因为坐标最少有1位数字，
                        //所以须将ch4的第2、3、4位置上的数字清除，
                        //此处采用'.'，有小数点的意思，在atoi()转换中，
                        //只会将小数点前面的数字转换为整型。
                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';
                        continue;
                    }
                    if(number == -4){//char(44) = ','
                        Point *pp = new Point;
                        pp->x = atoi(ch4);

                        p = pp;

                        //准备读入y坐标，从ch4[0]开始。
                        //因为坐标最少有1位数字，
                        //所以须将ch4的第2、3、4位置上的数字清除，
                        //此处采用'.'，有小数点的意思，在atoi()转换中，
                        //只会将小数点前面的数字转换为整型。
                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';

                        continue;
                    }
                    //如果不是数字就跳过。
                    if(number < 0)
                        continue;
                    if(number2 > 0)
                        continue;

                    ch4[pos++] = char(fpr[sumij]);
                }//Boundary坐标读取完毕。

                //'/'后面第11个字符是'B',
                //再后面第9个字符是'>'即结束标签的结尾尖括号。(B oundary>)
                //所以是 + 11 + 9。
                offset = j + 11 + 9;
            }
            continue;
        }
        /**
          * 如果当前字符为'E'，并且之后第13个字符为'L'则说明是线段。
          * E ntity Type="L
          * 如果再次遇到带有'y'字符的是结束标签。
          * 因为"EndPoint"中带有'E'字符，所以不用'E'判断是否结束。
          * 判断字符是不是数字，即 48 <= ch <= 57。
          * 首先是起点x坐标，遇到','则开始读取y坐标。
          * 如果遇到'/'字符，y坐标读取完毕。
          * 继续读取终点的xy坐标，直到遇到结束标签，"</Entity>"。
          */
        if(ch == 'E'){
            if(char(fpr[i + 13]) == 'L'){

                int j = 13;
                int sumij = 0;
                int number = 0;
                int number2 = 0;

                char ch4[5] = "....";
                int pos = 0;
                Point *p = NULL;//保存点的指针
                while(true){
                    j++;
                    sumij = i + j;
                    number = fpr[sumij] - 48;
                    number2 = fpr[sumij] - 57;
                    //'/'后面第18个字符若是'y',则说明是结束标签，(/ EndPoint> </Entity)
                    if(char(fpr[sumij + 18]) == 'y')
                        break;
                    if(char(fpr[sumij + 1]) == '/'){
                        p->y = atoi(ch4);
                        lineEntityPoints.push_back(*p);

                        delete p;
                        p = NULL;

                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';
                        continue;
                    }
                    if(number == -4){
                        Point *pp = new Point;
                        pp->x = atoi(ch4);
                        p = pp;

                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';
                        continue;
                    }
                    if(number < 0)
                        continue;
                    if(number2 > 0)
                        continue;
                    ch4[pos++] = char(fpr[sumij]);
                }//Line坐标读取完毕。
                //'/'后面第18个字符是'y',
                //再后面第2个字符是'>'即结束标签的结尾尖括号。(</Entit y>)
                //所以是 + 18 + 2。
                offset = j + 18 + 2;

                continue;
            }
            /**
              * 如果当前字符为'E'，并且之后第13个字符为'C'则说明是圆。
              * E ntity Type="C
              * 如果再次遇到带有'y'字符的是结束标签。
              * 判断字符是不是数字，即 48 <= ch <= 57。
              * 首先是x坐标，遇到','则开始读取y坐标。
              * 如果遇到'/'字符，y坐标读取完毕。
              * 继续读取半径，直到遇到结束标签，"</Entity>"。
              */
            if(char(fpr[i + 13] == 'C')){

                int j = 13;
                int sumij = 0;
                int number = 0;
                int number2 = 0;
                char ch4[5] = "....";
                int pos = 0;

                Circle *cl = NULL;//保存圆的指针

                bool startRadius = false;

                while(true){
                    j++;
                    sumij = i + j;

                    number = fpr[sumij] - 48;
                    number2 = fpr[sumij] - 57;
                    //'/'后面第16个字符若是'y',则说明是结束标签，(/ Radius> </Entity)
                    if(char(fpr[sumij + 16]) == 'y')
                        break;
                    if(char(fpr[sumij + 1]) == '/'){
                        //Y
                        if(startRadius){
                            cl->r = atoi(ch4);
                            circleEntitys.push_back(*cl);

                            delete cl;
                            cl = NULL;

                            startRadius = false;

                            continue;
                        }

                        cl->y = atoi(ch4);

                        startRadius = true;

                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';

                        continue;
                    }

                    if(number == -4){
                        //startY = true;
                        //X

                        Circle *cc = new Circle;
                        cc->x = atoi(ch4);

                        cl = cc;

                        pos = 0;
                        ch4[1] = '.';
                        ch4[2] = '.';
                        ch4[3] = '.';

                        continue;
                    }

                    if(number < 0)
                        continue;
                    if(number2 > 0)
                        continue;
                    ch4[pos++] = char(fpr[sumij]);
                }//Circle读取完毕。
                //'/'后面第16个字符是'y',
                //再后面第2个字符是'>'即结束标签的结尾尖括号。(</Entit y>)
                //所以是 + 16 + 2。
                offset = j + 16 + 2;

                continue;
            }
            continue;
        }
    }
    //移除内存映射
    file.unmap(fpr);
    //关闭文件
    file.close();
#ifdef QT_DEBUG
    //耗时
    qDebug()<<"time:"<<time.elapsed() << " ms";

    qDebug()<<"Circles:"<< circleEntitys.size();
    qDebug()<<"Lines:"<< lineEntityPoints.size() / 2;

    qDebug()<<"Boundarys vertex:"<<boundaryPoints.size();
#endif
    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 将线段和圆的统计数据置为0，
  * 并用swap的方法释放所有vector的空间。
  */
bool clearFormerState()
{
    lineClipStatistic.total = 0;
    lineClipStatistic.crossCount = 0;
    lineClipStatistic.crossPointCount = 0;
    lineClipStatistic.inCount = 0;
    lineClipStatistic.outCount = 0;

    circleClipStatistic.total = 0;
    circleClipStatistic.crossCount = 0;
    circleClipStatistic.crossPointCount = 0;
    circleClipStatistic.inCount = 0;
    circleClipStatistic.outCount = 0;

    {
        vector<Point> temp;
        temp.swap(lineEntityPoints);
    }

    {
        vector<Point> temp;
        temp.swap(boundaryPoints);
    }

    {
        vector<Circle> temp;
        temp.swap(circleEntitys);
    }

    {
        vector<LineFlag> temp;
        temp.swap(linesFlags);
    }
    {
        vector<Point> temp;
        temp.swap(clipLinePoints);
    }
    {
        vector<Point> temp;
        temp.swap(clipedLinePoints);
    }
    {
        vector<CircleFlag> temp;
        temp.swap(circleFlags);
    }
    {
        vector<CirclePoint> temp;
        temp.swap(clipCirclePoints);
    }
    {
        vector<CirclePoint> temp;
        temp.swap(clipedCircleArcs);
    }
    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 用直线求交点的方法计算线段与边界的所有交点
  */
vector<Point> calculateLineCrossPoints(Point *start, Point *end, vector<Point> *bounds)
{
    float dx1 = end->x - start->x;
    float dy1 = end->y - start->y;

    float dx2 = 0;
    float dy2 = 0;

    float y1 = start->y;
    float x1 = start->x;
    float x3 = 0;
    float y3 = 0;

    float xmin = 0;
    float xmax = 0;
    float ymin = 0;
    float ymax = 0;

    vector<Point> result;

    Point first = clonePoint(start);
    Point second = clonePoint(end);
    //将线段端点存入结果中
    result.push_back(first);
    result.push_back(second);

    int total = bounds->size();

    Point *p1 = NULL;
    Point *p2 = NULL;

    for(int i = 0;i < total - 1;i++){
        p1 = &(bounds->at(i));
        p2 = &(bounds->at(i + 1));

        dx2 = p2->x - p1->x;
        dy2 = p2->y - p1->y;

        //如果平行，则计算线段与边界下一条边的交点。
        if(dx1 * dy2 - dx2 * dy1 == 0){
            continue;
        }

        x3 = p1->x;
        y3 = p1->y;

        Point p = createPoint(0,0);

        p.x = (dx1 * dx2 * (y1 - y3) + dx1 * dy2 * x3 - dx2 * dy1 * x1) / (dx1 * dy2 - dx2 * dy1);

        if(dx1 != 0){
            p.y = (dy1 * (p.x - x1)) / dx1 + y1;
        }else{
            p.y = dy2 / dx2 * (p.x - p1->x) + p1->y;
        }
        xmin = p1->x < p2->x ? p1->x : p2->x;
        xmax = p1->x > p2->x ? p1->x : p2->x;

        ymin = p1->y < p2->y ? p1->y : p2->y;
        ymax = p1->y > p2->y ? p1->y : p2->y;

        //判断交点是不是在线段上
        if((p.x < xmin)||(p.x > xmax) || (p.y < ymin) || (p.y > ymax))
            continue;

        //判断交点是不是顶点
        int index = calculateVertexIndex(p.x,p.y,bounds);

        if(index == -1){//交点不是顶点
            p.type = CrossLinePoint;
        }else{//交点是顶点
            Point *vertex = NULL;
            Point *vertexL = NULL;
            Point *vertexR = NULL;
            Point *linePoint = &second;

            if(index == 0){
                vertex = &(bounds->at(0));
                vertexL = &(bounds->at(total - 2));
                vertexR = &(bounds->at(1));
            }else{
                vertex = &(bounds->at(index));
                vertexL = &(bounds->at(index - 1));
                vertexR = &(bounds->at(index + 1));
            }
            //判断是不是穿过顶点
            p.type = linePassByVertexPointType(vertex,vertexR,vertexL,false,start,end);

            vertex = NULL;
            vertexL = NULL;
            vertexR = NULL;
            linePoint = NULL;
        }
        result.push_back(p);
    }
    p1 = NULL;
    p2 = NULL;

#ifdef QT_DEBUG
    qDebug()<<"calculateLineCrossPoints result size:"<<result.size();
#endif
    return result;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 判断点是不是在边界内部。
  * 方法：过该点做Y轴的平行线，
  * 即 X = centerx，与边界求交点，
  * 利用"交点检验法"判断该点是不是在边界内部。
  */
bool pointWithinBounds(float centerx, float centery, vector<Point> *bounds)
{
    //float dy1 = centery;

    float dx2 = 0;
    float dy2 = 0;

    float x1 = centerx;
    float x3 = 0;
    float y3 = 0;

    float xmin = 0;
    float xmax = 0;
    float ymin = 0;
    float ymax = 0;

    vector<Point> result;

    Point center = createPoint(centerx,centery);

    result.push_back(center);

    int total = bounds->size();

    Point *p1 = NULL;
    Point *p2 = NULL;

    for(int i = 0;i < total - 1;i++){
        p1 = &(bounds->at(i));
        p2 = &(bounds->at(i + 1));

        dx2 = p2->x - p1->x;
        dy2 = p2->y - p1->y;

        if(dx2 == 0){//平行
            continue;
        }

        x3 = p1->x;
        y3 = p1->y;

        Point p = createPoint(0,0);

        p.x = x1;

        p.y = dy2 / dx2 * (p.x - p1->x) + p1->y;

        xmin = p1->x < p2->x ? p1->x : p2->x;
        xmax = p1->x > p2->x ? p1->x : p2->x;

        ymin = p1->y < p2->y ? p1->y : p2->y;
        ymax = p1->y > p2->y ? p1->y : p2->y;

        if((p.x < xmin)||(p.x > xmax) || (p.y < ymin) || (p.y > ymax))
            continue;

        p.type = CrossLinePoint;

        result.push_back(p);
    }

    p1 = NULL;
    p2 = NULL;

    vector<Point> sorted = getSortedUniquePoints(&result,0,result.size() - 1);
    //起点(小值)
    p1 = &(sorted.at(0));
    //终点(大值)
    p2 = &(sorted.at(sorted.size() - 1));

    Point *p = NULL;
    Point *vertex = NULL;
    Point *vertexL = NULL;
    Point *vertexR = NULL;

    for(int i = 0;i < sorted.size();i++){
        p = &(sorted.at(i));
        int index = calculateVertexIndex(p->x,p->y,bounds);
        if(index == -1){
            continue;
        }
        if(index == 0){
            vertex = &(bounds->at(0));
            vertexL = &(bounds->at(total - 2));
            vertexR = &(bounds->at(1));
        }else{
            vertex = &(bounds->at(index));
            vertexL = &(bounds->at(index - 1));
            vertexR = &(bounds->at(index + 1));
        }
        p->type = linePassByVertexPointType(vertex,vertexR,vertexL,true,p1,p2);
    }

    p1 = NULL;
    p2 = NULL;
    p = NULL;

    vertex = NULL;
    vertexL = NULL;
    vertexR = NULL;

    {
        vector<Point> temp;
        temp.swap(result);
    }

    return pointWithinBoundsByCrosses(sorted,centerx,centery);
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 交点检验法的实现
  */
bool pointWithinBoundsByCrosses(vector<Point> sorted, float centerx, float centery)
{
    Point *p = NULL;

    int total = sorted.size();

    int leftCount = 0;
    int rightCount = 0;

    int centerIndex = 0;

    for(int i = 0;i < total;i++){
        p = &(sorted.at(i));
        if((p->x == centerx) && (p->y == centery)){
            centerIndex = i;
            break;
        }
    }

    for(int i = 0;i < centerIndex;i++){
        p = &(sorted.at(i));
        if(p->type == PassByVertexPoint)
            continue;
        leftCount ++;
    }

    for(int i = centerIndex + 1;i < total;i++){
        p = &(sorted.at(i));
        if(p->type == PassByVertexPoint)
            continue;
        rightCount++;
    }

#ifdef QT_DEBUG
    qDebug()<<"left:"<<leftCount<<"right:"<<rightCount;
#endif

    //释放内存
    {
        vector<Point> temp;
        temp.swap(sorted);
    }

    p = NULL;

    if((leftCount % 2 == 1) && (rightCount % 2 == 1)){
        return true;
    }else{
        return false;
    }
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 通过线段与多边形的交点，
  * 判断现在是在多边形内部、外部、还是相交。
  * 同时计算交点数量。
  * 另外根据排序后的唯一交点，
  * 将线段划分为更小的线段，存入clipLinePoints，
  * 当执行裁剪显示时，只需要判断clipLinePoints中的
  * 线段是不是在多边形内部即可。
  * 为了区分小线段的隶属关系，在最后一个交点之后插入
  * 一个NoSensePoint(分割点)。当传入新线段的交点时，这些交点
  * 是属于当前线段的。NoSensePoint是为了防止上一条线段的
  * 最后一个交点与下一条线段的第一个交点，组成线段L，然后
  * 判断L是不是在多边形内部。这就导致一个错误，因为L线段
  * 不属于任何一条线段。
  */
bool calculateLineStatistic(vector<Point> *crosses, vector<Point> *bounds)
{
    Point *p = NULL;
    p = &(crosses->at(0));

    float x1 = p->x;
    float y1 = p->y;

    p = &(crosses->at(1));

    float x2 = p->x;
    float y2 = p->y;

    vector<Point> valids = getValidLineCrossPoints(crosses);

    lineClipStatistic.crossPointCount += valids.size();

    vector<Point> sorted = getSortedUniquePoints(crosses,0,crosses->size() - 1);

#ifdef QT_DEBUG
    qDebug()<<"sort before:"<<crosses->size()<<"after:"<<sorted.size();
#endif
    int middleCount = 0;

    int startIndex = 0;
    int endIndex = 0;

    int total = sorted.size();

    for(int i = 0;i < total;i++){
        p = &(sorted.at(i));

        if((p->x == x1) && (p->y == y1)){
            startIndex = i;
            continue;
        }
        if((p->x == x2) && (p->y == y2)){
            endIndex = i;
            continue;
        }
    }

    int minIndex = startIndex < endIndex ? startIndex : endIndex;
    int maxIndex = startIndex + endIndex - minIndex;

#ifdef QT_DEBUG
    qDebug()<<"minIndex"<<minIndex<<"maxIndex:"<<maxIndex;
#endif
    for(int i = minIndex + 1;i < maxIndex;i++){
        p = &(sorted.at(i));
        if(p->type == PassByVertexPoint)
            continue;
        middleCount ++;
    }
    //若线段两端点之间没有穿过边的交点，则说明该线段不与多边形相交。
    //用交点检验法判断线段在多边形内部还是外部。
    //思路：在线段两端点间取一个非多边形边界顶点的点，判断该点是否在多边形内部。
    //如果在内部，则说明线段在多边形内部，否则说明线段在多边形外部。
    if(middleCount == 0){
        float *xy = new float[2];
        xy[0] = 0.0;
        xy[1] = 0.0;
        for(int i = 1;i < 10;i++){
            // x2 = x1 + (x2 - x1) * t,0 <= t <= 1;
            // y2 = y1 + (y2 - y1) * t,0 <= t <= 1;
            xy[0] = x1 + (x2 - x1) * i / 10.0;
            xy[1] = y1 + (y2 - y1) * i / 10.0;
            if(calculateVertexIndex(xy[0],xy[1],bounds) == -1)
                break;
        }
        if(pointWithinBounds(xy[0],xy[1],bounds)){
            lineClipStatistic.inCount ++;
            linesFlags.push_back(WithinLine);
        }else{
            lineClipStatistic.outCount ++;
            linesFlags.push_back(EctadLine);
        }

        delete []xy;
        xy = NULL;

    }else{
        lineClipStatistic.crossCount ++;

        linesFlags.push_back(CrossLine);
        //将交点依次保存到clipLinePoints
        Point *p1;
        for(int i = minIndex;i <= maxIndex; i++){
            p1 = &(sorted.at(i));
            clipLinePoints.push_back(clonePoint(p1));
        }
        clipLinePoints.push_back(createNoSensePoint());

        p1 = NULL;
    }
    p = NULL;
    //释放内存
    {
        vector<Point> temp;
        temp.swap(valids);
    }

    {
        vector<Point> temp;
        temp.swap(sorted);
    }

    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 通过调用calculateLineStatistic判断所有线段
  * 是在多边形内部、外部还是相交，并且计算交点数量，
  * 同时将分割后的线段存入clipLinePoints。
  */
bool calculateLinesStatistic(vector<Point> *lines, vector<Point> *bounds)
{
    //释放内存
    {
        vector<Point> temp;
        temp.swap(clipLinePoints);
    }
    //线段总数
    int lineCount = lines->size() / 2;
    lineClipStatistic.total = lineCount;

    Point *start = NULL;
    Point *end = NULL;
    for(int i = 0;i < lineCount;i++){
        //起点
        start = &(lines->at(2 * i));
        //终点
        end = &(lines->at(2 * i + 1));
#ifdef QT_DEBUG
        qDebug()<<start->x<<start->y<<"-"<<end->x<<end->y;
#endif
        vector<Point> crosses = calculateLineCrossPoints(start,end,bounds);
        calculateLineStatistic(&crosses,bounds);
        //释放内存
        {
            vector<Point> temp;
            temp.swap(crosses);
        }
    }
    //释放内存
    {
        vector<Point> temp;
        temp.swap(clipedLinePoints);
    }

    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算点是不是多边形的顶点，
  * 如果是返回索引值，否者返回-1。
  */
int calculateVertexIndex(float x, float y, vector<Point> *bounds)
{
    int index = -1;
    int total = bounds->size();
    Point *pp = NULL;
    //多边形的最后一个点同第一个点
    for(int i = 0;i < total - 1;i++){
        pp = &(bounds->at(i));
        if((pp->x == x) && (pp->y == y)){
            index = i;
            break;
        }
    }

    pp = NULL;

    return index;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算交点类型，即是否穿过交点。
  *
  *     A
  *    /
  *   /
  *  /
  * O ------C
  *  \
  *   \
  *    D-------E
  *     \
  *      B
  *
  * 如上简图。
  * A、O、B为多边形的顶点，
  * O也是线段与多边形的交点。
  * 如果判断的是交点并且O是线段起点或者终点，则不穿过顶点。
  * 如果判断是中点(线段上的点、圆弧中点、圆心等)或者O不是线段中点或者起点，
  * 则做如下判断：
  * 假设，C是线段上的另一点，
  * 过OB边中点D做OC的平行线DE，
  * 计算DE与OA边的交点P，
  * 如果P在AO方向的延长线上，
  * 即向量AO与向量OP同向，OC穿过点O。
  * 否则OC不穿过O。
  */
PointType linePassByVertexPointType(Point *vertex, Point *vertexR, Point *vertexL, bool centerPoint, Point *start, Point *end)
{
    //如果判断的是交点，并且待判断点是线段的起点或者终点，
    //那么一定是经过顶点，即PassByVertexPoint
    if((!centerPoint) && (compare2Points(vertex,start) || compare2Points(vertex,end))){
        return PassByVertexPoint;
    }
    //如果是中点、或者是不是起点或者终点的交点
    Point *PA = vertexR;
    Point *PB = vertexL;
    Point *PO = vertex;
    Point *PC = start;

    //如果O、PC重合，则PC点取线段终点
    if(compare2Points(PO,PC)){
        PC->x = end->x;
        PC->y = end->y;
        //PC->type = end->type;
    }

    //DE // OC
    //Ax + By + C = 0;
    //Line DE
    float A = PC->y - PO->y;
    float B = PO->x - PC->x;
    float C = (-1) * (A * (PO->x + PB->x) / 2.0 + B * (PO->y + PB->y) / 2.0);
    //Line OA
    float A1 = PA->y - PO->y;
    float B1 = PO->x - PA->x;
    float C1 = (PA->x - PO->x) * PO->y - PO->x * (PA->y - PO->y);

    //计算交点
    float x = (B * C1 - B1 * C) / (A * B1 - A1 * B);
    float y = (A * C1 - A1 * C) / (A1 * B - A * B1);

    //计算向量AO与向量OP的方向。
    float vectorAOX = PO->x - PA->x;
    float vectorAOY = PO->y - PA->y;
    float vectorOPX = x - PO->x;
    float vectorOPY = y - PO->y;

    float value = vectorAOX * vectorOPX + vectorAOY * vectorOPY;

    if(value > 0){
        return CrossVertexPoint;
    }else{
        return PassByVertexPoint;
    }
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算在线段上去除重复后的排序后的交点
  */
vector<Point> getValidLineCrossPoints(vector<Point> *crosses)
{
    //前两个为直线端点
    Point *p = &(crosses->at(0));
    float x1 = p->x;
    float y1 = p->y;

    p = &(crosses->at(1));
    float x2 = p->x;
    float y2 = p->y;

    float xmin = x1 < x2 ? x1 : x2;
    float xmax = x1 + x2 - xmin;

    float ymin = y1 < y2 ? y1 : y2;
    float ymax = y1 + y2 - ymin;

    vector<Point> sorted = getSortedUniquePoints(crosses,2,crosses->size() - 1);

    vector<Point> result;

    int total = sorted.size();

    for(int i = 0;i < total;i++){
        p = &(sorted.at(i));
        if(p->x < xmin || p->y < ymin || p->x > xmax || p->y > ymax)
            continue;

        result.push_back(clonePoint(p));
    }

    return result;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 按照x升序y升序的方式对
  * [startIndex,endIndex]的点进行排序，
  * 并且去除重复的点。
  */
vector<Point> getSortedUniquePoints(vector<Point> *crosses, int startIndex, int endIndex)
{
    vector<Point> result;

    if(crosses->size() == 0)
        return result;

    if(startIndex > endIndex){
        return result;
    }
    Point *p1 = NULL;
    if(startIndex == endIndex){
        p1 = &(crosses->at(startIndex));

        Point p = clonePoint(p1);

        result.push_back(p);

        return result;
    }
    Point *p2 = NULL;
    float temp = 0.0;
    PointType tempi = LinePoint;
    for(int i = startIndex;i <= endIndex;i++){
        for(int j = i + 1;j <= endIndex;j++){

            p1 = &(crosses->at(i));
            p2 = &(crosses->at(j));

            if(p1->x > p2->x){
                temp = p1->x;
                p1->x = p2->x;
                p2->x = temp;

                temp = p1->y;
                p1->y = p2->y;
                p2->y = temp;

                tempi = p1->type;
                p1->type = p2->type;
                p2->type = tempi;

            }else if(p1->x == p2->x){
                if(p1->y > p2->y){

                    temp = p1->y;
                    p1->y = p2->y;
                    p2->y = temp;

                    tempi = p1->type;
                    p1->type = p2->type;
                    p2->type = tempi;
                }
            }
        }
    }
    Point *last = NULL;
    for(int i = startIndex;i <= endIndex;i++){
        p1 = &(crosses->at(i));
        if(i == startIndex){
            Point pp = clonePoint(p1);

            result.push_back(pp);

            last = &pp;
            continue;
        }
        if((last->x == p1->x) && (last->y == p1->y)){
            continue;
        }
        Point pp = clonePoint(p1);

        result.push_back(pp);

        last = &pp;
    }

    p1 = NULL;
    p2 = NULL;
    last = NULL;

    return result;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算相交线段裁剪后的线段
  */
bool calculateWithinLines(vector<Point> *bounds)
{
    //清除clipedLinePoints里的元素
    //并且释放内存，用于存放裁剪后的线段。
    {
        vector<Point> temp;
        temp.swap(clipedLinePoints);
    }

    Point *p1 = NULL;
    Point *p2 = NULL;
    int total = clipLinePoints.size();
    for(int i = 0;i < total - 1; i++){
        p1 = &(clipLinePoints.at(i));
        p2 = &(clipLinePoints.at(i + 1));
        //如果是分割点，则进行下次循环。
        if(p2->type == NoSensePoint || p1->type == NoSensePoint){
            continue;
        }

        if(pointWithinBounds((p1->x + p2->x)/2.0,(p1->y + p2->y)/2.0,bounds)){

            clipedLinePoints.push_back(clonePoint(p1));
            clipedLinePoints.push_back(clonePoint(p2));
        }
    }
    //释放内存
    {
        vector<Point> temp;
        temp.swap(clipLinePoints);
    }

    p1 = NULL;
    p2 = NULL;

    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 克隆点
  */
Point clonePoint(Point *p)
{
    Point pp;
    pp.type = p->type;
    pp.x = p->x;
    pp.y = p->y;

    return pp;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 创建NoSensePoint(分割点)
  */
Point createNoSensePoint()
{
    Point p;
    p.x = 0;
    p.y = 0;
    p.type = NoSensePoint;

    return p;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 创建(x,y)点
  */
Point createPoint(float x, float y)
{
    Point p;
    p.x = x;
    p.y = y;
    p.type = LinePoint;

    return p;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算圆与多边形的交点
  * 思路：首先计算直线方程，
  * 然后计算圆心到直线的距离d，
  * 如果d大于半径，则无交点。
  * 对于圆包含多边形的情况：
  * 首先假设圆包含多边形，
  * 当 d > R 时，假设不成立，
  * 当交点落在边上(不包含端点时)，假设亦不成立。
  * 如果交点是多边形的顶点，则判断圆是不是穿过该顶点。
  * 交点数据保存在outCrosses中。
  * 由于多边形的点是顺时针存取的，
  * 输出的点也是按照顺时针的顺序。
  */
bool calculateCircleCrossPoints(Circle *cl, vector<Point> *bounds,vector<Point> *outCrosses)
{
    Point *p1 = NULL;
    Point *p2 = NULL;
    //直线方程
    //ax + by + c = 0;

    float a = 0.0;
    float b = 0.0;
    float c = 0.0;

    float centerX = cl->x;
    float centerY = cl->y;
    float radius = cl->r;

    //直线到圆的距离
    float d = 0.0;
    //判别式
    float delta = 0.0;

    float A = 0.0;
    float B = 0.0;
    float C = 0.0;

    int total = bounds->size();
    float x1 = 0.0;
    float y1 = 0.0;
    float x2 = 0.0;
    float y2 = 0.0;

    float t1 = 0.0;
    float t2 = 0.0;
    float maxt = 0.0;

    bool circleContainingBounds = true;

    for(int i = 0;i < total - 1;i++){
        p1 = &(bounds->at(i));

        p2 = &(bounds->at(i+1));

        x1 = p1->x;
        y1 = p1->y;

        x2 = p2->x;
        y2 = p2->y;

        a = y2 - y1;
        b = x1 - x2;
        c = y1 * (x2 - x1) - x1 * (y2 - y1);

        d = fabs(a * centerX + b * centerY + c) / sqrt(a * a + b * b);
        if(d > radius){
            circleContainingBounds = false;

            continue;
        }

        A = x1 * x1 + x2 * x2 + y1 * y1 + y2 * y2 - 2 * x1 * x2 - 2 * y1 * y2;
        B = 2 * (centerX * x1 + x1 * x2 + centerY * y1 + y1 * y2 - centerX * x2 - centerY * y2 - x1 * x1 - y1 * y1);
        C = centerX * centerX + x1 * x1 + centerY * centerY + y1 * y1 - 2 * centerX * x1 - 2 * centerY * y1 - radius * radius;

        delta = B * B - 4 * A * C;

        if(delta < 0)
            continue;

        if(delta == 0){
            maxt =((-1) * B - sqrt(delta)) / (2 * A);
            if((0 <= maxt) && (maxt <= 1.0)){
                //默认点是经过交点的圆弧是穿过边的。
                Point p = createPoint(0,0);
                p.type = CrossLinePoint;
                //只有一个交点，而且交点在边上(不包括端点)，
                //那么该交点一定是切点。即PassByLinePoint。
                p.x = x1 + (x2 - x1) * maxt;
                p.y = y1 + (y2 - y1) * maxt;
                p.type = PassByLinePoint;

                if((maxt == 0) || (maxt == 1.0)){
                    //交点是顶点，则判断是不是穿过顶点
                    p.type = circlePassByVertexPointType(&p,bounds,cl);
                }else{
                    //交点在边上（不包括端点），圆一定不会包括多边形。
                    circleContainingBounds = false;
                }
                outCrosses->push_back(p);
            }
        }else{
            t1 = ((-1) * B - sqrt(delta)) / (2 * A);
            t2 = ((-1) * B + sqrt(delta)) / (2 * A);

            if(t1 > t2){
                maxt = t1;
                t1 = t2;
                t2 = maxt;
            }
            if((0 <= t1) && (t1 <= 1.0)){

                Point p = createPoint(0,0);
                p.type = CrossLinePoint;

                p.x = x1 + (x2 - x1) * t1;
                p.y = y1 + (y2 - y1) * t1;

                if((t1 == 0) || (t1 == 1.0)){
                    p.type = circlePassByVertexPointType(&p,bounds,cl);;
                }else{
                    circleContainingBounds = false;
                }

                outCrosses->push_back(p);
            }
            if((0 <= t2) && (t2 <= 1.0)){

                Point p = createPoint(0,0);
                p.type = CrossLinePoint;

                p.x = x1 + (x2 - x1) * t2;
                p.y = y1 + (y2 - y1) * t2;

                if((t2 == 0) || (t2 == 1.0)){
                    p.type = circlePassByVertexPointType(&p,bounds,cl);
                }else{
                    circleContainingBounds = false;
                }
                outCrosses->push_back(p);
            }
        }
    }
#ifdef QT_DEBUG
    //交点圆弧是按照顺时针的方式存取的。
    qDebug()<<"circle cross count:"<<outCrosses->size();
#endif
    p1 = NULL;
    p2 = NULL;

    return circleContainingBounds;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算圆弧中点
  * 思路：作弦的平行线，
  * 其中与圆相切的有两条线，
  * 按照弦的前进方向，判断取那条直线。
  * 用切线与弦中点和圆心组成的直线求交点。
  */
float *calculateArcCenter(Point *p1, Point *p2, short x, short y, short r)
{
    float *result = new float[2];
    result[0] = 0;
    result[1] = 0;

    //直线p1p2方程
    float x1 = p1->x;
    float y1 = p1->y;

    float x2 = p2->x;
    float y2 = p2->y;

    //圆心中点
    float x3 = x;
    float y3 = y;
    float radius = r;

    //两交点的直线方程
    float A1 = y2 - y1;
    float B1 = x1 - x2;
    //float C1 = y1 * (x2 - x1) - x1 * (y2 - y1);
    //平行于两交点的直线
    float C11 = radius * sqrt(A1 * A1 + B1 * B1) - A1 * x3 - B1 * y3;
    //float C12 = radius * sqrt(A1 * A1 + B1 * B1) * (-1) - A1 * x3 - B1 * y3;

    //O与圆弧中点的连线与两交点的连线所在直线垂直，
    //所以A2 = -B1，B2 = A1，代入圆心即可计算出C2。
    float A2 = B1;
    float B2 = -A1;
    float C2 = (-1) * (A2 * x3 + B2 * y3);
    //计算交点
    result[0] = (B1 * C2 - B2 * C11) / (A1 * B2 - A2 * B1);
    result[1] = (A1 * C2 - A2 * C11) / (A2 * B1 - A1 * B2);

    float dx = x2 - x1;
    float dy = y2 - y1;

    bool okay = false;
    //根据弦前进方向判断弧中点
    //弦是顺时针前进，
    //所以弧中点在左手边方向。
    if(dx == 0){
        if(dy < 0){
            okay = result[0] > x1;
        }else{
            okay = result[0] < x1;
        }
    }else if(dx > 0){
        if(dy <= 0 ){
            okay = result[0] > x1;
        }else{
            okay = result[0] < x2;
        }
    }else{
        if(dy <= 0){
            okay = result[0] > x2;
        }else{
            okay = result[0] < x1;
        }
    }

    if(okay){
        return result;
    }
    float C12 = radius * sqrt(A1 * A1 + B1 * B1) * (-1) - A1 * x3 - B1 * y3;
    //计算交点
    result[0] = (B1 * C2 - B2 * C12) / (A1 * B2 - A2 * B1);
    result[1] = (A1 * C2 - A2 * C12) / (A2 * B1 - A1 * B2);

    return result;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 判断圆在多边形内部、外部、还是相交。
  * 并将交点存入clipCirclePoints中。
  */
bool calculateCircleStatistic(Circle *cl, vector<Point> *bounds)
{
    vector<Point> cross;
    bool *circleContaining = new bool(calculateCircleCrossPoints(cl,bounds,&cross));

#ifdef QT_DEBUG
    qDebug()<<"calculateCircleStatistic size:"<<cross.size();
#endif
    vector<Point> unique = getUniquePoints(&cross);

    circleClipStatistic.crossPointCount += unique.size();

    //圆包括多边形
    if(*circleContaining){

        circleClipStatistic.outCount ++;

        circleFlags.push_back(EctadCircle);

        {
            vector<Point> temp;
            temp.swap(cross);
        }
        {
            vector<Point> temp;
            temp.swap(unique);
        }

        delete circleContaining;

        return true;
    }
    delete circleContaining;

    Point *p = NULL;
    int total = unique.size();

    bool acturalNoCross = true;

    //遍历所有交点，如果有穿过顶点或者穿过边的点，
    //那么圆与多边形一定相交。
    for(int i = 0;i < total;i++){
        p = &(unique.at(i));
        if(p->type == CrossVertexPoint || p->type == CrossLinePoint){
            acturalNoCross = false;
            break;
        }
    }

    if(acturalNoCross){
        //根据圆心是否在多边形内部，判断圆是否在多边形内部。
        if(pointWithinBounds(cl->x,cl->y,bounds)){

            circleClipStatistic.inCount ++;

            circleFlags.push_back(WithinCircle);
#ifdef QT_DEBUG
            qDebug()<<"inner circle:";
#endif

        }else{
#ifdef QT_DEBUG
            qDebug()<<"outer circle:";
#endif
            circleClipStatistic.outCount ++;

            circleFlags.push_back(EctadCircle);
        }
    }else{

        circleClipStatistic.crossCount ++;

        circleFlags.push_back(CrossCircle);

        //按照 圆和交点 的方式保存
        CirclePoint cp;

        cp.r = cl->r;
        cp.x = cl->x;
        cp.y = cl->y;

        for(int i = 0;i < total;i++){
            p = &(unique.at(i));

            cp.points.push_back(clonePoint(p));
        }

        clipCirclePoints.push_back(cp);
    }
    //释放内存
    {
        vector<Point> temp;
        temp.swap(cross);
    }
    {
        vector<Point> temp;
        temp.swap(unique);
    }

    p = NULL;

    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 判断所有圆在多边形内部、外部、还是相交。
  */
bool calculateCirclesStatistic(vector<Circle> *cls, vector<Point> *bounds)
{
    //清理clipCirclePoints中的元素，
    //并释放内存，准备保存圆和交点。
    {
        vector<CirclePoint> temp;
        temp.swap(clipCirclePoints);
    }

    int total = cls->size();

    circleClipStatistic.total = total;

    if(total == 0)
        return false;
    Circle *cl = NULL;
    for(int i = 0;i < total;i++){
        cl = &(cls->at(i));
        calculateCircleStatistic(cl,bounds);
    }
    cl = NULL;
    //释放内存
    {
        vector<CirclePoint> temp;
        temp.swap(clipedCircleArcs);
    }

    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 得到不重复的点，
  * 用于去除圆与多边形重复的交点。
  */
vector<Point> getUniquePoints(vector<Point> *crosses)
{
    vector<Point> result;

    if(crosses->size() == 0)
        return result;

    Point *p = NULL;
    Point *last = NULL;

    p = &(crosses->at(0));

    Point pp = clonePoint(p);

    result.push_back(pp);

    last = &pp;

    for(int i = 1;i < crosses->size();i++){
        p = &(crosses->at(i));
        if((p->x == last->x) && (p->y == last->y)){
            continue;
        }
        Point ppp = clonePoint(p);
        result.push_back(ppp);

        last = &ppp;
    }
    p = NULL;
    last = NULL;

    return result;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 计算与多边形相交的圆裁剪后的圆弧
  */
bool calculateWithinCircleArcs(vector<Point> *bounds)
{
    int total = clipCirclePoints.size();

    Point *p1 = NULL;
    Point *p2 = NULL;

    float *p3 = NULL;

    for(int i = 0;i < total;i++){

        CirclePoint cp = clipCirclePoints.at(i);

        CirclePoint cpp;
        cpp.r = cp.r;
        cpp.x = cp.x;
        cpp.y = cp.y;

        int pointTotal = cp.points.size();

        for(int j = 0;j < pointTotal;j++){
            //取出交点
            p1 = &(cp.points.at(j % pointTotal));
            p2 = &(cp.points.at((j + 1) % pointTotal));
            //计算弧中点
            p3 = calculateArcCenter(p1,p2,cpp.x,cpp.y,cpp.r);

            if(pointWithinBounds(p3[0],p3[1],bounds)){
                cpp.points.push_back(clonePoint(p1));
                cpp.points.push_back(clonePoint(p2));
#ifdef QT_DEBUG
                qDebug()<<"center Arc:"<<p1->x<<p1->y<<":"<<p2->x<<p2->y;
#endif
            }
            //释放内存
            delete []p3;
            p3 = NULL;
        }
        //释放内存
        {
            vector<Point> temp;
            temp.swap(cp.points);
        }

        clipedCircleArcs.push_back(cpp);
    }

    p1 = NULL;
    p2 = NULL;
    //释放内存
    {
        vector<CirclePoint> temp;
        temp.swap(clipCirclePoints);
    }

    return true;
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 比较两个点的坐标是否相等
  */
bool compare2Points(Point *p1, Point *p2)
{
    return ((p1->x == p2->x) && (p1->y == p2->y));
}

/**
  * @author yuliuchuan
  * @date 2015-04-10
  * 判断与圆相交的多边形的顶点的类型，
  * 即圆弧是否穿过交点。
  * 思路：过交点（也就是顶点）做圆的切线，
  * 在切点两边各取一个点，用这两个点组成一条线段，
  * 该问题就转换为判断与多边形的交点是顶点的线段的点的类型。
  */
PointType circlePassByVertexPointType(Point *vertex, vector<Point> *bounds, Circle *cl)
{
    Point *vertexL = NULL;
    Point *vertexR = NULL;

    int index = calculateVertexIndex(vertex->x,vertex->y,bounds);

    int total = bounds->size();

    if(index == 0){
        vertexL = &(bounds->at(total - 2));
        vertexR = &(bounds->at(1));
    }else{
        vertexL = &(bounds->at(index - 1));
        vertexR = &(bounds->at(index + 1));
    }

    // 圆心-顶点 直线方程
    float A1 = vertex->y - cl->y;
    float B1 = cl->x - vertex->x;
    // 切线直线方程
    float A2 = -B1;
    float B2 = A1;
    float C2 = (-1) * (A2 * vertex->x + B2 * vertex->y);

    //生成切线上的点
    Point p1 = createPoint(0,0);
    Point p2 = createPoint(0,0);

    if(B2 == 0){
        //与y轴平行
        p1.x = vertex->x;
        p2.x = vertex->x;
        p1.y = vertex->y - 10;
        p2.y = vertex->y + 10;
    }else{
        p1.x = vertex->x - 10;
        p2.x = vertex->x + 10;
        p1.y = (A2 * p1.x + C2) / (-B2);
        p2.y = (A2 * p2.x + C2) / (-B2);
    }

    return linePassByVertexPointType(vertex,vertexR,vertexL,true,&p1,&p2);
}
