#include "dataswitchcenter.h"

DataSwitchCenter::DataSwitchCenter(QObject *parent) : QObject(parent)
{
    initConfigFile(qApp->applicationDirPath() + QDir::separator() + "system.conf");
    //QtConcurrent::run(this, &DataSwitchCenter::thriftCollectData);

    iniDatabase();
    //initUdpSocket();
}

void DataSwitchCenter::thriftCollectData()
{
    //SingleRedisHelp->connect("192.168.12.200", 6379, "iec61850");

    /*
    std::shared_ptr<TTransport> socket(new TSocket("", 123));
    std::shared_ptr<TTransport> transport(new TFramedTransport(socket, 2048));
    std::shared_ptr<TProtocol> protocol(new TCompactProtocol(transport));
    XdbThriftClient *client_ = new XdbThriftClient(protocol);


    try {
        transport->open();

    } catch(TException& tx)
    {
        qDebug()<< "InvalidOperation:"<<tx.what();
    }


    std::vector<Point> vecQueryPoints;

    client_->getRealPoints(vecReturnPoints, vecQueryPoints);
    */


    while (true)
    {
        for(int i = 0; i < 50; ++i)
        {
            UserProtocalStru send_msg;
            send_msg.objectID = "12312312";
            send_msg.value = QString::number(i).toStdString();
            send_msg.type = "1";
            send_msg.datetime = "12312312";

            vectUserStruct.push_back(send_msg);
        }

        msgpack::sbuffer sbuf;
        msgpack::pack(sbuf, vectUserStruct);
        SingleRedisHelp->publish("IEC61850SERVER", sbuf.data(), sbuf.size());
        sleep(1);
    }
}

void DataSwitchCenter::iniDatabase()
{
    //qDebug()<<QSqlDatabase::drivers();//打印驱动列表

    //检测已连接的方式 - 默认连接名
    //QSqlDatabase::contains(QSqlDatabase::defaultConnection)
    if(QSqlDatabase::contains("qt_sql_default_connection"))
        db = QSqlDatabase::database("qt_sql_default_connection");
    else
        db = QSqlDatabase::addDatabase("QSQLITE");

    //设置数据库路径，不存在则创建
    db.setDatabaseName("CIDconf.db");


    //打开数据库
    if(db.open()){
        qDebug()<<"open success";

        QString sql = QString("SELECT * FROM SWGataway");
        QSqlQuery query;
        query.exec(sql);

        while(query.next()){
            qDebug()<<query.value(0).toString();
            qDebug()<<query.value(1).toString();
            qDebug()<<query.value(2).toString();
            qDebug()<<query.value(3).toString();
        }
        db.close();
    }
}

void DataSwitchCenter::initConfigFile(QString fileName)
{
    if(QFile::exists(fileName))
    {
        QSettings* settings_ = new QSettings(fileName,QSettings::IniFormat, this);
        settings_->setIniCodec("UTF-8");

        settings_->beginGroup("ServerType");
        qDebug()<<settings_->value("Type").toString();
        settings_->endGroup();

        settings_->beginGroup("XDB_Config");
        qDebug()<<settings_->value("XdbIp").toString();
        qDebug()<<settings_->value("XdbPort").toString();
        settings_->endGroup();

        settings_->beginGroup("Redis_Config");
        qDebug()<<settings_->value("RedisIp").toString();
        qDebug()<<settings_->value("RedisPort").toInt();
        qDebug()<<settings_->value("RedisPasswd").toString();
        settings_->endGroup();
    }
}


void DataSwitchCenter::initUdpSocket()
{
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::LocalHost, 7755);

    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
}


void DataSwitchCenter::readPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        //processTheDatagram(datagram);
    }
}
