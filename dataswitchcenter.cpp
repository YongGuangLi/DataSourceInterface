#include "dataswitchcenter.h"


typedef union{
    unsigned int length;
    unsigned char bytes[4];
}Int2Bytes;


DataSwitchCenter::DataSwitchCenter(QObject *parent) : QObject(parent)
{
    isRunning = true;
    isConnected = false;

    //以字母LiyG的ascii作为起始标志
    headBytes.append(0x4C);
    headBytes.append(0x69);
    headBytes.append(0x79);
    headBytes.append(0x47);

    initConfigFile(qApp->applicationDirPath() + QDir::separator() + "system.conf");

    iniSQLiteFile();

    initThriftClient();

    initTcpSocket();

    //订阅redis
    QtConcurrent::run(this, &DataSwitchCenter::redisSubscribe);

    QTimer *thriftTimer = new QTimer(this);
    connect(thriftTimer, SIGNAL(timeout()), this, SLOT(timerQueryThrift()));
    thriftTimer->start(interval * 1000);
}


DataSwitchCenter::~DataSwitchCenter()
{
    isRunning = false;
}


void DataSwitchCenter::initConfigFile(QString fileName)
{
    if(QFile::exists(fileName))
    {
        QSettings* settings_ = new QSettings(fileName,QSettings::IniFormat, this);
        settings_->setIniCodec("UTF-8");

        settings_->beginGroup("Period");
        interval = settings_->value("Interval").toInt();
        settings_->endGroup();

        settings_->beginGroup("XDB_Config");
        xdbIp = settings_->value("XdbIp").toString();
        xdbPort = settings_->value("XdbPort").toInt();
        xdbDir = settings_->value("XdbDir").toString();
        settings_->endGroup();

        settings_->beginGroup("Redis_Config");
        redisIp = settings_->value("RedisIp").toString();
        redisPort = settings_->value("RedisPort").toInt();
        redisPasswd = settings_->value("RedisPasswd").toString();
        settings_->endGroup();

        settings_->beginGroup("Tcp_Server");
        serverIp = settings_->value("Ip").toString();
        serverPort = settings_->value("Port").toInt();
        settings_->endGroup();
    }
}



void DataSwitchCenter::iniSQLiteFile()
{
    if(QSqlDatabase::contains("qt_sql_default_connection"))
        db = QSqlDatabase::database("qt_sql_default_connection");
    else
        db = QSqlDatabase::addDatabase("QSQLITE");

    //设置数据库路径，不存在则创建
    db.setDatabaseName("hndw.db");

    //打开数据库
    if(db.open())
    {
        qDebug()<<"SQLite Open Success";

        QString sql = QString("SELECT FULL_POINT_CODE FROM pointTable");
        QSqlQuery query;
        query.exec(sql);

        while(query.next())
        {
            QString pointName = query.value(0).toString();

            Point tmpPoint;
            tmpPoint.dir =  xdbDir.toStdString();
            tmpPoint.name = pointName.toStdString();             //ZJSWZDC_D1_ycnd
            vecXdbQueryPoints.push_back(tmpPoint);

            //qDebug()<<"Point Dir:"<<tmpPoint.dir.c_str()<<" Name:"<<tmpPoint.name.c_str();
        }

        sql = QString("SELECT WRITE_BACK_CODE,FULL_POINT_CODE FROM mapTable");
        query.exec(sql);

        while(query.next())
        {
            QString writeBackCode = query.value(0).toString();
            QString fullPointCode = query.value(1).toString();

            mapEdnaWriteBackCode[writeBackCode] = fullPointCode;
        }
        db.close();
    }
}


void DataSwitchCenter::initThriftClient()
{
    std::shared_ptr<TTransport> socket(new TSocket(xdbIp.toStdString(), xdbPort));
    std::shared_ptr<TTransport> transport(new TFramedTransport(socket, 2048));
    std::shared_ptr<TProtocol> protocol(new TCompactProtocol(transport));
    xdbClient = new XdbThriftClient(protocol);

    try {
        transport->open();
        qDebug()<< "Xdb Connect Success!";
    } catch(TException& tx)
    {
        qWarning()<<"Xdb Connect Failure,InvalidOperation:"<<tx.what();
        exit(0);
    }


    for(int i = 0; i < vecXdbQueryPoints.size(); ++i)
    {
        Point tmpPoint = vecXdbQueryPoints.at(i);
        bool result = xdbClient->checkPoint(tmpPoint);
        if(!result)
            qWarning()<<QString("Not Exist Point:%1.%2").arg(tmpPoint.dir.c_str()).arg(tmpPoint.name.c_str());
    }
}


void DataSwitchCenter::initTcpSocket()
{
    clientSocket = new QTcpSocket();

    //连接服务器成功
    connect(clientSocket, SIGNAL(connected()), SLOT(onConnected()));
    //读取消息
    connect(clientSocket, SIGNAL(readyRead()), SLOT(onReadyRead()));
    //断开连接
    connect(clientSocket, SIGNAL(disconnected()), SLOT(onDisconnected()));

    //连接
    clientSocket->connectToHost(QHostAddress(serverIp), serverPort);
    if(!clientSocket->waitForConnected())
        qWarning()<<"TcpServer Connect Failure!"<<serverIp<<serverPort<<clientSocket->errorString();
}


void DataSwitchCenter::onConnected()
{
    qDebug() << "TcpServer Connect Success!";
    isConnected = true;
}


//读取消息
void DataSwitchCenter::onReadyRead()
{
    clientSocket->readAll();
}


//断开连接
void DataSwitchCenter::onDisconnected()
{
    qDebug() << "disconnected";
    isConnected = false;
    if(!clientSocket)
        return;

    if(clientSocket->state()== QAbstractSocket::UnconnectedState || clientSocket->waitForDisconnected(1000))
        return;
}


std::vector<UserProtocalStru> DataSwitchCenter::msgpackConvert(string buf)
{
    std::vector<UserProtocalStru> my_class_vec_r;

    msgpack::v1::unpacked msg;
    try
    {
        msgpack::v1::unpack(&msg, buf.data(), buf.size());
    }catch (msgpack::unpack_error & e)
    {
        qWarning()<< "Serialization Fauilre, Err:"<<e.what();
    }

    try
    {
        msgpack::v1::object obj = msg.get();
        obj.convert(&my_class_vec_r);
    }catch (msgpack::type_error & e)
    {
        qWarning()<< "Serialization Fauilre, Err:"<<e.what();
    }

    return my_class_vec_r;
}



bool DataSwitchCenter::ckeckTimestamp(qint64 timestamp)
{
    if(QDateTime::fromMSecsSinceEpoch(timestamp) < QDateTime::fromString("2100-01-01 00:00:00","yyyy-MM-dd hh:mm:ss"))
        return true;
    else
        return false;
}


void DataSwitchCenter::putPointsToXdb(std::vector<UserProtocalStru> vecUserProtocalStru)
{
    std::vector<Point> vecRealPoints;
    for(int i = 0; i < vecUserProtocalStru.size(); ++i)
    {
        UserProtocalStru tmpUserProtocal = vecUserProtocalStru.at(i);

        QString fullPointCode = mapEdnaWriteBackCode.value(QString::fromStdString(tmpUserProtocal.PointName), QString::fromStdString(tmpUserProtocal.PointName));    //tmpUserProtocal.PointName  带目录名的全称

        Point tmpPoint;
        tmpPoint.dir = xdbDir.toStdString();
        tmpPoint.name = fullPointCode.toStdString();
        tmpPoint.value = QString::fromStdString(tmpUserProtocal.Value).toDouble();
        tmpPoint.time = QString::fromStdString(tmpUserProtocal.Datetime).toLongLong() * 1000;              //tmpUserProtocal.Datetime 单位是秒

        if((tmpPoint.time <= 0) || (!ckeckTimestamp(tmpPoint.time)))
        {
            qWarning()<<"Illeal timestamp,"<<" PointName:"<<tmpPoint.name.c_str() <<" Value:"<<tmpPoint.value<<" Datetime:"<<tmpPoint.time;
            continue;
        }

        qDebug()<<"PointName"<<tmpUserProtocal.PointName.c_str()<<" ToXdb "<<"PointName:"<<tmpPoint.name.c_str() <<" Value:"<<tmpPoint.value<<" Datetime:"<<tmpPoint.time
               <<" TimeStamp:"<<QDateTime::fromTime_t(QString::fromStdString(tmpUserProtocal.Datetime).toUInt()).toString("yyyy-MM-dd hh:mm:ss");;
        vecRealPoints.push_back(tmpPoint);
    }


    QMutexLocker mutexLocker(&mutex);
    if(vecRealPoints.size() > 0)
    {
        string strErr;
        xdbClient->putRealPoints(strErr, vecRealPoints);
        if(!strErr.empty())
            qWarning()<<"Xdb putRealPoints Err:"<<strErr.c_str();
    }
}


void DataSwitchCenter::redisSubscribe()
{
    while(1)
    {
        if(SingleRedisHelp->connect(redisIp.toStdString(), redisPort, redisPasswd.toStdString()))
            qDebug()<<"Redis Connect Success!";
        else
        {
            qWarning()<<QString("Redis Connect Failure:%1, RedisIp:%2").arg(SingleRedisHelp->getErr().c_str()).arg(redisIp);
            exit(0);
        }

        if(SingleRedisHelp->subscribe("DataSourceProxy", NULL))
            qDebug()<<QString("Redis Subscribe Success");
        else
        {
            qWarning()<<QString("Redis Subscribe Failure:%1").arg(SingleRedisHelp->getErr().c_str());
            exit(0);
        }

        while(1)
        {
            string message;
            string channel;
            if(SingleRedisHelp->getMessage(message, channel))
            {
                vector<UserProtocalStru> vecUserProtocalStru = msgpackConvert(message);
                putPointsToXdb(vecUserProtocalStru);
            }
        }
    }
}


bool DataSwitchCenter::writeBytesToServer(const char *data, qint64 len)
{
    if(!isConnected)
    {
        //发送失败，重连
        clientSocket->abort();
        clientSocket->connectToHost(QHostAddress(serverIp), serverPort);
        if(!clientSocket->waitForConnected())
        {
            qWarning()<<"TcpServer ReConnect Failure!"<<serverIp<<serverPort<<clientSocket->errorString();;
            return false;
        }
    }

    clientSocket->write(data, len);
    bool result = clientSocket->waitForBytesWritten();
    if(!result)
        isConnected = false;

    return result;
}


void DataSwitchCenter::timerQueryThrift()
{
    QMutexLocker mutexLocker(&mutex);

    std::vector<Point> vecReturnPoints;
    try
    {
        xdbClient->getRealPoints(vecReturnPoints, vecXdbQueryPoints);
    } catch(TException& tx)
    {
        qWarning()<< "getRealPoints Error:"<<tx.what();
        exit(0);
    }

    vector<UserProtocalStru> vecUserProtocalStru;
    for(int i = 0; i < vecReturnPoints.size(); ++i)
    {
        Point tmpPoint = vecReturnPoints.at(i);
        if(i < 5)
            qDebug()<<QString("queryThrift:%1.%2 %3 %4").arg(tmpPoint.dir.c_str()).arg(tmpPoint.name.c_str()).arg(tmpPoint.value).arg(tmpPoint.time);

        if((tmpPoint.time <= 0) || (!ckeckTimestamp(tmpPoint.time)))
        {
            qWarning()<<"Illeal timestamp,"<<"PointName:"<<tmpPoint.name.c_str()<<" Datetime:"<<tmpPoint.time;
            continue;
        }

        UserProtocalStru tmpUserProtocal;
        tmpUserProtocal.PointName = tmpPoint.name;
        tmpUserProtocal.Value = QString::number(tmpPoint.value).toStdString();
        tmpUserProtocal.Datetime =  QString::number(tmpPoint.time).toStdString();

        vecUserProtocalStru.push_back(tmpUserProtocal);
    }


    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, vecUserProtocalStru);

    Int2Bytes int2Bytes;
    int2Bytes.length = sbuf.size();

    qDebug()<<"Waiting to send PointCount:"<<vecUserProtocalStru.size()<<" dataLength:"<<int2Bytes.length;

    QByteArray bytes;
    bytes.append(headBytes); //以字母LiyG的ascii作为起始标志

    bytes.append((const char*)int2Bytes.bytes, 4);
    bytes.append(sbuf.data(), sbuf.size());

    if(!writeBytesToServer(bytes.data(), bytes.size()))
        qWarning()<<"UserProtocalStru Message Send Failure!";
}
