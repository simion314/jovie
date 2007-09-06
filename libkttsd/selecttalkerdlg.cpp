/***************************************************** vim:set ts=4 sw=4 sts=4:
  Description: 
     A dialog for user to select a Talker, either by specifying
     selected Talker attributes, or by specifying all attributes
     of an existing configured Talker.

  Copyright:
  (C) 2005 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: Gary Cramblitt <garycramblitt@comcast.net>

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

// Qt includes.
#include <QtGui/QCheckBox>
#include <QtGui/QRadioButton>
#include <QtGui/QTableWidget>
#include <QtGui/QHeaderView>

// KDE includes.
#include <kdialog.h>
#include <kcombobox.h>
#include <kpushbutton.h>
#include <klineedit.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kservicetypetrader.h>

// KTTS includes.
#include "utils.h"
#include "talkerlistmodel.h"
#include "selecttalkerdlg.h"
#include "selectlanguagedlg.h"
#include "selecttalkerdlg.moc"

SelectTalkerDlg::SelectTalkerDlg(
    QWidget* parent,
    const char* name,
    const QString& caption,
    const QString& talkerCode,
    bool runningTalkers) :

    KDialog(parent)
{
    Q_UNUSED(name);
    setCaption(caption);
    setButtons(KDialog::Ok|KDialog::Cancel);
    m_widget = new Ui::SelectTalkerWidget();
    QWidget* w = new QWidget();
    m_widget->setupUi( w );
    m_talkerListModel = new TalkerListModel();
    m_widget->talkersView->setModel(m_talkerListModel);

    setMainWidget( w );
    m_runningTalkers = runningTalkers;
    m_talkerCode = TalkerCode( talkerCode, false );

    // Fill combo boxes.
    KComboBox* cb = m_widget->genderComboBox;
    cb->addItem( QString() );
    cb->addItem( TalkerCode::translatedGender("male") );
    cb->addItem( TalkerCode::translatedGender("female") );
    cb->addItem( TalkerCode::translatedGender("neutral") );

    cb = m_widget->volumeComboBox;
    cb->addItem( QString() );
    cb->addItem( TalkerCode::translatedVolume("medium") );
    cb->addItem( TalkerCode::translatedVolume("loud") );
    cb->addItem( TalkerCode::translatedVolume("soft") );

    cb = m_widget->rateComboBox;
    cb->addItem( QString() );
    cb->addItem( TalkerCode::translatedRate("medium") );
    cb->addItem( TalkerCode::translatedRate("fast") );
    cb->addItem( TalkerCode::translatedRate("slow") );

    cb = m_widget->synthComboBox;
    cb->addItem( QString() );
	KService::List offers = KServiceTypeTrader::self()->query("KTTSD/SynthPlugin");
    for(int i=0; i < offers.count() ; ++i)
        cb->addItem(offers[i]->name());

    // Fill List View with list of Talkers.
    KConfig config("kttsdrc");
    m_talkerListModel->loadTalkerCodesFromConfig(&config);

    // Set initial radio button state.
    if ( talkerCode.isEmpty() )
        m_widget->useDefaultRadioButton->setChecked(true);
    else
    {
        QString dummy;
        if (talkerCode == TalkerCode::normalizeTalkerCode(talkerCode, dummy))
            m_widget->useSpecificTalkerRadioButton->setChecked(true);
        else
            m_widget->useClosestMatchRadioButton->setChecked(true);
    }

    applyTalkerCodeToControls();
    enableDisableControls();

    connect(m_widget->useDefaultRadioButton, SIGNAL(clicked()),
            this, SLOT(configChanged()));
    connect(m_widget->useClosestMatchRadioButton, SIGNAL(clicked()),
            this, SLOT(configChanged()));
    connect(m_widget->useSpecificTalkerRadioButton, SIGNAL(clicked()),
            this, SLOT(configChanged()));

    connect(m_widget->languageBrowseButton, SIGNAL(clicked()),
            this, SLOT(slotLanguageBrowseButton_clicked()));

    connect(m_widget->synthComboBox, SIGNAL(activated(const QString&)),
            this, SLOT(configChanged()));
    connect(m_widget->genderComboBox, SIGNAL(activated(const QString&)),
            this, SLOT(configChanged()));
    connect(m_widget->volumeComboBox, SIGNAL(activated(const QString&)),
            this, SLOT(configChanged()));
    connect(m_widget->rateComboBox, SIGNAL(activated(const QString&)),
            this, SLOT(configChanged()));

    connect(m_widget->synthCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(configChanged()));
    connect(m_widget->genderCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(configChanged()));
    connect(m_widget->volumeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(configChanged()));
    connect(m_widget->rateCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(configChanged()));

    connect(m_widget->talkersView, SIGNAL(clicked()),
            this, SLOT(slotTalkersView_clicked()));

    m_widget->talkersView->setMinimumHeight( 120 );
}

SelectTalkerDlg::~SelectTalkerDlg() { }

QString SelectTalkerDlg::getSelectedTalkerCode()
{
    return m_talkerCode.getTalkerCode();
}

QString SelectTalkerDlg::getSelectedTranslatedDescription()
{
    return m_talkerCode.getTranslatedDescription();
}

void SelectTalkerDlg::slotLanguageBrowseButton_clicked()
{
    SelectLanguageDlg* dlg = new SelectLanguageDlg(
        this,
        i18n("Select Language"),
        QStringList(m_talkerCode.fullLanguageCode()),
        SelectLanguageDlg::SingleSelect,
        SelectLanguageDlg::BlankAllowed);
    int dlgResult = dlg->exec();
    if (dlgResult == QDialog::Accepted) {
        m_talkerCode.setFullLanguageCode(dlg->selectedLanguageCode());
        QString language = dlg->selectedLanguage();
        m_widget->languageLabel->setText(language);
        m_widget->languageCheckBox->setChecked(!language.isEmpty());
        configChanged();
    }
    delete dlg;
}

void SelectTalkerDlg::slotTalkersView_clicked()
{
    QModelIndex modelIndex = m_widget->talkersView->currentIndex();
    if (!modelIndex.isValid()) return;
    if (!m_widget->useSpecificTalkerRadioButton->isChecked()) return;
    configChanged();
}

void SelectTalkerDlg::configChanged()
{
    applyControlsToTalkerCode();
    applyTalkerCodeToControls();
    enableDisableControls();
}

void SelectTalkerDlg::applyTalkerCodeToControls()
{
    bool preferred = false;
    QString code = m_talkerCode.getTalkerCode();

    // TODO: Need to display translated Synth names.
    KttsUtils::setCbItemFromText(m_widget->synthComboBox,
        TalkerCode::stripPrefer( m_talkerCode.plugInName(), preferred) );
    m_widget->synthCheckBox->setEnabled( !m_talkerCode.plugInName().isEmpty() );
    m_widget->synthCheckBox->setChecked( preferred );

    KttsUtils::setCbItemFromText(m_widget->genderComboBox,
        TalkerCode::translatedGender( TalkerCode::stripPrefer( m_talkerCode.gender(), preferred ) ) );
    m_widget->genderCheckBox->setEnabled( !m_talkerCode.gender().isEmpty() );
    m_widget->genderCheckBox->setChecked( preferred );

    KttsUtils::setCbItemFromText(m_widget->volumeComboBox,
        TalkerCode::translatedVolume( TalkerCode::stripPrefer(  m_talkerCode.volume(), preferred ) ) );
    m_widget->volumeCheckBox->setEnabled( !m_talkerCode.volume().isEmpty() );
    m_widget->volumeCheckBox->setChecked( preferred );

    KttsUtils::setCbItemFromText(m_widget->rateComboBox,
        TalkerCode::translatedRate( TalkerCode::stripPrefer( m_talkerCode.rate(), preferred ) ) );
    m_widget->rateCheckBox->setEnabled( !m_talkerCode.rate().isEmpty() );
    m_widget->rateCheckBox->setChecked( preferred );

    // Select closest matching specific Talker.
    const TalkerCode::TalkerCodeList talkers = m_talkerListModel->datastore();
    int talkerIndex = TalkerCode::findClosestMatchingTalker(talkers, m_talkerCode.getTalkerCode(), false);
    m_widget->talkersView->setCurrentIndex(m_talkerListModel->index(talkerIndex, 0));
}

void SelectTalkerDlg::applyControlsToTalkerCode()
{
    if ( m_widget->useDefaultRadioButton->isChecked() )
        m_talkerCode = TalkerCode(QString(), false);
    else if ( m_widget->useClosestMatchRadioButton->isChecked() )
    {
        // Language already stored in talker code.

        QString t = m_widget->synthComboBox->currentText();
        if ( !t.isEmpty() && m_widget->synthCheckBox->isChecked() ) t.prepend("*");
        m_talkerCode.setPlugInName( t );

        t = TalkerCode::untranslatedGender( m_widget->genderComboBox->currentText() );
        if ( !t.isEmpty() && m_widget->genderCheckBox->isChecked() ) t.prepend("*");
        m_talkerCode.setGender( t );

        t = TalkerCode::untranslatedVolume( m_widget->volumeComboBox->currentText() );
        if ( !t.isEmpty() && m_widget->volumeCheckBox->isChecked() ) t.prepend("*");
        m_talkerCode.setVolume( t );

        t = TalkerCode::untranslatedRate( m_widget->rateComboBox->currentText() );
        if ( !t.isEmpty() && m_widget->rateCheckBox->isChecked() ) t.prepend("*");
        m_talkerCode.setRate( t );
    }
    else if (m_widget->useSpecificTalkerRadioButton->isChecked() )
    {
        QModelIndex talkerIndex = m_widget->talkersView->currentIndex();
        if (talkerIndex.isValid())
            m_talkerCode = m_talkerListModel->getRow(talkerIndex.row());
    }
}

void SelectTalkerDlg::enableDisableControls()
{
    bool enableClosest = ( m_widget->useClosestMatchRadioButton->isChecked() );
    bool enableSpecific = ( m_widget->useSpecificTalkerRadioButton->isChecked() );
    m_widget->closestMatchGroupBox->setEnabled( enableClosest );
    m_widget->talkersView->setEnabled( enableSpecific );
}
