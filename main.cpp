#include <QCoreApplication>
#include <QtGlobal>
#include "dataswitchcenter.h"

// 加锁
QMutex mutex;


void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
 {
     QByteArray localMsg = msg.toLocal8Bit();
     QString logLevel;
     switch (type) {
     case QtDebugMsg:
         logLevel = "Debug";
         fprintf(stderr, "Debug: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
         break;
     case QtInfoMsg:
         logLevel = "Info";
         fprintf(stderr, "Info: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
         break;
     case QtWarningMsg:
         logLevel = "Warning";
         fprintf(stderr, "Warning: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
         break;
     case QtCriticalMsg:
         logLevel = "Critical";
         fprintf(stderr, "Critical: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
         break;
     case QtFatalMsg:
         logLevel = "Fatal";
         fprintf(stderr, "Fatal: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
         abort();
     }

     // 设置输出信息格式
     QString strDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
     QString strMessage = QString("%1 %2 %3").arg(logLevel).arg(strDateTime).arg(QString(localMsg));

     QMutexLocker locker(&mutex);
     QString strCurrentMonth = QDateTime::currentDateTime().toString("yyyyMMdd");
     QFile file(QString("./%1log.txt").arg(strCurrentMonth));
     if(file.open(QIODevice::ReadWrite | QIODevice::Append))
     {
         QTextStream stream(&file);
         stream << strMessage << "\r\n";
         file.flush();
         file.close();
     }

     QString strLastMonth = QDateTime::currentDateTime().addDays(-3).toString("yyyyMMdd");
     if(QFile::exists(QString("./%1log.txt").arg(strLastMonth)))
         QFile::remove(QString("./%1log.txt").arg(strLastMonth));
 }

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qInstallMessageHandler(outputMessage);

    DataSwitchCenter dataSwitchCenter;

    return a.exec();
}
