//
// C++ Implementation: kttsmgr
//
// Description: 
//
//
// Author: Gary Cramblitt <garycramblitt@comcast.net>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <kuniqueapplication.h>
#include <kcmultidialog.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <ksystemtray.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <kaboutapplication.h>
#include <dcopclient.h>

#include "kttsmgr.h"

int main (int argc, char *argv[])
{
    KAboutData aboutdata("kttsmgr", I18N_NOOP("kttsmgr"),
        "0.2.0", I18N_NOOP("Text-to-Speech Manager"),
        KAboutData::License_GPL, "(C) 2002, José Pablo Ezequiel Fernández");
    aboutdata.addAuthor("José Pablo Ezequiel Fernández",I18N_NOOP("Original Author"),"pupeno@pupeno.com");
    aboutdata.addAuthor("Gary Cramblitt", I18N_NOOP("Maintainer"),"garycramblitt@comcast.net");
    aboutdata.addAuthor("Gunnar Schmi Dt", I18N_NOOP("Contributor"),"gunnar@schmi-dt.de");
    aboutdata.addAuthor("Olaf Schmidt", I18N_NOOP("Contributor"),"ojschmidt@kde.org");
    aboutdata.addAuthor("Paul Giannaros", I18N_NOOP("Contributor"), "ceruleanblaze@gmail.com");
    KCmdLineArgs::init( argc, argv, &aboutdata );
    // KCmdLineArgs::addCmdLineOptions( options );
    KUniqueApplication::addCmdLineOptions();

    if(!KUniqueApplication::start()){
        kdDebug() << "kttsmgr is already running" << endl;
          return (0);
    }

    KUniqueApplication app;
    
    // The real work of KTSS Manager is done in the KControl Module kcmkttsmgr.
    KCMultiDialog dlg(KCMultiDialog::Plain, "KDE Text-to-Speech Manager", 0, "kttsmgrdlg", false);
    dlg.addModule("kcmkttsmgr");

    // Create system tray object
    KttsMgrTray tray(&dlg);
    
//    app.setMainWidget(&tray);
    tray.show();
    dlg.show();
    return app.exec();
}

/*  KttsMgrTray class */

KttsMgrTray::KttsMgrTray(QWidget *parent):
    DCOPStub("kttsd", "kspeech"),
    KSystemTray(parent, "kttsmgrsystemtray")
{
    QPixmap icon = KGlobal::iconLoader()->loadIcon("kttsd", KIcon::Small);
    setPixmap (icon);
    
    int id;
    id = contextMenu()->insertItem (KGlobal::iconLoader()->loadIcon("klipper", KIcon::Small),
        i18n("&Speak clipboard contents"), this, SLOT(speakClipboardSelected()));
    id = contextMenu()->insertItem (KGlobal::iconLoader()->loadIcon("contents", KIcon::Small),
        i18n("kttsmgr &Handbook"), this, SLOT(helpSelected()));
    // Handbook not available yet.
    contextMenu()->setItemEnabled(id, false);
    id = contextMenu()->insertItem (KGlobal::iconLoader()->loadIcon("kttsd", KIcon::Small),
        i18n("&About kttsmgr"), this, SLOT(aboutSelected()));
        
    connect(this, SIGNAL(quitSelected()), this, SLOT(quitSelected()));
}

KttsMgrTray::~KttsMgrTray() { }

void KttsMgrTray::speakClipboardSelected()
{
     DCOPClient *client = kapp->dcopClient();
     if (!client->isApplicationRegistered("kttsd"))
     {
         QString error;
         if (KApplication::startServiceByName("KTTSD", QStringList(), &error))
             kdError() << "Starting KTTSD failed with message " << error << endl;
         else
             // Wait for KTTSD to start.
             while (!client->isApplicationRegistered("kttsd"));
     }
     speakClipboard();
}

void KttsMgrTray::aboutSelected()
{
    KAboutApplication aboutDlg(kapp->aboutData(), 0, "kttsmgraboutdlg", true);
    aboutDlg.exec();
}

void KttsMgrTray::helpSelected()
{
    kapp->invokeHelp();
}

void KttsMgrTray::quitSelected()
{
    kdDebug() << "Running KttsMgrTray::quitSelected" << endl;
    kapp->quit();
}

#include "kttsmgr.moc"
