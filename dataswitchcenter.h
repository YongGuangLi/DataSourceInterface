#ifndef DATASWITCHCENTER_H
#define DATASWITCHCENTER_H

#include <QObject>
#include <QThreadPool>
#include <QtConcurrent>
#include <QUdpSocket>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <unistd.h>


#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TCompactProtocol.h>

#include "RedisHelper.h"

#include "XdbThrift.h"

#define MSGPACK_NO_BOOST

#include "msgpack.hpp"



using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;


struct  UserProtocalStru
{
public:
    std::string objectID;
    std::string value;
    std::string type;
    std::string datetime;
public:
    MSGPACK_DEFINE(objectID, value, type, datetime)
};

class DataSwitchCenter : public QObject
{
    Q_OBJECT
public:
    explicit DataSwitchCenter(QObject *parent = 0);

    void initConfigFile(QString fileName);

    void thriftCollectData();

    void iniDatabase();

    void initUdpSocket();


signals:

public slots:
    void readPendingDatagrams();

private:
    RedisHelper *redisHelper;
    QSqlDatabase db;
    QUdpSocket *udpSocket;

    std::vector<UserProtocalStru>  vectUserStruct;

    std::vector<Point> vecReturnPoints;
};

#endif // DATASWITCHCENTER_H
