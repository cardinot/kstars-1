/*  INDI Server Manager
    Copyright (C) 2012 Jasem Mutlaq (mutlaqja@ikarustech.com)

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

 */

#include <errno.h>
#include <sys/stat.h>

#include <indicom.h>

#include <config-kstars.h>

#include <QTcpSocket>
#include <QTextEdit>

#include <KProcess>
#include <QLocale>
#include <QDebug>
#include <KMessageBox>
#include <QStatusBar>
#include <QStandardPaths>


#include "servermanager.h"
#include "drivermanager.h"
#include "driverinfo.h"

#include "Options.h"
#include "kstars.h"
#include "kstarsdatetime.h"


const int INDI_MAX_TRIES=3;
const int MAX_FILENAME_LEN = 1024;

ServerManager::ServerManager(QString inHost, uint inPort)
{
    serverProcess	= NULL;
    XMLParser		= NULL;
    host		= inHost;
    port		= QString::number(inPort);

    //qDebug() << "We got port unit with value of " << inPort << " and now as tring it is equal to #" << port << "#" << endl;
}

ServerManager::~ServerManager()
{
    serverSocket.close();
    indiFIFO.close();

    QFile::remove(indiFIFO.fileName());

    if (serverProcess)
        serverProcess->close();

    delete (serverProcess);
}

bool ServerManager::start()
  {
    bool connected=false;
    int fd=0;
    serverProcess = new KProcess(this);

    *serverProcess << Options::indiServer();
    *serverProcess << "-v" << "-p" << port;
    *serverProcess << "-m" << "100";

    QString fifoFile = QString("/tmp/indififo%1").arg(QUuid::createUuid().toString().mid(1, 8));

    if ( (fd = mkfifo (fifoFile.toLatin1(), S_IRUSR| S_IWUSR) < 0))
    {
         KMessageBox::error(NULL, i18n("Error making FIFO file %1: %2.", fifoFile, strerror(errno)));
         return false;
   }

    indiFIFO.setFileName(fifoFile);

    driverCrashed = false;

   if (!indiFIFO.open(QIODevice::ReadWrite | QIODevice::Text))
   {
         qDebug() << "Unable to create INDI FIFO file: " << fifoFile << endl;
         return false;
   }


    *serverProcess << "-f" << fifoFile;

    serverProcess->setOutputChannelMode(KProcess::SeparateChannels);
    serverProcess->setReadChannel(QProcess::StandardError);


    serverProcess->start();

    connected = serverProcess->waitForStarted();

    if (connected)
    {
        connect(serverProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processServerError(QProcess::ProcessError)));
        connect(serverProcess, SIGNAL(readyReadStandardError()),  this, SLOT(processStandardError()));

        emit started();
    }

    return connected;



}

bool ServerManager::startDriver(DriverInfo *dv)
{
    QTextStream out(&indiFIFO);

    // Check for duplicates within existing clients
    if (dv->getUniqueLabel().isEmpty() && dv->getTreeLabel().isEmpty() == false)
        dv->setUniqueLabel(DriverManager::Instance()->getUniqueDeviceLabel(dv->getTreeLabel()));

    // Check for duplicates within managed drivers
    if (dv->getUniqueLabel().isEmpty() == false)
    {
        int nset=0;
        QString uniqueLabel;
        QString label = dv->getUniqueLabel();
        foreach(DriverInfo *drv, managedDrivers)
        {
            if (label == drv->getUniqueLabel())
                nset++;
        }
        if (nset > 0)
        {
            uniqueLabel = QString("%1 %2").arg(label).arg(nset+1);
            dv->setUniqueLabel(uniqueLabel);
        }
    }

    managedDrivers.append(dv);
    dv->setServerManager(this);

    if (QStandardPaths::findExecutable(dv->getDriver()).isEmpty())
    {
         KMessageBox::error(NULL, i18n("Driver %1 was not found on the system. Please make sure the package that provides the '%1' binary is installed.", dv->getDriver()));
         return false;
    }

        out << "start " << dv->getDriver();
        if (dv->getUniqueLabel().isEmpty() == false)
            out << " -n \"" << dv->getUniqueLabel() << "\"";
        if (dv->getSkeletonFile().isEmpty() == false)
            out << " -s \"" << Options::indiDriversDir() << QDir::separator() << dv->getSkeletonFile() << "\"";
        out << endl;

        out.flush();

        dv->setServerState(true);

        dv->setPort(port);

        return true;
}

void ServerManager::stopDriver(DriverInfo *dv)
{
    QTextStream out(&indiFIFO);

    managedDrivers.removeOne(dv);

    if (dv->getUniqueLabel().isEmpty() == false)
       out << "stop " << dv->getDriver() << " -n \"" << dv->getUniqueLabel() << "\"" << endl;
    else
        out << "stop " << dv->getDriver() << endl;

    out.flush();
    dv->setServerState(false);
    dv->setPort(dv->getUserPort());
}

bool ServerManager::isDriverManaged(DriverInfo *di)
{
    foreach(DriverInfo *dv, managedDrivers)
    {
        if (dv == di)
            return true;
    }

    return false;
}

void ServerManager::stop()
{
    if (serverProcess == NULL)
        return;

    foreach(DriverInfo *device, managedDrivers)
    {
        device->setServerState(false);
        device->clear();
    }

    serverProcess->disconnect(SIGNAL(error(QProcess::ProcessError)));

    serverProcess->terminate();

    serverProcess->waitForFinished();

    delete serverProcess;

    serverProcess = NULL;

}

void ServerManager::terminate()
{
    if (serverProcess == NULL)
        return;

    serverProcess->terminate();

    serverProcess->waitForFinished();
}

void ServerManager::connectionSuccess()
{

    foreach(DriverInfo *device, managedDrivers)
         device->setServerState(true);

    connect(serverProcess, SIGNAL(readyReadStandardError()),  this, SLOT(processStandardError()));

    emit started();
}

void ServerManager::processServerError(QProcess::ProcessError err)
{
  INDI_UNUSED(err);
  emit serverFailure(this);
}

void ServerManager::processStandardError()
{
    QString stderr = serverProcess->readAllStandardError();

    serverBuffer.append(stderr);

    if (Options::iNDILogging())
    {
        foreach(QString msg, stderr.split("\n"))
            qDebug() << "INDI Server: " << msg;
    }

    if (driverCrashed == false && (serverBuffer.contains("stdin EOF") || serverBuffer.contains("stderr EOF")))
    {
        QStringList parts = serverBuffer.split("Driver");
        foreach(QString driver, parts)
        {
            if (driver.contains("stdin EOF") || driver.contains("stderr EOF"))
            {
                driverCrashed=true;
                QString driverName = driver.left(driver.indexOf(':')).trimmed();
                KMessageBox::information(0, i18n("KStars detected INDI driver %1 crashed. Please check INDI server log in the Device Manager.", driverName));
                break;
            }
        }
   }

    emit newServerLog();
}

QString ServerManager::errorString()
{
    if (serverProcess)
        return serverProcess->errorString();

    return NULL;
}


