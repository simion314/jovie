/***************************************************** vim:set ts=4 sw=4 sts=4:
  kttsd.cpp
  KTTSD main class
  -------------------
  Copyright : (C) 2002-2003 by José Pablo Ezequiel "Pupeno" Fernández
  -------------------
  Original author: José Pablo Ezequiel "Pupeno" Fernández <pupeno@kde.org>
  Current Maintainer: José Pablo Ezequiel "Pupeno" Fernández <pupeno@kde.org>
 ******************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 ***************************************************************************/

 // $Id$

#include <qcstring.h>
#include <kdebug.h>

#include "kttsd.moc"

#include "kttsd.h"

extern "C" {
    KDEDModule *create_kttsd(const QCString &obj){
        return new KTTSD(obj);
    }
};
/*
class TestObject : public KShared{
    public:
        TestObject(const QCString &_app) : app(_app){
            kdDebug() << "Running: TestObject::TestObject(const QCString &_app)" << endl;
            qWarning("Creating TestObject belonging to '%s'", app.data());
        }
        ~TestObject(){
            kdDebug() << "Running: TestObject::~TestObject()" << endl;
            qWarning("Destructing TestObject belonging to '%s'", app.data());
        }

    protected:
        QCString app;
};
*/

KTTSD::KTTSD(const QCString &obj) : KDEDModule(obj){
    kdDebug() << "Running: KTTSD::KTTSD(const QCString &obj)" << endl;
    // Do stuff here
    //setIdleTimeout(15); // 15 seconds idle timeout.
}

QString KTTSD::world(){
    kdDebug() << "Running: KTTSD::world()" << endl;
    return "Hello World!";
}
/*
void KTTSD::idle(){
    kdDebug() << "Running: KTTSD::idle()" << endl;
    qWarning("TestModule is idle.");
}
*/
/*
void KTTSD::registerMe(const QCString &app){
    kdDebug() << "Running: KTTSD::registerMe(const QCString &app)" << endl;
    insert(app, "kttsd", new TestObject(app));
    // When 'app' unregisters with DCOP, the TestObject will get deleted.
}
*/

