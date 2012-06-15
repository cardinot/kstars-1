/***************************************************************************
                          ksuserdb.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : Wed May 2 2012
    copyright            : (C) 2012 by Rishab Arora
    email                : ra.rishab@gmail.com
 ***************************************************************************/


/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kstarsdata.h"
#include "ksuserdb.h"
#include <kdebug.h>
#include <klocale.h>


bool KSUserDB::initialize() {
    //TODO: This needs to be merged with verification, and made into a static member
    //to achieve 'aggregation' with kstarsdata
    userdb = QSqlDatabase::addDatabase("QSQLITE", "userdb");
    QString dbfile = KStandardDirs::locateLocal( "appdata", "userdb.sqlite" );
    QFile testdb(dbfile);
    bool first = false;
    if (!testdb.exists()){
        kWarning() << i18n("User DB does not exist! New User DB will be created.");
        first = true;
    }
    userdb.setDatabaseName(dbfile);
    if (!userdb.open()) {
           kWarning() << i18n("Unable to open user database file!");
           kWarning() << lastError();
    }
    else {
        kDebug() << i18n("Opened the User DB. Ready!");
    }
    if (first == true){
        firstRun();
    }
    userdb.close();
    return true;
}

void KSUserDB::deallocate() {
    userdb.close();
    QSqlDatabase::removeDatabase("userdb");
}

QSqlError KSUserDB::lastError() {
    //error description is in QSqlError::text()
    return userdb.lastError();
}

// Replacing bool KSUserDB::loadDatabase(QString dbfile = KSTARS_USERDB)
// Every logged in user should have their own db.
bool KSUserDB::verifyDatabase(QString dbfile) {
    //TO BE REMOVED
    //Before QSqlDatabase::database returns a reference to a valid database, 
    //this database must have been opened with open() which means that you can not 
    //simply establish a connection and afterwards and elsewhere retrieve this connection 
    //with QSqlDatabase::database to give some more connectiondata as name, server and 
    //password and only then opening the database. 
return true;    
}

bool KSUserDB::firstRun() {
    kWarning() << i18n("Rebuilding User Database");
    QVector<QString> tables;
    tables.append("CREATE TABLE flags (ra FLOAT,dec FLOAT,epoch INTEGER,imagename TEXT,label TEXT,labelcolor TEXT)");
    tables.append("CREATE TABLE user (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,Name TEXT NOT NULL  DEFAULT 'NULL',Surname TEXT NOT NULL  DEFAULT 'NULL',Contact TEXT DEFAULT NULL)");
    tables.append("CREATE TABLE telescope (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,Vendor TEXT DEFAULT NULL,Aperture REAL NOT NULL  DEFAULT NULL,Model TEXT DEFAULT NULL,Driver TEXT DEFAULT NULL,Type TEXT DEFAULT NULL,Focal Length REAL DEFAULT NULL)");
    tables.append("CREATE TABLE flags (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,RA REAL NOT NULL  DEFAULT NULL,Dec REAL NOT NULL  DEFAULT NULL,Icon TEXT NOT NULL  DEFAULT 'NULL',Label TEXT NOT NULL  DEFAULT 'NULL',Color TEXT DEFAULT NULL,Epoch REAL DEFAULT NULL)");
    tables.append("CREATE TABLE Lens (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,Vendor TEXT NOT NULL  DEFAULT 'NULL',Model TEXT DEFAULT NULL,Factor REAL NOT NULL  DEFAULT NULL)");
    tables.append("CREATE TABLE Eyepiece (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,Vendor TEXT DEFAULT NULL,Model TEXT DEFAULT NULL,FocalLength REAL NOT NULL  DEFAULT NULL,ApparentFOV REAL NOT NULL  DEFAULT NULL)");
    tables.append("CREATE TABLE Filter (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,Vendor TEXT DEFAULT NULL,Model TEXT DEFAULT NULL,Type TEXT DEFAULT NULL,Color TEXT DEFAULT NULL)");
    tables.append("CREATE TABLE Wishlist (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,Date NUMERIC NOT NULL  DEFAULT NULL,Type TEXT DEFAULT NULL,UIUD TEXT DEFAULT NULL)");
    tables.append("CREATE TABLE FOV (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL  DEFAULT 'NULL',color TEXT DEFAULT NULL,sizeX NUMERIC DEFAULT NULL,sizeY NUMERIC DEFAULT NULL,shape TEXT DEFAULT NULL)");
    tables.append("CREATE TABLE logentry (id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,content TEXT NOT NULL  DEFAULT 'NULL',UIUD TEXT DEFAULT NULL,DateTime NUMERIC NOT NULL  DEFAULT NULL,User TEXT DEFAULT NULL REFERENCES user (id),Location TEXT DEFAULT NULL,Telescope TEXT DEFAULT NULL REFERENCES telescope (id),Filter TEXT DEFAULT NULL REFERENCES Filter (id),lens TEXT DEFAULT NULL REFERENCES Lens (id),Eyepiece TEXT DEFAULT NULL REFERENCES Eyepiece (id),FOV TEXT DEFAULT NULL REFERENCES FOV (id))");
    
    for (int i=0; i<tables.count(); i++){
        QSqlQuery query(userdb);
        if (!query.exec(tables[i])) {
            kWarning() << query.lastError();
        }
    }
     
    return true;

}

bool KSUserDB::addObserver(QString name, QString surname, QString contact) {
    userdb = QSqlDatabase::database("userdb");
    QSqlQuery query(userdb);
    query.prepare("INSERT INTO user (Name,Surname,Contact) VALUES (:name, :surname, :contact)");
    query.bindValue(":name", name);
    query.bindValue(":surname", surname);
    query.bindValue(":contact", contact);
    query.exec();
    return true;    
}