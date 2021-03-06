/***************************************************** vim:set ts=4 sw=4 sts=4:
  Widget to configure Talker parameters including language, synthesizer, volume,
  rate, and pitch. Uses talkerwidget.ui.
  -------------------
  Copyright 2010 by Jeremy Whiting <jpwhiting@kde.org>
  -------------------

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

// KTTS includes.
#include "talkerwidget.h"

// Qt includes.
#include <QtGui/QTableWidgetItem>
#include <QtGui/QWidget>

// KDE includes.
#include <kcombobox.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include "kspeechinterface.h"

#include "ui_talkerwidget.h"

const int kLanguageColumn = 0;
const int kSynthesizerColumn = 1;
const int kVoiceNameColumn = 2;

TalkerWidget::TalkerWidget(QWidget* parent)
    : QWidget(parent)
{
    mUi = new Ui::TalkerWidget;
    mUi->setupUi(this);
    connect(mUi->nameEdit, SIGNAL(textEdited(QString)),
            this, SIGNAL(talkerChanged()));
    connect(mUi->AvailableTalkersTable, SIGNAL(itemSelectionChanged()),
            this, SIGNAL(talkerChanged()));
    connect(mUi->voiceComboBox, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(talkerChanged()));
    connect(mUi->volumeSlider, SIGNAL(valueChanged(int)),
            this, SIGNAL(talkerChanged()));
    connect(mUi->pitchSlider, SIGNAL(valueChanged(int)),
            this, SIGNAL(talkerChanged()));
    connect(mUi->speedSlider, SIGNAL(valueChanged(int)),
            this, SIGNAL(talkerChanged()));
    connect(mUi->punctuationComboBox,SIGNAL(currentIndexChanged(int)),
            this,SIGNAL(talkerChanged()));

    org::kde::KSpeech* kspeech = new OrgKdeKSpeechInterface(QLatin1String( "org.kde.KSpeech" ), QLatin1String( "/KSpeech" ), QDBusConnection::sessionBus());

    m_outputModules = kspeech->outputModules();

    mUi->AvailableTalkersTable->setSortingEnabled(false);

    QString fullLanguageCode = KGlobal::locale()->defaultLanguage();
    QString languageCode;
    QString countryCode;
    TalkerCode::splitFullLanguageCode(fullLanguageCode, languageCode, countryCode);

    QTableWidgetItem * defaultItem = 0;

    QStringList possibleTalkers = kspeech->getPossibleTalkers();
    foreach (const QString & talkerString, possibleTalkers)
    {
        TalkerCode code(talkerString);
        kDebug() << "Adding talker to talker widget with value " << talkerString;

        int rowcount = mUi->AvailableTalkersTable->rowCount();
        mUi->AvailableTalkersTable->setRowCount(rowcount + 1);

        // set the synthesizer item
        QTableWidgetItem * item = new QTableWidgetItem(code.outputModule());
        mUi->AvailableTalkersTable->setItem(rowcount, kSynthesizerColumn, item);

        // set the voice name item
        item = new QTableWidgetItem(code.voiceName());
        item->setToolTip(code.voiceName());
        mUi->AvailableTalkersTable->setItem(rowcount, kVoiceNameColumn, item);

        QString language = code.language();
        QString langName = TalkerCode::languageCodeToLanguage(language);
        if (language == languageCode)
        {
            defaultItem = item;
        }

        // set the language name item
        item = new QTableWidgetItem(langName.isEmpty() ? language : langName);
        item->setToolTip(language);
        mUi->AvailableTalkersTable->setItem(rowcount, kLanguageColumn, item);
    }

    // turn sorting on now that the table is populated
    mUi->AvailableTalkersTable->setSortingEnabled(true);

    // sort by language by default
    mUi->AvailableTalkersTable->sortItems(kLanguageColumn);

    if (defaultItem)
    {
        mUi->AvailableTalkersTable->setCurrentItem(defaultItem);
    }
}

TalkerWidget::~TalkerWidget()
{
    delete mUi;
}

void TalkerWidget::setNameReadOnly(bool value)
{
    mUi->nameLabel->setVisible(!value);
    mUi->nameEdit->setVisible(!value);
}

void TalkerWidget::setTalkerCode(const TalkerCode &talker)
{
    mUi->nameEdit->setText(talker.name());
    mUi->voiceComboBox->setCurrentIndex(talker.voiceType() - 1);
    mUi->volumeSlider->setValue(talker.volume());
    mUi->speedSlider->setValue(talker.rate());
    mUi->pitchSlider->setValue(talker.pitch());
    mUi->punctuationComboBox->setCurrentIndex(talker.punctuation());

    // Then we need to find the row in the availabletalkerstable that matches
    // the talker's language and output module.
    int rowCount = mUi->AvailableTalkersTable->rowCount();
    for (int i = 0; i < rowCount; ++i) {

        // We match the language with the language column's tooltip since it
        // contains the language code.
        if (mUi->AvailableTalkersTable->item(i, kLanguageColumn)->toolTip() ==
            talker.language() &&
            mUi->AvailableTalkersTable->item(i, kSynthesizerColumn)->text() ==
            talker.outputModule()) {
            mUi->AvailableTalkersTable->setCurrentCell(i, kLanguageColumn);
            break;
        }
    }
}

QString TalkerWidget::getName() const
{
    return mUi->nameEdit->text();
}

TalkerCode TalkerWidget::getTalkerCode() const
{
    TalkerCode retval;
    int row = mUi->AvailableTalkersTable->currentRow();
    if (row > 0 && row < mUi->AvailableTalkersTable->rowCount())
    {
        retval.setName(mUi->nameEdit->text());
        retval.setLanguage(mUi->AvailableTalkersTable->item(row, kLanguageColumn)->toolTip());
        retval.setVoiceType(mUi->voiceComboBox->currentIndex() + 1); // add 1 because the enumeration starts at 1
        retval.setVolume(mUi->volumeSlider->value());
        retval.setRate(mUi->speedSlider->value());
        retval.setPitch(mUi->pitchSlider->value());
        retval.setOutputModule(mUi->AvailableTalkersTable->item(row, kSynthesizerColumn)->text());
        retval.setPunctuation(mUi->punctuationComboBox->currentIndex());
    }
    return retval;
}

#include "talkerwidget.moc"

