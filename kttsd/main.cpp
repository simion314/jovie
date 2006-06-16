/***************************************************** vim:set ts=4 sw=4 sts=4:
  Where the main function for KTTSD resides.
  -------------------
  Copyright:
  (C) 2002-2003 by José Pablo Ezequiel "Pupeno" Fernández <pupeno@kde.org>
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
  -------------------
  Original author: José Pablo Ezequiel "Pupeno" Fernández
 ******************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 ***************************************************************************/

#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>

#include "kttsd.h"

int main (int argc, char *argv[]){
    KLocale::setMainCatalog("kttsd");
    KAboutData aboutdata("kttsd", I18N_NOOP("kttsd"),
         "0.4.0", I18N_NOOP("Text-to-speech synthesis deamon"),
         KAboutData::License_GPL, "(C) 2002, José Pablo Ezequiel Fernández");
    aboutdata.addAuthor("José Pablo Ezequiel Fernández",I18N_NOOP("Original Author"),"pupeno@pupeno.com");
    aboutdata.addAuthor("Gary Cramblitt", I18N_NOOP("Maintainer"),"garycramblitt@comcast.net");
    aboutdata.addAuthor("Gunnar Schmi Dt", I18N_NOOP("Contributor"),"gunnar@schmi-dt.de");
    aboutdata.addAuthor("Olaf Schmidt", I18N_NOOP("Contributor"),"ojschmidt@kde.org");
    aboutdata.addAuthor("Paul Giannaros", I18N_NOOP("Contributor"), "ceruleanblaze@gmail.com");
    aboutdata.addCredit("Jorge Luis Arzola", I18N_NOOP("Testing"), "arzolacub@hotmail.com");
    aboutdata.addCredit("David Powell", I18N_NOOP("Testing"), "achiestdragon@gmail.com");

    KCmdLineArgs::init( argc, argv, &aboutdata );
    // KCmdLineArgs::addCmdLineOptions( options );
    KUniqueApplication::addCmdLineOptions();
    
    KUniqueApplication::setOrganizationDomain("kde.org");
    KUniqueApplication::setApplicationName("kttsd");
    KUniqueApplication app;

    if(!KUniqueApplication::start()){
        kDebug() << "KTTSD is already running" << endl;
        return (0);
    }

    // This app is started automatically, no need for session management
    app.disableSessionManagement();
    kDebug() << "main: Creating KTTSD Service" << endl;
    KTTSD *service = new KTTSD();

    // kDebug() << "Entering event loop." << endl;
    return app.exec();
    delete service;
}
