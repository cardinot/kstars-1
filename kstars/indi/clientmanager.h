/*  INDI Client Manager
    Copyright (C) 2012 Jasem Mutlaq (mutlaqja@ikarustech.com)

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

*/

#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <baseclient.h>
#include <QObject>

#include "config-kstars.h"

class DeviceInfo;
class DriverInfo;
class ServerManager;

/**
 * @class ClientManager
 * ClientManager manages connection to INDI server, creation of devices, and receiving/sending properties.
 *
 * ClientManager is a subclass of INDI::BaseClient class part of the INDI Library.
 * This enables the class to communicate with INDI server and to receive notification of devices, properties, and messages.
 *
 * @author Jasem Mutlaq
 * @version 1.0
 */
class ClientManager : public QObject, public INDI::BaseClient
{
    Q_OBJECT

public:
    ClientManager();
    virtual ~ClientManager();

    void appendManagedDriver(DriverInfo *dv);
    void removeManagedDriver(DriverInfo *dv);

    int count() { return managedDrivers.count(); }

    ServerManager* getServerManager() { return sManager;}

    DriverInfo * findDriverInfoByName(const QString &name);
    DriverInfo * findDriverInfoByLabel(const QString &label);

    bool isDriverManaged(DriverInfo *);

    QList<DriverInfo *> getManagedDrivers() const;

protected:
    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void newProperty(INDI::Property *prop);
    virtual void removeProperty(INDI::Property *prop);
    #ifdef INDI_VERSION_MAJOR
    #if (INDI_VERSION_MAJOR >= 1 && INDI_VERSION_MINOR >= 1)
    virtual void removeDevice(INDI::BaseDevice *dp);
    #endif
    #endif
    virtual void newBLOB(IBLOB *bp);
    virtual void newSwitch(ISwitchVectorProperty *svp);
    virtual void newNumber(INumberVectorProperty *);
    virtual void newText(ITextVectorProperty *);
    virtual void newLight(ILightVectorProperty *);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID);

    virtual void serverConnected();
    virtual void serverDisconnected(int exit_code);

private:

    QList<DriverInfo *> managedDrivers;
    ServerManager *sManager;

signals:
    void connectionSuccessful();
    void connectionFailure(ClientManager *);

    void newINDIDevice(DeviceInfo *dv);
    void removeINDIDevice(DeviceInfo *dv);

    void newINDIProperty(INDI::Property *prop);
    void removeINDIProperty(INDI::Property *prop);

    void newINDIBLOB(IBLOB *bp);
    void newINDISwitch(ISwitchVectorProperty *svp);
    void newINDINumber(INumberVectorProperty *nvp);
    void newINDIText(ITextVectorProperty *tvp);
    void newINDILight(ILightVectorProperty *lvp);
    void newINDIMessage(INDI::BaseDevice *dp, int messageID);

    //void INDIDeviceRemoved(DeviceInfo *dv);

};

#endif // CLIENTMANAGER_H
