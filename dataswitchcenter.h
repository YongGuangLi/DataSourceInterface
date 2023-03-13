/*
 *  dataswitchcenter.h
 *
 *  Created on: Sep 7, 2022
 *      Author: LiyG
 */

#ifndef DATASWITCHCENTER_H
#define DATASWITCHCENTER_H

#include <QObject>
#include <QThreadPool>
#include <QtConcurrent>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>
#include <unistd.h>
#include <QMutexLocker>
#include <QTcpSocket>

#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TCompactProtocol.h>

#include "RedisHelper.h"

#include "XdbThrift.h"

#define MSGPACK_NO_BOOST

#include "msgpack.hpp"

struct  UserProtocalStru
{
 std::string ObjectID;
 std::string PointName;
 std::string Value;
 std::string Datetime;
public:
 MSGPACK_DEFINE(ObjectID, PointName, Value, Datetime)
};




using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

class DataSwitchCenter : public QObject
{
    Q_OBJECT
public:
    explicit DataSwitchCenter(QObject *parent = 0);
    ~DataSwitchCenter();

    /**
    * @date      2022-07-14
    * @param
    * @return
    * @brief     初始化conf文件
    */
    void initConfigFile(QString fileName);

    /**
    * @date      2022-07-14
    * @param
    * @return
    * @brief     初始化sqlite文件
    */
    void iniSQLiteFile();


    /**
    * @date      2022-07-14
    * @param
    * @return
    * @brief     初始化thrift客户的
    */
    void initThriftClient();

    /**
    * @date      2022-07-14
    * @param
    * @return
    * @brief     初始化tcp客户端
    */
    void initTcpSocket();

    /**
    * @date      2022-09-21
    * @param
    * @return
    * @brief     反序列msgpack数据
    */
    std::vector<UserProtocalStru> msgpackConvert(string buf);



    /**
    * @date      2022-09-22
    * @param
    * @return
    * @brief     检查时标是否合法
    */
    bool ckeckTimestamp(qint64 timestamp);

    /**
    * @date      2022-09-22
    * @param
    * @return
    * @brief     数据存入xdb,时标为毫秒
    */
    void putPointsToXdb(std::vector<UserProtocalStru> vecUserProtocalStru);

    /**
    * @date      2022-09-22
    * @param
    * @return
    * @brief     redis订阅
    */
    void redisSubscribe();


    /**
    * @date      2022-04-24
    * @param
    * @return
    * @brief     数据发送给服务端
    */
    bool writeBytesToServer(const char *data, qint64 len);
signals:

public slots:
    void timerQueryThrift();

    void onConnected();

    void onReadyRead();

    void onDisconnected();
private:
    RedisHelper *redisHelper;

    QSqlDatabase db;

    QUdpSocket *udpSocket;

    QTcpServer *tcpServer;

    QMap<QString, QString> mapEdnaWriteBackCode;

    std::vector<Point> vecXdbQueryPoints;
private:
    XdbThriftClient *xdbClient;
    int interval;

    QString xdbIp;
    int xdbPort;
    QString xdbDir;

    QString redisIp;
    int redisPort;
    QString redisPasswd;

    QString serverIp;
    int serverPort;

    bool isRunning;

    bool isConnected;

    QMutex mutex;

    QTcpSocket *clientSocket;

    QByteArray headBytes;
};

#endif // DATASWITCHCENTER_H
