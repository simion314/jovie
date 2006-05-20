/***************************************************** vim:set ts=4 sw=4 sts=4:
  Configuration for the Command Plug in
  -------------------
  Copyright : (C) 2002,2004 by Gunnar Schmi Dt and Gary Cramblitt
  -------------------
  Original author: Gunnar Schmi Dt <kmouth@schmi-dt.de>
  Current Maintainer: Gary Cramblitt <garycramblitt@comcast.net>
 ******************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 ***************************************************************************/

// Qt includes.
#include <QLayout>
#include <QCheckBox>
#include <QFile>
#include <QApplication>
#include <QTextCodec>
#include <QVBoxLayout>

// KDE includes.
#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <ktempfile.h>
#include <kstandarddirs.h>
#include <kprogressbar.h>
#include <kprogressdialog.h>
// KTTS includes.
#include <pluginconf.h>
#include <testplayer.h>

// Command Plugin includes.
#include "commandproc.h"
#include "commandconf.h"

/** Constructor */
CommandConf::CommandConf( QWidget* parent, const QStringList& /*args*/) :
    PlugInConf(parent, "commandconf")
{
    // kDebug() << "CommandConf::CommandConf: Running" << endl;
    m_commandProc = 0;
    m_progressDlg = 0;

    setupUi(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment (Qt::AlignTop);
    layout->addWidget(this);

    // Build codec list and fill combobox.
    m_codecList = PlugInProc::buildCodecList();
    characterCodingBox->clear();
    characterCodingBox->addItems(m_codecList);

    defaults();
    connect(characterCodingBox, SIGNAL(textChanged(const QString&)),
        this, SLOT(configChanged()));
    connect(characterCodingBox, SIGNAL(activated(const QString&)),
        this, SLOT(configChanged()));
    connect(stdInButton, SIGNAL(toggled(bool)),
        this, SLOT(configChanged()));
    connect(urlReq, SIGNAL(textChanged(const QString&)),
        this, SLOT(configChanged()));
    connect(commandTestButton, SIGNAL(clicked()),
        this, SLOT(slotCommandTest_clicked()));
}

/** Destructor */
CommandConf::~CommandConf()
{
    // kDebug() << "CommandConf::~CommandConf: Running" << endl;
    if (!m_waveFile.isNull()) QFile::remove(m_waveFile);
    delete m_commandProc;
    delete m_progressDlg;
}

void CommandConf::load(KConfig *config, const QString &configGroup) {
    // kDebug() << "CommandConf::load: Running" << endl;
    config->setGroup(configGroup);
    urlReq->setURL (config->readEntry("Command", "cat -"));
    stdInButton->setChecked(config->readEntry("StdIn", QVariant(false)).toBool());
    QString codecString = config->readEntry("Codec", "Local");
    m_languageCode = config->readEntry("LanguageCode", m_languageCode);
    int codec = PlugInProc::codecNameToListIndex(codecString, m_codecList);
    characterCodingBox->setCurrentIndex(codec);
}

void CommandConf::save(KConfig *config, const QString &configGroup) {
    // kDebug() << "CommandConf::save: Running" << endl;
    config->setGroup(configGroup);
    config->writeEntry("Command", urlReq->url());
    config->writeEntry("StdIn", stdInButton->isChecked());
    int codec = characterCodingBox->currentIndex();
    config->writeEntry("Codec", PlugInProc::codecIndexToCodecName(codec, m_codecList));
}

void CommandConf::defaults(){
    // kDebug() << "CommandConf::defaults: Running" << endl;
    urlReq->setURL("cat -");
    stdInButton->setChecked(false);
    urlReq->setShowLocalProtocol (false);
    characterCodingBox->setCurrentIndex(0);
}

void CommandConf::setDesiredLanguage(const QString &lang)
{
    m_languageCode = lang;
}

QString CommandConf::getTalkerCode()
{
    QString url = urlReq->url();
    if (!url.isEmpty())
    {
        // Must contain either text or file parameter, or StdIn checkbox must be checked,
        // otherwise, does nothing!
        if ((url.contains("%t") > 0) || (url.contains("%f") > 0) || stdInButton->isChecked())
        {
            return QString(
                "<voice lang=\"%1\" name=\"%2\" gender=\"%3\" />"
                "<prosody volume=\"%4\" rate=\"%5\" />"
                "<kttsd synthesizer=\"%6\" />")
                .arg(m_languageCode)
                .arg("fixed")
                .arg("neutral")
                .arg("medium")
                .arg("medium")
                .arg("Command");
        }
    }
    return QString();
}

void CommandConf::slotCommandTest_clicked()
{
    // kDebug() << "CommandConf::slotCommandTest_clicked(): " << endl;
    // If currently synthesizing, stop it.
    if (m_commandProc)
        m_commandProc->stopText();
    else
    {
        m_commandProc = new CommandProc();
        connect (m_commandProc, SIGNAL(stopped()), this, SLOT(slotSynthStopped()));
    }

    // Create a temp file name for the wave file.
    KTempFile tempFile (locateLocal("tmp", "commandplugin-"), ".wav");
    QString tmpWaveFile = tempFile.file()->fileName();
    tempFile.close();

    // Get test message in the language of the voice.
    QString testMsg = testMessage(m_languageCode);

    // Tell user to wait.
    m_progressDlg = new KProgressDialog(this,
        i18n("Testing"),
        i18n("Testing."),
        true);
    m_progressDlg->progressBar()->hide();
    m_progressDlg->setAllowCancel(true);

    // TODO: Do codec names contain non-ASCII characters?
    connect (m_commandProc, SIGNAL(synthFinished()), this, SLOT(slotSynthFinished()));
    m_commandProc->synth(
        testMsg,
        tmpWaveFile,
        urlReq->url(),
        stdInButton->isChecked(),
        PlugInProc::codecIndexToCodec(characterCodingBox->currentIndex(), m_codecList),
        m_languageCode);

    // Display progress dialog modally.  Processing continues when plugin signals synthFinished,
    // or if user clicks Cancel button.
    m_progressDlg->exec();
    disconnect (m_commandProc, SIGNAL(synthFinished()), this, SLOT(slotSynthFinished()));
    if (m_progressDlg->wasCancelled()) m_commandProc->stopText();
    delete m_progressDlg;
    m_progressDlg = 0;
}

void CommandConf::slotSynthFinished()
{
    // If user canceled, progress dialog is gone, so exit.
    if (!m_progressDlg)
    {
        m_commandProc->ackFinished();
        return;
    }
    // Hide the Cancel button so user can't cancel in the middle of playback.
    m_progressDlg->showCancelButton(false);
    // Get new wavefile name.
    m_waveFile = m_commandProc->getFilename();
    // Tell synth we're done.
    m_commandProc->ackFinished();
    // Play the wave file (possibly adjusting its Speed).
    // Player object deletes the wave file when done.
    if (m_player) m_player->play(m_waveFile);
    QFile::remove(m_waveFile);
    m_waveFile.clear();
    if (m_progressDlg) m_progressDlg->close();
}

void CommandConf::slotSynthStopped()
{
    // Clean up after canceling test.
    QString filename = m_commandProc->getFilename();
    if (!filename.isNull()) QFile::remove(filename);
}

#include "commandconf.moc"
