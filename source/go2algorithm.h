#ifndef GO2ALGORITHM_H
#define GO2ALGORITHM_H

#include <iostream>
#include <vector>
#include <math.h>
#include <QFile>

#ifdef QT_DEBUG
#include <QDebug>
#include <QTime>
#endif

using namespace std;

/**
 * @brief 定义点的类型
 */
enum PointType{
    LinePoint,//线段端点
    VertexPoint,//多边形端点
    CrossLinePoint,//穿过边的交点
    CrossVertexPoint,//穿过顶点的交点
    PassByVertexPoint,//经过顶点的交点
    PassByLinePoint,//经过边的交点
    NoSensePoint//区分线段的点(分割点)
};

/**
 * @brief 定义线的标识
 */
enum LineFlag{
    WithinLine,//在内部
    EctadLine,//在外部
    CrossLine//相交
};

/**
 * @brief 定义圆的标识
 */
enum CircleFlag{
    WithinCircle,//在内部
    EctadCircle,//在外部
    CrossCircle//相交
};

/**
 * @brief 定义点的结构体
 */
struct Point
{
    float x;
    float y;
    PointType type;
};

/**
 * @brief 定义圆的结构体
 * 因为画布大小为1440 * 900，
 * 所以2个字节的short类型就足够
 */
struct Circle
{
    short x;
    short y;
    short r;
};

/**
 * @brief 定义线的统计数据结构体
 *
 */
struct LineClipStatistic
{
    int total;//总数
    int inCount;//在内部的数量
    int outCount;//在外部的数量
    int crossCount;//相交的数量
    int crossPointCount;//交点的数量
};

/**
 * @brief 定义圆的统计数据结构体
 */
struct CirCleClipStatistic
{
    int total;//总数
    int inCount;//在内部的数量
    int outCount;//在外部的数量
    int crossCount;//相交的数量
    int crossPointCount;//交点的数量
};

/**
 * @brief 定义带有交点信息的圆
 */
struct CirclePoint
{
    short x;
    short y;
    short r;
    vector<Point> points;//交点
};

/**
 * @brief readXmlFileFiltedByID 读取xml文件中指定ID的数据
 * @param filename 文件名
 * @param filterId 指定ID
 * @return 文件出错返回false，否则返回true
 */
bool readXmlFileFiltedByID(QString filename, int filterId);

/**
 * @brief readXmlFileAll 读取xml文件中所有的数据
 * @param filename 文件名
 * @return 文件出错返回false，否则返回true
 */
bool readXmlFileAll(QString filename);

/**
 * @brief clearFormerState 初始化或者清除之前的数据
 * @return 运行结束返回true
 */
bool clearFormerState();

/**
 * @brief calculateLineCrossPoints 计算线段和边界的交点
 * @param start 起点
 * @param end 终点
 * @param bounds 边界
 * @return 所有在线段上的交点
 */
vector<Point> calculateLineCrossPoints(Point *start,Point *end,vector<Point> *bounds);

/**
 * @brief pointWithinBounds 判断点是不是在边界内
 * @param centerx 点的x坐标
 * @param centery 点的y坐标
 * @param bounds 边界
 * @return 在内部返回true，否则返回false
 */
bool pointWithinBounds(float centerx, float centery, vector<Point> *bounds);

/**
 * @brief pointWithinBoundsByCrosses 交点检验法判断，供pointWithinBounds调用
 * @param sorted 排序后的交点
 * @param centerx 待判断点的x坐标
 * @param centery 待判断点的y坐标
 * @return 在内部返回true，否则返回false
 */
bool pointWithinBoundsByCrosses(vector<Point> sorted, float centerx, float centery);

/**
 * @brief calculateLineStatistic 统计一条线段的数据
 * @param crosses 线段上的所有交点
 * @param bounds 边界
 * @return 运行结束返回true
 */
bool calculateLineStatistic(vector<Point> *crosses, vector<Point> *bounds);

/**
 * @brief calculateLinesStatistic 统计所有线段的数据
 * @param lines 所有线段
 * @param bounds 边界
 * @return 运行结束返回true
 */
bool calculateLinesStatistic(vector<Point> *lines,vector<Point> *bounds);

/**
 * @brief calculateVertexIndex 计算顶点在边界顶点中的索引值
 * @param x x坐标
 * @param y y坐标
 * @param bounds 边界
 * @return 不是顶点返回-1，否则返回实际索引值
 */
int calculateVertexIndex(float x,float y,vector<Point> *bounds);

/**
 * @brief linePassByVertexPointType 计算经过交点是顶点的交点的类型（经过or穿过）
 * @param vertex 当前顶点
 * @param vertexR 下一顶点
 * @param vertexL 上一顶点
 * @param centerPoint 是不是中点(线段上的点、圆弧中点、圆心)，
 *        true的意思是判断经过线段上的点或者圆心做Y轴的平行线与多边形的交点的类型，
 *        false的意思是判断线段的交点。
 * @param start 线段的起点
 * @param end 线段的中点
 * @return 经过返回PassByVertexPoint，穿过返回CrossVertexPoint
 */
PointType linePassByVertexPointType(Point *vertex, Point *vertexR, Point *vertexL, bool centerPoint, Point *start, Point *end);

/**
 * @brief getValidLineCrossPoints 计算有效的交点
 * @param crosses 所有交点
 * @return 在线段上的排序的唯一值交点
 */
vector<Point> getValidLineCrossPoints(vector<Point> *crosses);

/**
 * @brief getSortedUniquePoints 计算排序后的唯一值点
 * @param crosses 交点
 * @param startIndex 开始的索引
 * @param endIndex 结束的索引
 * @return 返回从[startIndex,endIndex]的排序后的唯一值点
 */
vector<Point> getSortedUniquePoints(vector<Point> *crosses,int startIndex,int endIndex);

/**
 * @brief calculateWithinLines 计算相交线段裁剪后的线段
 * @param bounds 边界
 * @return 运行结束返回true
 */
bool calculateWithinLines(vector<Point> *bounds);

/**
 * @brief clonePoint 克隆点
 * @param p 要克隆的点
 * @return 返回克隆后的点
 */
Point clonePoint(Point *p);

/**
 * @brief createNoSensePoint 创建分割线段的点
 * @return 返回(0,0)的NoSensePoint类型的点
 */
Point createNoSensePoint();

/**
 * @brief createPoint 创建LinePoint类型的点
 * @param x
 * @param y
 * @return 返回(x,y)的LinePoint类型的点
 */
Point createPoint(float x, float y);

/**
 * @brief calculateCircleCrossPoints 计算圆与边界的交点
 * @param cl 圆
 * @param bounds 边界
 * @param outCrosses 输出的交点
 * @return 若圆包含多边形返回true，否则返回false
 */
bool calculateCircleCrossPoints(Circle *cl, vector<Point> *bounds, vector<Point> *outCrosses);

/**
 * @brief calculateArcCenter 计算顺时针方向的圆弧中点（因为多边形的点为顺时针方向存储）
 * @param p1 交点1
 * @param p2 交点2
 * @param x 圆心x坐标
 * @param y 圆心y坐标
 * @param r 圆心半径
 * @return 返回圆弧中点数组指针,[0]为x,[1]为y
 */
float *calculateArcCenter(Point *p1, Point *p2, short x, short y, short r);

/**
 * @brief calculateCircleStatistic 统计一个圆的数据
 * @param cl 圆
 * @param bounds 边界
 * @return 运行结束返回true
 */
bool calculateCircleStatistic(Circle *cl, vector<Point> *bounds);

/**
 * @brief calculateCirclesStatistic 统计所有圆的数据
 * @param cls 所有圆
 * @param bounds 边界
 * @return 运行结束返回true
 */
bool calculateCirclesStatistic(vector<Circle> *cls,vector<Point> *bounds);

/**
 * @brief getUniquePoints 返回唯一值点
 * @param crosses 交点
 * @return 返回不重复的交点
 */
vector<Point> getUniquePoints(vector<Point> *crosses);

/**
 * @brief calculateWithinCircleArcs 计算与边界相交的圆裁剪后的圆弧
 * @param bounds 边界
 * @return 运行结束返回true
 */
bool calculateWithinCircleArcs(vector<Point> *bounds);

/**
 * @brief compare2Points 比较两个点是不是在同一位置
 * @param p1 点1
 * @param p2 点2
 * @return 是，返回true；否者返回false。
 */
bool compare2Points(Point *p1,Point *p2);

/**
 * @brief circlePassByVertexPointType 计算经过与圆的交点是顶点的交点的类型（经过or穿过）
 * @param vertex 顶点(交点)
 * @param bounds 边界
 * @param cl 圆
 * @return 经过返回PassByVertexPoint，穿过返回CrossVertexPoint
 */
PointType circlePassByVertexPointType(Point *vertex, vector<Point> *bounds, Circle *cl);
#endif // GO2ALGORITHM_H
