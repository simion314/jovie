/***************************************************** vim:set ts=4 sw=4 sts=4:
  KControl module for KTTSD configuration and Job Management
  -------------------
  Copyright : (C) 2002-2003 by José Pablo Ezequiel "Pupeno" Fernández
  Copyright : (C) 2004-2005 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: José Pablo Ezequiel "Pupeno" Fernández <pupeno@kde.org>
  Current Maintainer: Gary Cramblitt <garycramblitt@comcast.net>
 ******************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 ***************************************************************************/

// Note to programmers.  There is a subtle difference between a plugIn name and a 
// synthesizer name.  The latter is a translated name, for example, "Festival Interactivo",
// while the former is alway an English name, example "Festival Interactive".

// C++ includes.
#include <math.h>

// Qt includes.
#include <QWidget>
#include <QTabWidget>
#include <QCheckBox>
#include <QLayout>
#include <QRadioButton>
#include <QSlider>
#include <QLabel>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTextStream>

#include <Q3PopupMenu>

// KDE includes.
#include <dcopclient.h>
//#include <klistview.h>
#include <kparts/componentfactory.h>
#include <klineedit.h>
#include <kurlrequester.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kgenericfactory.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>
#include <kconfig.h>
#include <knuminput.h>
#include <kcombobox.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

// KTTS includes.
#include "talkercode.h"
#include "pluginconf.h"
#include "filterconf.h"
#include "testplayer.h"
#include "player.h"
#include "selecttalkerdlg.h"
#include "selectevent.h"
#include "notify.h"
#include "utils.h"

// KCMKttsMgr includes.
#include "kwidgetprobe.h"
#include "kcmkttsmgr.h"
#include "kcmkttsmgr.moc"

// Some constants.
// Defaults set when clicking Defaults button.
const bool embedInSysTrayCheckBoxValue = true;
const bool showMainWindowOnStartupCheckBoxValue = true;

const bool autostartMgrCheckBoxValue = true;
const bool autoexitMgrCheckBoxValue = true;

const bool notifyEnableCheckBoxValue = false;
const bool notifyExcludeEventsWithSoundCheckBoxValue = true;

const bool textPreMsgCheckValue = true;
const QString textPreMsgValue = i18n("Text interrupted. Message.");

const bool textPreSndCheckValue = false;
const QString textPreSndValue = "";

const bool textPostMsgCheckValue = true;
const QString textPostMsgValue = i18n("Resuming text.");

const bool textPostSndCheckValue = false;
const QString textPostSndValue = "";

const int timeBoxValue = 100;

const bool keepAudioCheckBoxValue = false;

// Make this a plug in.
typedef KGenericFactory<KCMKttsMgr, QWidget> KCMKttsMgrFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_kttsd, KCMKttsMgrFactory("kttsd") );

// ----------------------------------------------------------------------------

FilterListModel::FilterListModel(FilterList filters, QObject *parent)
    : QAbstractListModel(parent), m_filters(filters)
{
}

int FilterListModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_filters.count();
    else
        return 0;
}

int FilterListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QModelIndex FilterListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, 0);
    else
        return QModelIndex();
}

QModelIndex FilterListModel::parent(const QModelIndex &index ) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

QVariant FilterListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() < 0 || index.row() >= m_filters.count())
        return QVariant();

    if (index.column() < 0 || index.column() >= 2)
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        switch (index.column()) {
            case 0: return m_filters.at(index.row()).enabled; break;
            case 1: return m_filters.at(index.row()).userFilterName; break;
        }

    return QVariant();
}

Qt::ItemFlags FilterListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    switch (index.column()) {
        // TODO: ItemIsUserCheckable should be displaying the boolean as a checkbox, but instead
        // it displays as string "true" or "false".
        case 0: return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled |
                    Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEditable; break;
        case 1: return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable; break;
    }
    return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled;
}

QVariant FilterListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
            case 0: return "";
            case 1: return i18n("Filter");
        };
    return QVariant();
}

bool FilterListModel::removeRow(int row, const QModelIndex & parent)
{
    beginRemoveRows(parent, row, row);
    m_filters.removeAt(row);
    endRemoveRows();
    return true;
}

FilterItem FilterListModel::getRow(int row) const
{
    if (row < 0 || row >= rowCount()) return FilterItem();
    return m_filters[row];
}

bool FilterListModel::appendRow(FilterItem& filter)
{
    beginInsertRows(QModelIndex(), m_filters.count(), m_filters.count());
    m_filters.append(filter);
    endInsertRows();
    return true;
}

bool FilterListModel::updateRow(int row, FilterItem& filter)
{
    m_filters.replace(row, filter);
    emit dataChanged(index(row, 0, QModelIndex()), index(row, columnCount()-1, QModelIndex()));
    return true;
}

bool FilterListModel::swap(int i, int j)
{
    m_filters.swap(i, j);
    emit dataChanged(index(i, 0, QModelIndex()), index(j, columnCount()-1, QModelIndex()));
    return true;
}

void FilterListModel::clear()
{
    m_filters.clear();
    emit reset();
}

// ----------------------------------------------------------------------------

SbdFilterListModel::SbdFilterListModel(FilterList filters, QObject *parent)
    : FilterListModel(filters, parent)
{
}

int SbdFilterListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant SbdFilterListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() < 0 || index.row() >= m_filters.count())
        return QVariant();

    if (index.column() != 0)
        return QVariant();

    if (role == Qt::DisplayRole)
        return m_filters.at(index.row()).userFilterName;
    else 
        return QVariant();
}

Qt::ItemFlags SbdFilterListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant SbdFilterListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
            case 0: return i18n("Sentence Boundary Detector");
        };
    return QVariant();
}

// ----------------------------------------------------------------------------

/**
* Constructor.
*/
KCMKttsMgr::KCMKttsMgr(QWidget *parent, const char *name, const QStringList &) :
    DCOPStub("kttsd", "KSpeech"),
    DCOPObject("kcmkttsmgr_kspeechsink"),
    KCModule(KCMKttsMgrFactory::instance(), parent/*, name*/)
{
    Q_UNUSED(name);

    // kdDebug() << "KCMKttsMgr constructor running." << endl;

    // Initialize some variables.
    m_config = 0;
    m_jobMgrPart = 0;
    m_configDlg = 0;
    m_changed = false;
    m_suppressConfigChanged = false;

    // Add the KTTS Manager widget
    setupUi(this);

    // Connect Views to Models and set row selection mode.
    talkersView->setModel(&m_talkerListModel);
    filtersView->setModel(&m_filterListModel);
    sbdsView->setModel(&m_sbdFilterListModel);
    talkersView->setSelectionBehavior(QAbstractItemView::SelectRows);
    filtersView->setSelectionBehavior(QAbstractItemView::SelectRows);
    sbdsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    talkersView->setRootIsDecorated(false);
    filtersView->setRootIsDecorated(false);
    sbdsView->setRootIsDecorated(false);
    talkersView->setItemsExpandable(false);
    filtersView->setItemsExpandable(false);
    sbdsView->setItemsExpandable(false);

    // Give buttons icons.
    // Talkers tab.
    higherTalkerPriorityButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("up", KIcon::Small));
    lowerTalkerPriorityButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("down", KIcon::Small));
    removeTalkerButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("edittrash", KIcon::Small));
    configureTalkerButton->setIconSet(
        KGlobal::iconLoader()->loadIconSet("configure", KIcon::Small));

    // Filters tab.
    higherFilterPriorityButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("up", KIcon::Small));
    lowerFilterPriorityButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("down", KIcon::Small));
    removeFilterButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("edittrash", KIcon::Small));
    configureFilterButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("configure", KIcon::Small));

    // Notify tab.
    notifyListView->setColumnCount(7);
    notifyListView->setSelectionBehavior(QAbstractItemView::SelectRows);
    notifyListView->setSelectionMode(QAbstractItemView::SingleSelection);
    QStringList notifyListViewHeaders;
    notifyListViewHeaders << i18n("Application/Event");
    notifyListViewHeaders << i18n("Action");
    notifyListViewHeaders << i18n("Talker");
    notifyListView->setHeaderLabels( notifyListViewHeaders );
    QHeaderView* header = notifyListView->header();
    header->setSectionHidden( nlvcEventSrc, true );
    header->setSectionHidden( nlvcEvent, true );
    header->setSectionHidden( nlvcAction, true );
    header->setSectionHidden( nlvcTalker, true );
    header->setResizeMode( QHeaderView::Interactive );
    header->setStretchLastSection( true );
    notifyActionComboBox->clear();
    for (int ndx = 0; ndx < NotifyAction::count(); ++ndx)
        notifyActionComboBox->insertItem( NotifyAction::actionDisplayName( ndx ) );
    notifyPresentComboBox->clear();
    for (int ndx = 0; ndx < NotifyPresent::count(); ++ndx)
        notifyPresentComboBox->insertItem( NotifyPresent::presentDisplayName( ndx ) );

    notifyRemoveButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("edittrash", KIcon::Small));
    notifyTestButton->setIconSet(
            KGlobal::iconLoader()->loadIconSet("speak", KIcon::Small));

    sinkComboBox->setEditable(false);
    pcmComboBox->setEditable(false);

    // Construct a popup menu for the Sentence Boundary Detector buttons on Filter tab.
    m_sbdPopmenu = new Q3PopupMenu( this, "SbdPopupMenu" );
    m_sbdPopmenu->insertItem( i18n("&Edit..."), this, SLOT(slot_configureSbdFilter()), 0, sbdBtnEdit );
    m_sbdPopmenu->insertItem( KGlobal::iconLoader()->loadIconSet("up", KIcon::Small),
                              i18n("U&p"), this, SLOT(slot_higherSbdFilterPriority()), 0, sbdBtnUp );
    m_sbdPopmenu->insertItem( KGlobal::iconLoader()->loadIconSet("down", KIcon::Small),
                              i18n("Do&wn"), this, SLOT(slot_lowerSbdFilterPriority()), 0, sbdBtnDown );
    m_sbdPopmenu->insertItem( i18n("&Add..."), this, SLOT(slot_addSbdFilter()), 0, sbdBtnAdd );
    m_sbdPopmenu->insertItem( i18n("&Remove"), this, SLOT(slot_removeSbdFilter()), 0, sbdBtnRemove );
    sbdButton->setPopup( m_sbdPopmenu );

    // If aRts is available, enable its radio button.
    // Determine if available by loading its plugin.  If it fails, not available.
    TestPlayer* testPlayer = new TestPlayer();
    Player* player = testPlayer->createPlayerObject(0);
    if (player)
        artsRadioButton->setEnabled(true);
    else
        artsRadioButton->setEnabled(false);
    delete player;
    delete testPlayer;

    // If GStreamer is available, enable its radio button.
    // Determine if available by loading its plugin.  If it fails, not available.
    testPlayer = new TestPlayer();
    player = testPlayer->createPlayerObject(1);
    if (player)
    {
        gstreamerRadioButton->setEnabled(true);
        sinkLabel->setEnabled(true);
        sinkComboBox->setEnabled(true);
        QStringList sinkList = player->getPluginList("Sink/Audio");
        // kdDebug() << "KCMKttsMgr::KCMKttsMgr: GStreamer Sink List = " << sinkList << endl;
        sinkComboBox->clear();
        sinkComboBox->insertStringList(sinkList);
    }
    delete player;
    delete testPlayer;

    // If ALSA is available, enable its radio button.
    // Determine if available by loading its plugin.  If it fails, not available.
    testPlayer = new TestPlayer();
    player = testPlayer->createPlayerObject(2);
    if (player)
    {
        alsaRadioButton->setEnabled(true);
        pcmLabel->setEnabled(true);
        pcmComboBox->setEnabled(true);
        QStringList pcmList = player->getPluginList("");
        kdDebug() << "KCMKttsMgr::KCMKttsMgr: ALSA pcmList = " << pcmList << endl;
        pcmComboBox->clear();
        pcmComboBox->insertStringList(pcmList);
    }
    delete player;
    delete testPlayer;

    // If aKode is available, enable its radio button.
    // Determine if available by loading its plugin.  If it fails, not available.
    testPlayer = new TestPlayer();
    player = testPlayer->createPlayerObject(3);
    if (player)
    {
        akodeRadioButton->setEnabled(true);
        akodeSinkLabel->setEnabled(true);
        akodeComboBox->setEnabled(true);
        QStringList pcmList = player->getPluginList("");
        kdDebug() << "KCMKttsMgr::KCMKttsMgr: aKode Sink List = " << pcmList << endl;
        akodeComboBox->clear();
        akodeComboBox->insertStringList(pcmList);
    }
    delete player;
    delete testPlayer;

    // Set up Keep Audio Path KURLRequestor.
    keepAudioPath->setMode(KFile::Directory);
    keepAudioPath->setURL(locateLocal("data", "kttsd/audio/"));

    // Object for the KTTSD configuration.
    m_config = new KConfig("kttsdrc");

    // Load configuration.
    load();

    // Connect the signals from the KCMKtssMgrWidget to this class.

    // Talker tab.
    connect(addTalkerButton, SIGNAL(clicked()),
            this, SLOT(slot_addTalker()));
    connect(higherTalkerPriorityButton, SIGNAL(clicked()),
            this, SLOT(slot_higherTalkerPriority()));
    connect(lowerTalkerPriorityButton, SIGNAL(clicked()),
            this, SLOT(slot_lowerTalkerPriority()));
    connect(removeTalkerButton, SIGNAL(clicked()),
            this, SLOT(slot_removeTalker()));
    connect(configureTalkerButton, SIGNAL(clicked()),
            this, SLOT(slot_configureTalker()));
    connect(talkersView, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(updateTalkerButtons()));

    // Filter tab.
    connect(addFilterButton, SIGNAL(clicked()),
            this, SLOT(slot_addNormalFilter()));
    connect(higherFilterPriorityButton, SIGNAL(clicked()),
            this, SLOT(slot_higherNormalFilterPriority()));
    connect(lowerFilterPriorityButton, SIGNAL(clicked()),
            this, SLOT(slot_lowerNormalFilterPriority()));
    connect(removeFilterButton, SIGNAL(clicked()),
            this, SLOT(slot_removeNormalFilter()));
    connect(configureFilterButton, SIGNAL(clicked()),
            this, SLOT(slot_configureNormalFilter()));
    connect(filtersView, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(updateFilterButtons()));
    //connect(filtersView, SIGNAL(stateChanged()),
    //        this, SLOT(configChanged()));
    connect(sbdsView, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(updateSbdButtons()));

    // Audio tab.
    connect(gstreamerRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(slotGstreamerRadioButton_toggled(bool)));
    connect(alsaRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(slotAlsaRadioButton_toggled(bool)));
    connect(akodeRadioButton, SIGNAL(toggled(bool)),
            this, SLOT(slotAkodeRadioButton_toggled(bool)));
    connect(timeBox, SIGNAL(valueChanged(int)),
            this, SLOT(timeBox_valueChanged(int)));
    connect(timeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(timeSlider_valueChanged(int)));
    connect(timeBox, SIGNAL(valueChanged(int)), this, SLOT(configChanged()));
    connect(timeSlider, SIGNAL(valueChanged(int)), this, SLOT(configChanged()));
    connect(keepAudioCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(keepAudioCheckBox_toggled(bool)));
    connect(keepAudioPath, SIGNAL(textChanged(const QString&)),
            this, SLOT(configChanged()));

    // General tab.
    connect(enableKttsdCheckBox, SIGNAL(toggled(bool)),
            SLOT(enableKttsdToggled(bool)));
    connect(screenReaderCheckBox, SIGNAL(toggled(bool)),
            SLOT(enableScreenReaderToggled(bool)));

    // Notify tab.
    connect(notifyEnableCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotNotifyEnableCheckBox_toggled(bool)));
    connect(notifyExcludeEventsWithSoundCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(configChanged()));
    connect(notifyAddButton, SIGNAL(clicked()),
            this, SLOT(slotNotifyAddButton_clicked()));
    connect(notifyRemoveButton, SIGNAL(clicked()),
            this, SLOT(slotNotifyRemoveButton_clicked()));
    connect(notifyClearButton, SIGNAL(clicked()),
            this, SLOT(slotNotifyClearButton_clicked()));
    connect(notifyLoadButton, SIGNAL(clicked()),
            this, SLOT(slotNotifyLoadButton_clicked()));
    connect(notifySaveButton, SIGNAL(clicked()),
            this, SLOT(slotNotifySaveButton_clicked()));
    connect(notifyListView, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(slotNotifyListView_currentItemChanged()));
    connect(notifyPresentComboBox, SIGNAL(activated(int)),
            this, SLOT(slotNotifyPresentComboBox_activated(int)));
    connect(notifyActionComboBox, SIGNAL(activated(int)),
            this, SLOT(slotNotifyActionComboBox_activated(int)));
    connect(notifyTestButton, SIGNAL(clicked()),
            this, SLOT(slotNotifyTestButton_clicked()));
    connect(notifyMsgLineEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotNotifyMsgLineEdit_textChanged(const QString&)));
    connect(notifyTalkerButton, SIGNAL(clicked()),
            this, SLOT(slotNotifyTalkerButton_clicked()));

    // Others.
    connect(this, SIGNAL( configChanged() ),
            this, SLOT( configChanged() ) );
    connect(mainTab, SIGNAL(currentChanged(QWidget*)),
            this, SLOT(slotTabChanged()));

    // Connect KTTSD DCOP signals to our slots.
    if (!connectDCOPSignal("kttsd", "KSpeech",
        "kttsdStarted()",
        "kttsdStarted()",
        false)) kdDebug() << "connectDCOPSignal failed" << endl;
    connectDCOPSignal("kttsd", "KSpeech",
        "kttsdExiting()",
        "kttsdExiting()",
        false);

    // See if KTTSD is already running, and if so, create jobs tab.
    if (kapp->dcopClient()->isApplicationRegistered("kttsd"))
        kttsdStarted();
    else
        // Start KTTSD if check box is checked.
        enableKttsdToggled(enableKttsdCheckBox->isChecked());

    // Switch to Talkers tab if none configured,
    // otherwise switch to Jobs tab if it is active.
    if (m_talkerListModel.rowCount() == 0)
        mainTab->setCurrentPage(wpTalkers);
    else if (enableKttsdCheckBox->isChecked())
        mainTab->setCurrentPage(wpJobs);
} 

/**
* Destructor.
*/
KCMKttsMgr::~KCMKttsMgr(){
    // kdDebug() << "KCMKttsMgr::~KCMKttsMgr: Running" << endl;
    delete m_config;
}

/**
* This method is invoked whenever the module should read its 
* configuration (most of the times from a config file) and update the 
* user interface. This happens when the user clicks the "Reset" button in
* the control center, to undo all of his changes and restore the currently
* valid settings. NOTE that this is not called after the modules is loaded,
* so you probably want to call this method in the constructor.
*/
void KCMKttsMgr::load()
{
    // kdDebug() << "KCMKttsMgr::load: Running" << endl;

    m_changed = false;
    // Don't emit changed() signal while loading.
    m_suppressConfigChanged = true;

    // Set the group general for the configuration of kttsd itself (no plug ins)
    m_config->setGroup("General");

    // Load the configuration of the text interruption messages and sound
    textPreMsgCheck->setChecked(m_config->readBoolEntry("TextPreMsgEnabled", textPreMsgCheckValue));
    textPreMsg->setText(m_config->readEntry("TextPreMsg", textPreMsgValue));
    textPreMsg->setEnabled(textPreMsgCheck->isChecked());

    textPreSndCheck->setChecked(m_config->readBoolEntry("TextPreSndEnabled", textPreSndCheckValue));
    textPreSnd->setURL(m_config->readEntry("TextPreSnd", textPreSndValue));
    textPreSnd->setEnabled(textPreSndCheck->isChecked());

    textPostMsgCheck->setChecked(m_config->readBoolEntry("TextPostMsgEnabled", textPostMsgCheckValue));
    textPostMsg->setText(m_config->readEntry("TextPostMsg", textPostMsgValue));
    textPostMsg->setEnabled(textPostMsgCheck->isChecked());

    textPostSndCheck->setChecked(m_config->readBoolEntry("TextPostSndEnabled", textPostSndCheckValue));
    textPostSnd->setURL(m_config->readEntry("TextPostSnd", textPostSndValue));
    textPostSnd->setEnabled(textPostSndCheck->isChecked());

    // Overall settings.
    embedInSysTrayCheckBox->setChecked(m_config->readBoolEntry("EmbedInSysTray",
        embedInSysTrayCheckBox->isChecked()));
    showMainWindowOnStartupCheckBox->setChecked(m_config->readBoolEntry(
        "ShowMainWindowOnStartup", showMainWindowOnStartupCheckBox->isChecked()));
    showMainWindowOnStartupCheckBox->setEnabled(
        embedInSysTrayCheckBox->isChecked());

    enableKttsdCheckBox->setChecked(m_config->readBoolEntry("EnableKttsd",
        enableKttsdCheckBox->isChecked()));

    autostartMgrCheckBox->setChecked(m_config->readBoolEntry("AutoStartManager", true));
    autoexitMgrCheckBox->setChecked(m_config->readBoolEntry("AutoExitManager", true));

    // Notification settings.
    notifyEnableCheckBox->setChecked(m_config->readBoolEntry("Notify",
        notifyEnableCheckBox->isChecked()));
    notifyExcludeEventsWithSoundCheckBox->setChecked(
        m_config->readBoolEntry("ExcludeEventsWithSound",
        notifyExcludeEventsWithSoundCheckBox->isChecked()));
    slotNotifyClearButton_clicked();
    loadNotifyEventsFromFile( locateLocal("config", "kttsd_notifyevents.xml"), true );
    slotNotifyEnableCheckBox_toggled( notifyEnableCheckBox->isChecked() );
    // Auto-expand and position on the Default item.
    QTreeWidgetItem* item = findTreeWidgetItem( notifyListView, "default", nlvcEventSrc );
    if ( item )
        if ( item->childCount() > 0 ) {
            notifyListView->setItemExpanded( item, true );
            item = item->child(0);
        }
    if ( item )
        notifyListView->scrollToItem( item );

    // Audio Output.
    int audioOutputMethod = 0;
    if (gstreamerRadioButton->isChecked()) audioOutputMethod = 1;
    if (alsaRadioButton->isChecked()) audioOutputMethod = 2;
    if (akodeRadioButton->isChecked()) audioOutputMethod = 3;
    audioOutputMethod = m_config->readNumEntry("AudioOutputMethod", audioOutputMethod);
    switch (audioOutputMethod)
    {
        case 0:
            artsRadioButton->setChecked(true);
            break;
        case 1:
            gstreamerRadioButton->setChecked(true);
            break;
        case 2:
            alsaRadioButton->setChecked(true);
            break;
        case 3:
            akodeRadioButton->setChecked(true);
            break;
    }
    timeBox->setValue(m_config->readNumEntry("AudioStretchFactor", timeBoxValue));
    timeBox_valueChanged(timeBox->value());
    keepAudioCheckBox->setChecked(
        m_config->readBoolEntry("KeepAudio",                                             keepAudioCheckBox->isChecked()));
    keepAudioPath->setURL(
        m_config->readEntry("KeepAudioPath",
        keepAudioPath->url()));
    keepAudioPath->setEnabled(keepAudioCheckBox->isChecked());

    // Last filter ID.  Used to generate a new ID for an added filter.
    m_lastFilterID = 0;

    // Load existing Talkers into Talker List.
    m_talkerListModel.loadTalkerCodesFromConfig(m_config);

    // Last talker ID.  Used to generate a new ID for an added talker.
    m_lastTalkerID = m_talkerListModel.highestTalkerId();

    // Dictionary mapping languages to language codes.
    m_languagesToCodes.clear();
    for (int i = 0; i < m_talkerListModel.rowCount(); ++i) {
        QString fullLanguageCode = m_talkerListModel.getRow(i).fullLanguageCode();
        QString language = TalkerCode::languageCodeToLanguage(fullLanguageCode);
        m_languagesToCodes[language] = fullLanguageCode;
    }

    // Query for all the KCMKTTSD SynthPlugins and store the list in offers.
    KTrader::OfferList offers = KTrader::self()->query("KTTSD/SynthPlugin");

    // Iterate thru the possible plug ins getting their language support codes.
    for(int i=0; i < offers.count() ; ++i)
    {
        QString synthName = offers[i]->name();
        QStringList languageCodes = offers[i]->property("X-KDE-Languages").toStringList();
        // Add language codes to the language-to-language code map.
        QStringList::ConstIterator endLanguages(languageCodes.constEnd());
        for( QStringList::ConstIterator it = languageCodes.constBegin(); it != endLanguages; ++it )
        {
            QString language = TalkerCode::languageCodeToLanguage(*it);
            m_languagesToCodes[language] = *it;
        }

        // All plugins support "Other".
        // TODO: Eventually, this should not be necessary, since all plugins will know
        // the languages they support and report them in call to getSupportedLanguages().
        if (!languageCodes.contains("other")) languageCodes.append("other");

        // Add supported language codes to synthesizer-to-language map.
        m_synthToLangMap[synthName] = languageCodes;
    }

    // Add "Other" language.
    m_languagesToCodes[i18n("Other")] = "other";

    // Load Filters.
    m_filterListModel.clear();
    m_sbdFilterListModel.clear();
    m_config->setGroup("General");
    QStringList filterIDsList = m_config->readListEntry("FilterIDs", ',');
    // kdDebug() << "KCMKttsMgr::load: FilterIDs = " << filterIDsList << endl;
    if (!filterIDsList.isEmpty())
    {
        QStringList::ConstIterator itEnd = filterIDsList.constEnd();
        for (QStringList::ConstIterator it = filterIDsList.constBegin(); it != itEnd; ++it)
        {
            QString filterID = *it;
            // kdDebug() << "KCMKttsMgr::load: filterID = " << filterID << endl;
            m_config->setGroup("Filter_" + filterID);
            QString desktopEntryName = m_config->readEntry("DesktopEntryName", QString::null);
            // If a DesktopEntryName is not in the config file, it was configured before
            // we started using them, when we stored translated plugin names instead.
            // Try to convert the translated plugin name to a DesktopEntryName.
            // DesktopEntryNames are better because user can change their desktop language
            // and DesktopEntryName won't change.
            QString filterPlugInName;
            filterPlugInName = FilterDesktopEntryNameToName(desktopEntryName);
            if (!filterPlugInName.isEmpty())
            {
                FilterItem fi;
                fi.plugInName = filterPlugInName;
                fi.desktopEntryName = desktopEntryName;
                fi.userFilterName = m_config->readEntry("UserFilterName", filterPlugInName);
                fi.multiInstance = m_config->readBoolEntry("MultiInstance", false);
                fi.enabled = m_config->readBoolEntry("Enabled", false);
                // Determine if this filter is a Sentence Boundary Detector (SBD).
                bool isSbd = m_config->readBoolEntry("IsSBD", false);
                if (isSbd)
                    m_sbdFilterListModel.appendRow(fi);
                else
                    m_filterListModel.appendRow(fi);
                if (filterID.toInt() > m_lastFilterID) m_lastFilterID = filterID.toInt();
            }
        }
    }

    // Add at least one unchecked instance of each available filter plugin if there is
    // not already at least one instance and the filter can autoconfig itself.
    offers = KTrader::self()->query("KTTSD/FilterPlugin");
    for (int i=0; i < offers.count() ; ++i)
    {
        QString filterPlugInName = offers[i]->name();
        QString desktopEntryName = FilterNameToDesktopEntryName(filterPlugInName);
        if (!desktopEntryName.isEmpty() && (countFilterPlugins(filterPlugInName) == 0))
        {
            // Must load plugin to determine if it supports multiple instances
            // and to see if it can autoconfigure itself.
            KttsFilterConf* filterPlugIn = loadFilterPlugin(filterPlugInName);
            if (filterPlugIn)
            {
                ++m_lastFilterID;
                QString filterID = QString::number(m_lastFilterID);
                QString groupName = "Filter_" + filterID;
                filterPlugIn->load(m_config, groupName);
                QString userFilterName = filterPlugIn->userPlugInName();
                if (!userFilterName.isEmpty())
                {
                    kdDebug() << "KCMKttsMgr::load: auto-configuring filter " << userFilterName << endl;
                    bool multiInstance = filterPlugIn->supportsMultiInstance();
                    FilterItem fi;
                    fi.id = filterID;
                    fi.userFilterName = userFilterName;
                    fi.plugInName = filterPlugInName;
                    fi.desktopEntryName = desktopEntryName;
                    fi.enabled = true;
                    fi.multiInstance = multiInstance;
                    // Determine if plugin is an SBD filter.
                    bool isSbd = filterPlugIn->isSBD();
                    if (isSbd)
                        m_sbdFilterListModel.appendRow(fi);
                    else
                        m_filterListModel.appendRow(fi);
                    m_config->setGroup(groupName);
                    filterPlugIn->save(m_config, groupName);
                    m_config->setGroup(groupName);
                    m_config->writeEntry("DesktopEntryName", desktopEntryName);
                    m_config->writeEntry("UserFilterName", userFilterName);
                    m_config->writeEntry("Enabled", isSbd);
                    m_config->writeEntry("MultiInstance", multiInstance);
                    m_config->writeEntry("IsSBD", isSbd);
                    filterIDsList.append(filterID);
                } else m_lastFilterID--;
            } else
                kdDebug() << "KCMKttsMgr::load: Unable to load filter plugin " << filterPlugInName 
                    << " DesktopEntryName " << desktopEntryName << endl;
            delete filterPlugIn;
        }
    }
    // Rewrite list of FilterIDs in case we added any.
    QString filterIDs = filterIDsList.join(",");
    m_config->setGroup("General");
    m_config->writeEntry("FilterIDs", filterIDs);
    m_config->sync();

    // Uncheck and disable KTTSD checkbox if no Talkers are configured.
    if (m_talkerListModel.rowCount() == 0)
    {
        enableKttsdCheckBox->setChecked(false);
        enableKttsdCheckBox->setEnabled(false);
        enableKttsdToggled(false);
    }

    // Enable ShowMainWindowOnStartup checkbox based on EmbedInSysTray checkbox.
    showMainWindowOnStartupCheckBox->setEnabled(
        embedInSysTrayCheckBox->isChecked());

    // GStreamer settings.
    m_config->setGroup("GStreamerPlayer");
    KttsUtils::setCbItemFromText(sinkComboBox, m_config->readEntry("SinkName", "osssink"));

    // ALSA settings.
    m_config->setGroup("ALSAPlayer");
    KttsUtils::setCbItemFromText(pcmComboBox, m_config->readEntry("PcmName", "default"));

    // aKode settings.
    m_config->setGroup("aKodePlayer");
    KttsUtils::setCbItemFromText(akodeComboBox, m_config->readEntry("SinkName", "auto"));

    // Update controls based on new states.
    slotNotifyListView_currentItemChanged();
    updateTalkerButtons();
    updateFilterButtons();
    updateSbdButtons();
    slotGstreamerRadioButton_toggled(gstreamerRadioButton->isChecked());
    slotAlsaRadioButton_toggled(alsaRadioButton->isChecked());
    slotAkodeRadioButton_toggled(akodeRadioButton->isChecked());

    m_changed = false;
    m_suppressConfigChanged = false;
}

/**
* This function gets called when the user wants to save the settings in 
* the user interface, updating the config files or wherever the 
* configuration is stored. The method is called when the user clicks "Apply" 
* or "Ok".
*/
void KCMKttsMgr::save()
{
    // kdDebug() << "KCMKttsMgr::save: Running" << endl;
    m_changed = false;

    // Clean up config.
    m_config->deleteGroup("General");

    // Set the group general for the configuration of kttsd itself (no plug ins)
    m_config->setGroup("General");

    // Set text interrumption messages and paths
    m_config->writeEntry("TextPreMsgEnabled", textPreMsgCheck->isChecked());
    m_config->writeEntry("TextPreMsg", textPreMsg->text());

    m_config->writeEntry("TextPreSndEnabled", textPreSndCheck->isChecked()); 
    m_config->writeEntry("TextPreSnd", PlugInConf::realFilePath(textPreSnd->url()));

    m_config->writeEntry("TextPostMsgEnabled", textPostMsgCheck->isChecked());
    m_config->writeEntry("TextPostMsg", textPostMsg->text());

    m_config->writeEntry("TextPostSndEnabled", textPostSndCheck->isChecked());
    m_config->writeEntry("TextPostSnd", PlugInConf::realFilePath(textPostSnd->url()));

    // Overall settings.
    m_config->writeEntry("EmbedInSysTray", embedInSysTrayCheckBox->isChecked());
    m_config->writeEntry("ShowMainWindowOnStartup",
        showMainWindowOnStartupCheckBox->isChecked());
    m_config->writeEntry("AutoStartManager", autostartMgrCheckBox->isChecked());
    m_config->writeEntry("AutoExitManager", autoexitMgrCheckBox->isChecked());

    // Uncheck and disable KTTSD checkbox if no Talkers are configured.
    // Enable checkbox if at least one Talker is configured.
    bool enableKttsdWasToggled = false;
    if (m_talkerListModel.rowCount() == 0)
    {
        enableKttsdWasToggled = enableKttsdCheckBox->isChecked();
        enableKttsdCheckBox->setChecked(false);
        enableKttsdCheckBox->setEnabled(false);
        // Might as well zero LastTalkerID as well.
        m_lastTalkerID = 0;
    }
    else
        enableKttsdCheckBox->setEnabled(true);

    m_config->writeEntry("EnableKttsd", enableKttsdCheckBox->isChecked());

    // Notification settings.
    m_config->writeEntry("Notify", notifyEnableCheckBox->isChecked());
    m_config->writeEntry("ExcludeEventsWithSound",
        notifyExcludeEventsWithSoundCheckBox->isChecked());
    saveNotifyEventsToFile( locateLocal("config", "kttsd_notifyevents.xml") );

    // Audio Output.
    int audioOutputMethod = 0;
    if (gstreamerRadioButton->isChecked()) audioOutputMethod = 1;
    if (alsaRadioButton->isChecked()) audioOutputMethod = 2;
    if (akodeRadioButton->isChecked()) audioOutputMethod = 3;
    m_config->writeEntry("AudioOutputMethod", audioOutputMethod);
    m_config->writeEntry("AudioStretchFactor", timeBox->value());
    m_config->writeEntry("KeepAudio", keepAudioCheckBox->isChecked());
    m_config->writeEntry("KeepAudioPath", keepAudioPath->url());

    // Get ordered list of all talker IDs.
    QStringList talkerIDsList;
    for (int i = 0; i < m_talkerListModel.rowCount(); ++i)
        talkerIDsList.append(m_talkerListModel.getRow(i).id());
    QString talkerIDs = talkerIDsList.join(",");
    m_config->writeEntry("TalkerIDs", talkerIDs);

    // Erase obsolete Talker_nn sections.
    QStringList groupList = m_config->groupList();
    int groupListCount = groupList.count();
    for (int groupNdx = 0; groupNdx < groupListCount; ++groupNdx)
    {
        QString groupName = groupList[groupNdx];
        if (groupName.left(7) == "Talker_")
        {
            QString groupTalkerID = groupName.mid(7);
            if (!talkerIDsList.contains(groupTalkerID)) m_config->deleteGroup(groupName);
        }
    }

    // Get ordered list of all filter IDs.  Record enabled state of each filter.
    QStringList filterIDsList;
    for (int i = 0; i < m_filterListModel.rowCount(); ++i) {
        FilterItem fi = m_filterListModel.getRow(i);
        filterIDsList.append(fi.id);
        m_config->setGroup("Filter_" + fi.id);
        m_config->writeEntry("Enabled", fi.enabled);
        m_config->writeEntry("IsSBD", false);
    }
    for (int i = 0; i < m_sbdFilterListModel.rowCount(); ++i) {
        FilterItem fi = m_sbdFilterListModel.getRow(i);
        filterIDsList.append(fi.id);
        m_config->setGroup("Filter_" + fi.id);
        m_config->writeEntry("Enabled", fi.enabled);
        m_config->writeEntry("IsSBD", true);
    }
    QString filterIDs = filterIDsList.join(",");
    m_config->setGroup("General");
    m_config->writeEntry("FilterIDs", filterIDs);

    // Erase obsolete Filter_nn sections.
    for (int groupNdx = 0; groupNdx < groupListCount; ++groupNdx)
    {
        QString groupName = groupList[groupNdx];
        if (groupName.left(7) == "Filter_")
        {
            QString groupFilterID = groupName.mid(7);
            if (!filterIDsList.contains(groupFilterID)) m_config->deleteGroup(groupName);
        }
    }

    // GStreamer settings.
    m_config->setGroup("GStreamerPlayer");
    m_config->writeEntry("SinkName", sinkComboBox->currentText());

    // ALSA settings.
    m_config->setGroup("ALSAPlayer");
    m_config->writeEntry("PcmName", pcmComboBox->currentText());

    // aKode settings.
    m_config->setGroup("aKodePlayer");
    m_config->writeEntry("SinkName", akodeComboBox->currentText());

    m_config->sync();

    // If we automatically unchecked the Enable KTTSD checkbox, stop KTTSD.
    if (enableKttsdWasToggled)
        enableKttsdToggled(false);
    else
    {
        // If KTTSD is running, reinitialize it.
        DCOPClient *client = kapp->dcopClient();
        bool kttsdRunning = (client->isApplicationRegistered("kttsd"));
        if (kttsdRunning)
        {
            kdDebug() << "Restarting KTTSD" << endl;
            QByteArray data;
            client->send("kttsd", "KSpeech", "reinit()", data);
        }
    }
}

void KCMKttsMgr::enableScreenReaderToggled(bool enabled)
{
    if (enabled)
        m_screenReader = new KWidgetProbe(this, "ktts_widgetprobe");
    else {
        delete m_screenReader;
        m_screenReader = 0;
    }
}

void KCMKttsMgr::slotTabChanged()
{
    setButtons(buttons());
    int currentPageIndex = mainTab->currentPageIndex();
    if (currentPageIndex == wpJobs)
    {
        if (m_changed)
        {
            KMessageBox::information(this,
                i18n("You have made changes to the configuration but have not saved them yet.  "
                     "Click Apply to save the changes or Cancel to abandon the changes."));
        }
    }
}

/**
* This function is called to set the settings in the module to sensible
* default values. It gets called when hitting the "Default" button. The 
* default values should probably be the same as the ones the application 
* uses when started without a config file.
*/
void KCMKttsMgr::defaults() {
    // kdDebug() << "Running: KCMKttsMgr::defaults: Running"<< endl;

    int currentPageIndex = mainTab->currentPageIndex();
    bool changed = false;
    switch (currentPageIndex)
    {
        case wpGeneral:
            if (embedInSysTrayCheckBox->isChecked() != embedInSysTrayCheckBoxValue)
            {
                changed = true;
                embedInSysTrayCheckBox->setChecked(embedInSysTrayCheckBoxValue);
            }
            if (showMainWindowOnStartupCheckBox->isChecked() !=
                showMainWindowOnStartupCheckBoxValue)
            {
                changed = true;
                showMainWindowOnStartupCheckBox->setChecked(
                    showMainWindowOnStartupCheckBoxValue);
            }
            if (autostartMgrCheckBox->isChecked() != autostartMgrCheckBoxValue)
            {
                changed = true;
                autostartMgrCheckBox->setChecked(
                    autostartMgrCheckBoxValue);
            }
            if (autoexitMgrCheckBox->isChecked() != autoexitMgrCheckBoxValue)
            {
                changed = true;
                autoexitMgrCheckBox->setChecked(
                    autoexitMgrCheckBoxValue);
            }
            break;

        case wpNotify:
            if (notifyEnableCheckBox->isChecked() != notifyEnableCheckBoxValue)
            {
                changed = true;
                notifyEnableCheckBox->setChecked(notifyEnableCheckBoxValue);
                notifyGroup->setChecked( notifyEnableCheckBoxValue );
            }
            if (notifyExcludeEventsWithSoundCheckBox->isChecked() !=
                notifyExcludeEventsWithSoundCheckBoxValue )
            {
                changed = true;
                notifyExcludeEventsWithSoundCheckBox->setChecked(
                    notifyExcludeEventsWithSoundCheckBoxValue );
            }
            break;

        case wpInterruption:
            if (textPreMsgCheck->isChecked() != textPreMsgCheckValue)
            {
                changed = true;
                textPreMsgCheck->setChecked(textPreMsgCheckValue);
            }
            if (textPreMsg->text() != i18n(textPreMsgValue.utf8()))
            {
                changed = true;
                textPreMsg->setText(i18n(textPreMsgValue.utf8()));
            }
            if (textPreSndCheck->isChecked() != textPreSndCheckValue)
            {
                changed = true;
                textPreSndCheck->setChecked(textPreSndCheckValue);
            }
            if (textPreSnd->url() != textPreSndValue)
            {
                changed = true;
                textPreSnd->setURL(textPreSndValue);
            }
            if (textPostMsgCheck->isChecked() != textPostMsgCheckValue)
            {
                changed = true;
                textPostMsgCheck->setChecked(textPostMsgCheckValue);
            }
            if (textPostMsg->text() != i18n(textPostMsgValue.utf8()))
            {
                changed = true;
                textPostMsg->setText(i18n(textPostMsgValue.utf8()));
            }
            if (textPostSndCheck->isChecked() != textPostSndCheckValue)
            {
                changed = true;
                textPostSndCheck->setChecked(textPostSndCheckValue);
            }
            if (textPostSnd->url() != textPostSndValue)
            {
                changed = true;
                textPostSnd->setURL(textPostSndValue);
            }
            break;

        case wpAudio:
            if (!artsRadioButton->isChecked())
            {
                changed = true;
                artsRadioButton->setChecked(true);
            }
            if (timeBox->value() != timeBoxValue)
            {
                changed = true;
                timeBox->setValue(timeBoxValue);
            }
            if (keepAudioCheckBox->isChecked() !=
                 keepAudioCheckBoxValue)
            {
                changed = true;
                keepAudioCheckBox->setChecked(keepAudioCheckBoxValue);
            }
            if (keepAudioPath->url() != locateLocal("data", "kttsd/audio/"))
            {
                changed = true;
                keepAudioPath->setURL(locateLocal("data", "kttsd/audio/"));
            }
            keepAudioPath->setEnabled(keepAudioCheckBox->isEnabled());
    }
    if (changed) configChanged();
}

/**
* This is a static method which gets called to realize the modules settings
* during the startup of KDE. NOTE that most modules do not implement this
* method, but modules like the keyboard and mouse modules, which directly
* interact with the X-server, need this method. As this method is static,
* it can avoid to create an instance of the user interface, which is often
* not needed in this case.
*/
void KCMKttsMgr::init(){
    // kdDebug() << "KCMKttsMgr::init: Running" << endl;
}

/**
* The control center calls this function to decide which buttons should
* be displayed. For example, it does not make sense to display an "Apply" 
* button for one of the information modules. The value returned can be set by 
* modules using setButtons.
*/
int KCMKttsMgr::buttons() {
    // kdDebug() << "KCMKttsMgr::buttons: Running"<< endl;
    return KCModule::Ok|KCModule::Apply|KCModule::Help|KCModule::Default;
}

/**
* This function returns the small quickhelp.
* That is displayed in the sidebar in the KControl
*/
QString KCMKttsMgr::quickHelp() const{
    // kdDebug() << "KCMKttsMgr::quickHelp: Running"<< endl;
    return i18n(
        "<h1>Text-to-Speech</h1>"
        "<p>This is the configuration for the text-to-speech dcop service</p>"
        "<p>This allows other applications to access text-to-speech resources</p>"
        "<p>Be sure to configure a default language for the language you are using as this will be the language used by most of the applications</p>");
}

const KAboutData* KCMKttsMgr::aboutData() const{
    KAboutData *about =
    new KAboutData(I18N_NOOP("kttsd"), I18N_NOOP("KCMKttsMgr"),
        0, 0, KAboutData::License_GPL,
        I18N_NOOP("(c) 2002, José Pablo Ezequiel Fernández"));

    about->addAuthor("José Pablo Ezequiel Fernández", I18N_NOOP("Author") , "pupeno@kde.org");
    about->addAuthor("Gary Cramblitt", I18N_NOOP("Maintainer") , "garycramblitt@comcast.net");
    about->addAuthor("Olaf Schmidt", I18N_NOOP("Contributor"), "ojschmidt@kde.org");
    about->addAuthor("Paul Giannaros", I18N_NOOP("Contributor"), "ceruleanblaze@gmail.com");

    return about;
}

/**
* Loads the configuration plug in for a named talker plug in and type.
*/
PlugInConf* KCMKttsMgr::loadTalkerPlugin(const QString& name)
{
    // kdDebug() << "KCMKttsMgr::loadPlugin: Running"<< endl;

    // Find the plugin.
    KTrader::OfferList offers = KTrader::self()->query("KTTSD/SynthPlugin",
        QString("Name == '%1'").arg(name));

    if (offers.count() == 1)
    {
        // When the entry is found, load the plug in
        // First create a factory for the library
        KLibFactory *factory = KLibLoader::self()->factory(offers[0]->library().latin1());
        if(factory){
            // If the factory is created successfully, instantiate the PlugInConf class for the
            // specific plug in to get the plug in configuration object.
            PlugInConf *plugIn = KParts::ComponentFactory::createInstanceFromLibrary<PlugInConf>(
                    offers[0]->library().latin1(), NULL, offers[0]->library().latin1());
            if(plugIn){
                // If everything went ok, return the plug in pointer.
                return plugIn;
            } else {
                // Something went wrong, returning null.
                kdDebug() << "KCMKttsMgr::loadTalkerPlugin: Unable to instantiate PlugInConf class for plugin " << name << endl;
                return NULL;
            }
        } else {
            // Something went wrong, returning null.
            kdDebug() << "KCMKttsMgr::loadTalkerPlugin: Unable to create Factory object for plugin "
                << name << endl;
            return NULL;
        }
    }
    // The plug in was not found (unexpected behaviour, returns null).
    kdDebug() << "KCMKttsMgr::loadTalkerPlugin: KTrader did not return an offer for plugin "
        << name << endl;
    return NULL;
}

/**
 * Loads the configuration plug in for a named filter plug in.
 */
KttsFilterConf* KCMKttsMgr::loadFilterPlugin(const QString& plugInName)
{
    // kdDebug() << "KCMKttsMgr::loadPlugin: Running"<< endl;

    // Find the plugin.
    KTrader::OfferList offers = KTrader::self()->query("KTTSD/FilterPlugin",
        QString("Name == '%1'").arg(plugInName));

    if (offers.count() == 1)
    {
        // When the entry is found, load the plug in
        // First create a factory for the library
        KLibFactory *factory = KLibLoader::self()->factory(offers[0]->library().latin1());
        if(factory){
            // If the factory is created successfully, instantiate the KttsFilterConf class for the
            // specific plug in to get the plug in configuration object.
            int errorNo;
            KttsFilterConf *plugIn =
                KParts::ComponentFactory::createInstanceFromLibrary<KttsFilterConf>(
                    offers[0]->library().latin1(), NULL, offers[0]->library().latin1(),
                    QStringList(), &errorNo);
            if(plugIn){
                // If everything went ok, return the plug in pointer.
                return plugIn;
            } else {
                // Something went wrong, returning null.
                kdDebug() << "KCMKttsMgr::loadFilterPlugin: Unable to instantiate KttsFilterConf class for plugin " << plugInName << " error: " << errorNo << endl;
                return NULL;
            }
        } else {
            // Something went wrong, returning null.
            kdDebug() << "KCMKttsMgr::loadFilterPlugin: Unable to create Factory object for plugin " << plugInName << endl;
            return NULL;
        }
    }
    // The plug in was not found (unexpected behaviour, returns null).
    kdDebug() << "KCMKttsMgr::loadFilterPlugin: KTrader did not return an offer for plugin " << plugInName << endl;
    return NULL;
}

/**
 * Add a talker.
 */
void KCMKttsMgr::slot_addTalker()
{
    AddTalker* addTalkerWidget = new AddTalker(m_synthToLangMap, this, "AddTalker_widget");
    KDialogBase* dlg = new KDialogBase(
        KDialogBase::Swallow,
        i18n("Add Talker"),
        KDialogBase::Help|KDialogBase::Ok|KDialogBase::Cancel,
        KDialogBase::Cancel,
        this,
        "AddTalker_dlg",
        true,
        true);
    dlg->setMainWidget(addTalkerWidget);
    dlg->setHelp("select-plugin", "kttsd");
    int dlgResult = dlg->exec();
    QString languageCode = addTalkerWidget->getLanguageCode();
    QString synthName = addTalkerWidget->getSynthesizer();
    delete dlg;
    // TODO: Also delete addTalkerWidget?
    if (dlgResult != QDialog::Accepted) return;

    // If user chose "Other", must now get a language from him.
    if(languageCode == "other")
    {
        // Create a  QHBox to host KListView.
        Q3HBox* hBox = new Q3HBox(this, "SelectLanguage_hbox");
        // Create a KListView and fill with all known languages.
        KListView* langLView = new KListView(hBox, "SelectLanguage_lview");
        langLView->addColumn(i18n("Language"));
        langLView->addColumn(i18n("Code"));
        QStringList allLocales = KGlobal::locale()->allLanguagesTwoAlpha();
        QString locale;
        QString countryCode;
        QString charSet;
        QString language;
        const int allLocalesCount = allLocales.count();
        for (int ndx=0; ndx < allLocalesCount; ++ndx)
        {
            locale = allLocales[ndx];
            language = TalkerCode::languageCodeToLanguage(locale);
            new Q3ListViewItem(langLView, language, locale);
        }
        // Sort by language.
        langLView->setSorting(0);
        langLView->sort();
        // Display the box in a dialog.
        KDialogBase* dlg = new KDialogBase(
            KDialogBase::Swallow,
            i18n("Select Language"),
            KDialogBase::Help|KDialogBase::Ok|KDialogBase::Cancel,
            KDialogBase::Cancel,
            this,
            "SelectLanguage_dlg",
            true,
            true);
        dlg->setMainWidget(hBox);
        dlg->setHelp("select-plugin", "kttsd");
        dlg->setInitialSize(QSize(200, 500), false);
        dlgResult = dlg->exec();
        languageCode = QString::null;
        if (langLView->selectedItem()) languageCode = langLView->selectedItem()->text(1);
        delete dlg;
        // TODO: Also delete KListView and QHBox?
        if (dlgResult != QDialog::Accepted) return;
    }

    if (languageCode.isEmpty()) return;
    QString language = TalkerCode::languageCodeToLanguage(languageCode);
    if (language.isEmpty()) return;

    m_languagesToCodes[language] = languageCode;

    // Assign a new Talker ID for the talker.  Wraps around to 1.
    QString talkerID = QString::number(m_lastTalkerID + 1);

    // Erase extraneous Talker configuration entries that might be there.
    m_config->deleteGroup(QString("Talker_")+talkerID);
    m_config->sync();

    // Convert translated plugin name to DesktopEntryName.
    QString desktopEntryName = TalkerCode::TalkerNameToDesktopEntryName(synthName);
    // This shouldn't happen, but just in case.
    if (desktopEntryName.isEmpty()) return;

    // Load the plugin.
    m_loadedTalkerPlugIn = loadTalkerPlugin(synthName);
    if (!m_loadedTalkerPlugIn) return;

    // Give plugin the user's language code and permit plugin to autoconfigure itself.
    m_loadedTalkerPlugIn->setDesiredLanguage(languageCode);
    m_loadedTalkerPlugIn->load(m_config, QString("Talker_")+talkerID);

    // If plugin was able to configure itself, it returns a full talker code.
    // If not, display configuration dialog for user to configure the plugin.
    QString talkerCode = m_loadedTalkerPlugIn->getTalkerCode();
    if (talkerCode.isEmpty())
    {
        // Display configuration dialog.
        configureTalker();
        // Did user Cancel?
        if (!m_loadedTalkerPlugIn)
        {
            m_configDlg->setMainWidget(0);
            delete m_configDlg;
            m_configDlg = 0;
            return;
        }
        talkerCode = m_loadedTalkerPlugIn->getTalkerCode();
    }

    // If still no Talker Code, abandon.
    if (!talkerCode.isEmpty())
    {
        // Let plugin save its configuration.
        m_config->setGroup(QString("Talker_")+talkerID);
        m_loadedTalkerPlugIn->save(m_config, QString("Talker_"+talkerID));

        // Record last Talker ID used for next add.
        m_lastTalkerID = talkerID.toInt();

        // Record configuration data.  Note, might as well do this now.
        m_config->setGroup(QString("Talker_")+talkerID);
        m_config->writeEntry("DesktopEntryName", desktopEntryName);
        talkerCode = TalkerCode::normalizeTalkerCode(talkerCode, languageCode);
        m_config->writeEntry("TalkerCode", talkerCode);
        m_config->sync();

        // Add to list of Talkers.
        TalkerCode tc = TalkerCode(talkerCode);
        tc.setId(talkerID);
        tc.setDesktopEntryName(desktopEntryName);
        m_talkerListModel.appendRow(tc);

        // Make sure visible.
        const QModelIndex modelIndex = m_talkerListModel.index(m_talkerListModel.rowCount(),
            0, QModelIndex());
        talkersView->scrollTo(modelIndex);

        // Select the new item, update buttons.
        talkersView->setCurrentIndex(modelIndex);
        updateTalkerButtons();

        // Inform Control Center that change has been made.
        configChanged();
    }

    // Don't need plugin in memory anymore.
    delete m_loadedTalkerPlugIn;
    m_loadedTalkerPlugIn = 0;
    if (m_configDlg)
    {
        m_configDlg->setMainWidget(0);
        delete m_configDlg;
        m_configDlg = 0;
    }

    // kdDebug() << "KCMKttsMgr::addTalker: done." << endl;
}

void KCMKttsMgr::slot_addNormalFilter()
{
    addFilter( false );
}

void KCMKttsMgr:: slot_addSbdFilter()
{
    addFilter( true );
}

/**
* Add a filter.
*/
void KCMKttsMgr::addFilter( bool sbd)
{
    // Build a list of filters that support multiple instances and let user choose.
    QTreeView* lView = filtersView;
    if (sbd) lView = sbdsView;

    QStringList filterPlugInNames;
    for (int i = 0; i < m_filterListModel.rowCount(); ++i) {
        FilterItem fi = m_filterListModel.getRow(i);
        if (fi.multiInstance)
        {
            if (!filterPlugInNames.contains(fi.plugInName))
                filterPlugInNames.append(fi.plugInName);
        }
    }
    // Append those available plugins not yet in the list at all.
    KTrader::OfferList offers = KTrader::self()->query("KTTSD/FilterPlugin");
    for (int i=0; i < offers.count() ; ++i)
    {
        QString filterPlugInName = offers[i]->name();
        if (countFilterPlugins(filterPlugInName) == 0)
        {
            KttsFilterConf* filterConf = loadFilterPlugin( filterPlugInName );
            if (filterConf)
            {
                if (filterConf->isSBD() == sbd)
                    filterPlugInNames.append(filterPlugInName);
                delete filterConf;
            }
        }
    }

    // If no choice (shouldn't happen), bail out.
    // kdDebug() << "KCMKttsMgr::addFilter: filterPluginNames = " << filterPlugInNames << endl;
    if (filterPlugInNames.count() == 0) return;

    // If exactly one choice, skip selection dialog, otherwise display list to user to select from.
    bool okChosen = false;
    QString filterPlugInName;
    if (filterPlugInNames.count() > 1)
    {
        filterPlugInName = KInputDialog::getItem(
            i18n("Select Filter"),
            i18n("Filter"),
            filterPlugInNames,
            0,
            false,
            &okChosen,
            this,
            "selectfilter_kttsd");
        if (!okChosen) return;
    } else
        filterPlugInName = filterPlugInNames[0];

    // Assign a new Filter ID for the filter.  Wraps around to 1.
    QString filterID = QString::number(m_lastFilterID + 1);

    // Erase extraneous Filter configuration entries that might be there.
    m_config->deleteGroup(QString("Filter_")+filterID);
    m_config->sync();

    // Get DesktopEntryName from the translated name.
    QString desktopEntryName = FilterNameToDesktopEntryName(filterPlugInName);
    // This shouldn't happen, but just in case.
    if (desktopEntryName.isEmpty()) return;

    // Load the plugin.
    m_loadedFilterPlugIn = loadFilterPlugin(filterPlugInName);
    if (!m_loadedFilterPlugIn) return;

    // Permit plugin to autoconfigure itself.
    m_loadedFilterPlugIn->load(m_config, QString("Filter_")+filterID);

    // Display configuration dialog for user to configure the plugin.
    configureFilter();

    // Did user Cancel?
    if (!m_loadedFilterPlugIn)
    {
        m_configDlg->setMainWidget(0);
        delete m_configDlg;
        m_configDlg = 0;
        return;
    }

    // Get user's name for Filter.
    QString userFilterName = m_loadedFilterPlugIn->userPlugInName();

    // If user properly configured the plugin, save its configuration.
    if ( !userFilterName.isEmpty() )
    {
        // Let plugin save its configuration.
        m_config->setGroup(QString("Filter_")+filterID);
        m_loadedFilterPlugIn->save(m_config, QString("Filter_"+filterID));

        // Record last Filter ID used for next add.
        m_lastFilterID = filterID.toInt();

        // Determine if filter supports multiple instances.
        bool multiInstance = m_loadedFilterPlugIn->supportsMultiInstance();

        // Record configuration data.  Note, might as well do this now.
        m_config->setGroup(QString("Filter_")+filterID);
        m_config->writeEntry("DesktopEntryName", desktopEntryName);
        m_config->writeEntry("UserFilterName", userFilterName);
        m_config->writeEntry("MultiInstance", multiInstance);
        m_config->writeEntry("Enabled", true);
        m_config->writeEntry("IsSBD", sbd);
        m_config->sync();

        // Add listview item.
        FilterItem fi;
        fi.id = filterID;
        fi.plugInName = filterPlugInName;
        fi.userFilterName = userFilterName;
        fi.desktopEntryName = desktopEntryName;
        fi.multiInstance = multiInstance;
        fi.enabled = true;
        if (sbd)
            m_sbdFilterListModel.appendRow(fi);
        else
            m_filterListModel.appendRow(fi);

        // Make sure visible.
        QModelIndex modelIndex = m_filterListModel.index(m_filterListModel.rowCount() - 1, 0, QModelIndex());
        lView->scrollTo(modelIndex);

        // Select the new item, update buttons.
        lView->setCurrentIndex(modelIndex);
        if (sbd)
            updateSbdButtons();
        else
            updateFilterButtons();

        // Inform Control Center that change has been made.
        configChanged();
    }

    // Don't need plugin in memory anymore.
    delete m_loadedFilterPlugIn;
    m_loadedFilterPlugIn = 0;
    m_configDlg->setMainWidget(0);
    delete m_configDlg;
    m_configDlg = 0;

    // kdDebug() << "KCMKttsMgr::addFilter: done." << endl;
}

/**
* Remove talker.
*/
void KCMKttsMgr::slot_removeTalker(){
    // kdDebug() << "KCMKttsMgr::removeTalker: Running"<< endl;

    // Get the selected talker.
    QModelIndex modelIndex = talkersView->currentIndex();
    if (!modelIndex.isValid()) return;

    // Delete the talker from configuration file.
//    QString talkerID = itemToRemove->text(tlvcTalkerID);
//    m_config->deleteGroup("Talker_"+talkerID, true, false);

    // Delete the talker from the list of Talkers.
    m_talkerListModel.removeRow(modelIndex.row());

    updateTalkerButtons();

    // Emit configuraton changed.
    configChanged();
}

void KCMKttsMgr::slot_removeNormalFilter()
{
    removeFilter( false );
}

void KCMKttsMgr::slot_removeSbdFilter()
{
    removeFilter( true );
}

/**
* Remove filter.
*/
void KCMKttsMgr::removeFilter( bool sbd )
{
    // kdDebug() << "KCMKttsMgr::removeFilter: Running"<< endl;

    QModelIndex modelIndex;
    if (sbd) {
        // Get currently-selected filter.
        modelIndex = sbdsView->currentIndex();
        if (!modelIndex.isValid()) return;
        // Delete the filter from list view.
        m_sbdFilterListModel.removeRow(modelIndex.row());
        updateSbdButtons();
    } else {
        // Get currently-selected filter.
        modelIndex = filtersView->currentIndex();
        if (!modelIndex.isValid()) return;
        // Delete the filter from list view.
        m_filterListModel.removeRow(modelIndex.row());
        updateFilterButtons();
    }

    // Emit configuraton changed.
    configChanged();
}

void KCMKttsMgr::slot_higherTalkerPriority()
{
    QModelIndex modelIndex = talkersView->currentIndex();
    if (!modelIndex.isValid()) return;
    m_talkerListModel.swap(modelIndex.row(), modelIndex.row() - 1);
    modelIndex = m_talkerListModel.index(modelIndex.row() - 1, 0, QModelIndex());
    talkersView->scrollTo(modelIndex);
    talkersView->setCurrentIndex(modelIndex);
    updateTalkerButtons();
    configChanged();
}

void KCMKttsMgr::slot_higherNormalFilterPriority()
{
    QModelIndex modelIndex = filtersView->currentIndex();
    if (!modelIndex.isValid()) return;
    m_filterListModel.swap(modelIndex.row(), modelIndex.row() - 1);
    modelIndex = m_filterListModel.index(modelIndex.row() - 1, 0, QModelIndex());
    filtersView->scrollTo(modelIndex);
    filtersView->setCurrentIndex(modelIndex);
    updateFilterButtons();
    configChanged();
}

void KCMKttsMgr::slot_higherSbdFilterPriority()
{
    QModelIndex modelIndex = sbdsView->currentIndex();
    if (!modelIndex.isValid()) return;
    m_sbdFilterListModel.swap(modelIndex.row(), modelIndex.row() - 1);
    modelIndex = m_sbdFilterListModel.index(modelIndex.row() - 1, 0, QModelIndex());
    sbdsView->scrollTo(modelIndex);
    sbdsView->setCurrentIndex(modelIndex);
    updateSbdButtons();
    configChanged();
}

void KCMKttsMgr::slot_lowerTalkerPriority()
{
    QModelIndex modelIndex = talkersView->currentIndex();
    if (!modelIndex.isValid()) return;
    m_talkerListModel.swap(modelIndex.row(), modelIndex.row() + 1);
    modelIndex = m_talkerListModel.index(modelIndex.row() + 1, 0, QModelIndex());
    talkersView->scrollTo(modelIndex);
    talkersView->setCurrentIndex(modelIndex);
    updateTalkerButtons();
    configChanged();
}

void KCMKttsMgr::slot_lowerNormalFilterPriority()
{
    QModelIndex modelIndex = filtersView->currentIndex();
    if (!modelIndex.isValid()) return;
    m_filterListModel.swap(modelIndex.row(), modelIndex.row() + 1);
    modelIndex = m_filterListModel.index(modelIndex.row() + 1, 0, QModelIndex());
    filtersView->scrollTo(modelIndex);
    filtersView->setCurrentIndex(modelIndex);
    updateFilterButtons();
    configChanged();
}

void KCMKttsMgr::slot_lowerSbdFilterPriority()
{
    QModelIndex modelIndex = sbdsView->currentIndex();
    if (!modelIndex.isValid()) return;
    m_sbdFilterListModel.swap(modelIndex.row(), modelIndex.row() + 1);
    modelIndex = m_sbdFilterListModel.index(modelIndex.row() + 1, 0, QModelIndex());
    sbdsView->scrollTo(modelIndex);
    sbdsView->setCurrentIndex(modelIndex);
    updateSbdButtons();
    configChanged();
}

/**
* Update the status of the Talker buttons.
*/
void KCMKttsMgr::updateTalkerButtons(){
    // kdDebug() << "KCMKttsMgr::updateTalkerButtons: Running"<< endl;
    QModelIndex modelIndex = talkersView->currentIndex();
    if (modelIndex.isValid()) {
        removeTalkerButton->setEnabled(true);
        configureTalkerButton->setEnabled(true);
        higherTalkerPriorityButton->setEnabled(modelIndex.row() != 0);
        lowerTalkerPriorityButton->setEnabled(modelIndex.row() < (m_talkerListModel.rowCount() - 1));
    } else {
        removeTalkerButton->setEnabled(false);
        configureTalkerButton->setEnabled(false);
        higherTalkerPriorityButton->setEnabled(false);
        lowerTalkerPriorityButton->setEnabled(false);
    }
    // kdDebug() << "KCMKttsMgr::updateTalkerButtons: Exiting"<< endl;
}

/**
* Update the status of the normal Filter buttons.
*/
void KCMKttsMgr::updateFilterButtons(){
    // kdDebug() << "KCMKttsMgr::updateFilterButtons: Running"<< endl;
    QModelIndex modelIndex = filtersView->currentIndex();
    if (modelIndex.isValid()) {
        removeFilterButton->setEnabled(true);
        configureFilterButton->setEnabled(true);
        higherFilterPriorityButton->setEnabled(modelIndex.row() != 0);
        lowerFilterPriorityButton->setEnabled(modelIndex.row() < (m_filterListModel.rowCount() - 1));
    } else {
        removeFilterButton->setEnabled(false);
        configureFilterButton->setEnabled(false);
        higherFilterPriorityButton->setEnabled(false);
        lowerFilterPriorityButton->setEnabled(false);
    }
    // kdDebug() << "KCMKttsMgr::updateFilterButtons: Exiting"<< endl;
}

/**
 * Update the status of the SBD buttons.
 */
void KCMKttsMgr::updateSbdButtons(){
    // kdDebug() << "KCMKttsMgr::updateSbdButtons: Running"<< endl;
    QModelIndex modelIndex = sbdsView->currentIndex();
    if (modelIndex.isValid()) {
        m_sbdPopmenu->setItemEnabled( sbdBtnEdit, true );
        m_sbdPopmenu->setItemEnabled( sbdBtnUp, modelIndex.row() != 0 );
        m_sbdPopmenu->setItemEnabled( sbdBtnDown, modelIndex.row() < (m_sbdFilterListModel.rowCount() -1));
        m_sbdPopmenu->setItemEnabled( sbdBtnRemove, true );
    } else {
        m_sbdPopmenu->setItemEnabled( sbdBtnEdit, false );
        m_sbdPopmenu->setItemEnabled( sbdBtnUp, false );
        m_sbdPopmenu->setItemEnabled( sbdBtnDown, false );
        m_sbdPopmenu->setItemEnabled( sbdBtnRemove, false );
    }
    // kdDebug() << "KCMKttsMgr::updateSbdButtons: Exiting"<< endl;
}

/**
* This signal is emitted whenever user checks/unchecks the Enable TTS System check box.
*/
void KCMKttsMgr::enableKttsdToggled(bool)
{
    // Prevent re-entrancy.
    static bool reenter;
    if (reenter) return;
    reenter = true;
    // See if KTTSD is running.
    DCOPClient *client = kapp->dcopClient();
    bool kttsdRunning = (client->isApplicationRegistered("kttsd"));
    // kdDebug() << "KCMKttsMgr::enableKttsdToggled: kttsdRunning = " << kttsdRunning << endl;
    // If Enable KTTSD check box is checked and it is not running, then start KTTSD.
    if (enableKttsdCheckBox->isChecked())
    {
        if (!kttsdRunning)
        {
            // kdDebug() << "KCMKttsMgr::enableKttsdToggled:: Starting KTTSD" << endl;
            QString error;
            if (KApplication::startServiceByDesktopName("kttsd", QStringList(), &error))
            {
                kdDebug() << "Starting KTTSD failed with message " << error << endl;
                enableKttsdCheckBox->setChecked(false);
                notifyTestButton->setEnabled(false);
            }
        }
    }
    else
    // If check box is not checked and it is running, then stop KTTSD.
    {
    if (kttsdRunning)
        {
            // kdDebug() << "KCMKttsMgr::enableKttsdToggled:: Stopping KTTSD" << endl;
            QByteArray data;
            client->send("kttsd", "KSpeech", "kttsdExit()", data);
        }
    }
    reenter = false;
}

/**
* This signal is emitted whenever user checks/unchecks the GStreamer radio button.
*/
void KCMKttsMgr::slotGstreamerRadioButton_toggled(bool state)
{
    sinkLabel->setEnabled(state);
    sinkComboBox->setEnabled(state);
}

/**
* This signal is emitted whenever user checks/unchecks the ALSA radio button.
*/
void KCMKttsMgr::slotAlsaRadioButton_toggled(bool state)
{
    pcmLabel->setEnabled(state);
    pcmComboBox->setEnabled(state);
}

/**
* This signal is emitted whenever user checks/unchecks the aKode radio button.
*/
void KCMKttsMgr::slotAkodeRadioButton_toggled(bool state)
{
    akodeSinkLabel->setEnabled(state);
    akodeComboBox->setEnabled(state);
}

/**
* This slot is called whenever KTTSD starts or restarts.
*/
void KCMKttsMgr::kttsdStarted()
{
    // kdDebug() << "KCMKttsMgr::kttsdStarted: Running" << endl;
    bool kttsdLoaded = (m_jobMgrPart != 0);
    // Load Job Manager Part library.
    if (!kttsdLoaded)
    {
        KLibFactory *factory = KLibLoader::self()->factory( "libkttsjobmgrpart" );
        if (factory)
        {
            // Create the Job Manager part
            m_jobMgrPart = (KParts::ReadOnlyPart *)factory->create( mainTab, "kttsjobmgr",
                "KParts::ReadOnlyPart" );
            if (m_jobMgrPart)
            {
                // Add the Job Manager part as a new tab.
                mainTab->addTab(m_jobMgrPart->widget(), i18n("&Jobs"));
                kttsdLoaded = true;
            }
            else
                kdDebug() << "Could not create kttsjobmgr part." << endl;
        }
        else kdDebug() << "Could not load libkttsjobmgrpart.  Is libkttsjobmgrpart installed?" << endl;
    }
    // Check/Uncheck the Enable KTTSD check box.
    if (kttsdLoaded)
    {
        enableKttsdCheckBox->setChecked(true);
        // Enable/disable notify Test button.
        slotNotifyListView_currentItemChanged();
    } else {
        enableKttsdCheckBox->setChecked(false);
        notifyTestButton->setEnabled(false);
    }
}

/**
* This slot is called whenever KTTSD is about to exit.
*/
void KCMKttsMgr::kttsdExiting()
{
    // kdDebug() << "KCMKttsMgr::kttsdExiting: Running" << endl;
    if (m_jobMgrPart)
    {
        mainTab->removePage(m_jobMgrPart->widget());
        delete m_jobMgrPart;
        m_jobMgrPart = 0;
    }
    enableKttsdCheckBox->setChecked(false);
    notifyTestButton->setEnabled(false);
}

/**
* User has requested display of talker configuration dialog.
*/
void KCMKttsMgr::slot_configureTalker()
{
    // Get highlighted plugin from Talker ListView and load into memory.
    QModelIndex modelIndex = talkersView->currentIndex();
    if (!modelIndex.isValid()) return;
    TalkerCode tc = m_talkerListModel.getRow(modelIndex.row());
    QString talkerID = tc.id();
    QString synthName = tc.plugInName();
    QString languageCode = tc.fullLanguageCode();
    m_loadedTalkerPlugIn = loadTalkerPlugin(synthName);
    if (!m_loadedTalkerPlugIn) return;
    // kdDebug() << "KCMKttsMgr::slot_configureTalker: plugin for " << synthName << " loaded successfully." << endl;

    // Tell plugin to load its configuration.
    m_config->setGroup(QString("Talker_")+talkerID);
    m_loadedTalkerPlugIn->setDesiredLanguage(languageCode);
    // kdDebug() << "KCMKttsMgr::slot_configureTalker: about to call plugin load() method with Talker ID = " << talkerID << endl;
    m_loadedTalkerPlugIn->load(m_config, QString("Talker_")+talkerID);

    // Display configuration dialog.
    configureTalker();

    // Did user Cancel?
    if (!m_loadedTalkerPlugIn)
    {
        m_configDlg->setMainWidget(0);
        delete m_configDlg;
        m_configDlg = 0;
        return;
    }

    // Get Talker Code.  Note that plugin may return a code different from before.
    QString talkerCode = m_loadedTalkerPlugIn->getTalkerCode();

    // If plugin was successfully configured, save its configuration.
    if (!talkerCode.isEmpty())
    {
        m_config->setGroup(QString("Talker_")+talkerID);
        m_loadedTalkerPlugIn->save(m_config, QString("Talker_")+talkerID);
        m_config->setGroup(QString("Talker_")+talkerID);
        talkerCode = TalkerCode::normalizeTalkerCode(talkerCode, languageCode);
        m_config->writeEntry("TalkerCode", talkerCode);
        m_config->sync();

        // Update display.
        tc.setTalkerCode(talkerCode);
        // TODO
        // updateTalkerItem(talkerItem, talkerCode);

        // Inform Control Center that configuration has changed.
        configChanged();
    }

    delete m_loadedTalkerPlugIn;
    m_loadedTalkerPlugIn = 0;
    m_configDlg->setMainWidget(0);
    delete m_configDlg;
    m_configDlg = 0;
}

void KCMKttsMgr::slot_configureNormalFilter()
{
    configureFilterItem( false );
}

void KCMKttsMgr::slot_configureSbdFilter()
{
    configureFilterItem( true );
}

/**
 * User has requested display of filter configuration dialog.
 */
void KCMKttsMgr::configureFilterItem( bool sbd )
{
    // Get highlighted plugin from Filter ListView and load into memory.
    QTreeView* lView;
    FilterListModel* model;
    if (sbd) {
        lView = sbdsView;
        model = &m_sbdFilterListModel;
    } else {
        lView = filtersView;
        model = &m_filterListModel;
    }
    QModelIndex modelIndex = lView->currentIndex();
    if (!modelIndex.isValid()) return;
    FilterItem fi = model->getRow(modelIndex.row());
    QString filterID = fi.id;
    QString filterPlugInName = fi.plugInName;
    QString desktopEntryName = fi.desktopEntryName;
    if (desktopEntryName.isEmpty()) return;
    m_loadedFilterPlugIn = loadFilterPlugin(filterPlugInName);
    if (!m_loadedFilterPlugIn) return;
    // kdDebug() << "KCMKttsMgr::slot_configureFilter: plugin for " << filterPlugInName << " loaded successfully." << endl;

    // Tell plugin to load its configuration.
    m_config->setGroup(QString("Filter_")+filterID);
    // kdDebug() << "KCMKttsMgr::slot_configureFilter: about to call plugin load() method with Filter ID = " << filterID << endl;
    m_loadedFilterPlugIn->load(m_config, QString("Filter_")+filterID);

    // Display configuration dialog.
    configureFilter();

    // Did user Cancel?
    if (!m_loadedFilterPlugIn)
    {
        m_configDlg->setMainWidget(0);
        delete m_configDlg;
        m_configDlg = 0;
        return;
    }

    // Get user's name for the plugin.
    QString userFilterName = m_loadedFilterPlugIn->userPlugInName();

    // If user properly configured the plugin, save the configuration.
    if ( !userFilterName.isEmpty() )
    {

        // Let plugin save its configuration.
        m_config->setGroup(QString("Filter_")+filterID);
        m_loadedFilterPlugIn->save(m_config, QString("Filter_")+filterID);

        // Save configuration.
        m_config->setGroup("Filter_"+filterID);
        m_config->writeEntry("DesktopEntryName", desktopEntryName);
        m_config->writeEntry("UserFilterName", userFilterName);
        m_config->writeEntry("Enabled", true);
        m_config->writeEntry("MultiInstance", m_loadedFilterPlugIn->supportsMultiInstance());
        m_config->writeEntry("IsSBD", sbd);

        m_config->sync();

        // Update display.
        FilterItem fi;
        fi.id = filterID;
        fi.desktopEntryName = desktopEntryName;
        fi.userFilterName = userFilterName;
        fi.enabled = true;
        fi.multiInstance = m_loadedFilterPlugIn->supportsMultiInstance();
        // TODO
        // filterList->append(fi);

        // Inform Control Center that configuration has changed.
        configChanged();
    }

    delete m_loadedFilterPlugIn;
    m_loadedFilterPlugIn = 0;
    m_configDlg->setMainWidget(0);
    delete m_configDlg;
    m_configDlg = 0;
}

/**
* Display talker configuration dialog.  The plugin is assumed already loaded into
* memory referenced by m_loadedTalkerPlugIn.
*/
void KCMKttsMgr::configureTalker()
{
    if (!m_loadedTalkerPlugIn) return;
    m_configDlg = new KDialogBase(
        KDialogBase::Swallow,
        i18n("Talker Configuration"),
        KDialogBase::Help|KDialogBase::Default|KDialogBase::Ok|KDialogBase::Cancel,
        KDialogBase::Cancel,
        this,
        "configureTalker_dlg",
        true,
        true);
    m_configDlg->setInitialSize(QSize(700, 300), false);
    m_configDlg->setMainWidget(m_loadedTalkerPlugIn);
    m_configDlg->setHelp("configure-plugin", "kttsd");
    m_configDlg->enableButtonOK(false);
    connect(m_loadedTalkerPlugIn, SIGNAL( changed(bool) ), this, SLOT( slotConfigTalkerDlg_ConfigChanged() ));
    connect(m_configDlg, SIGNAL( defaultClicked() ), this, SLOT( slotConfigTalkerDlg_DefaultClicked() ));
    connect(m_configDlg, SIGNAL( cancelClicked() ), this, SLOT (slotConfigTalkerDlg_CancelClicked() ));
    // Create a Player object for the plugin to use for testing.
    int playerOption = 0;
    QString sinkName;
    if (gstreamerRadioButton->isChecked()) {
        playerOption = 1;
        sinkName = sinkComboBox->currentText();
    }
    if (alsaRadioButton->isChecked()) {
        playerOption = 2;
        sinkName = pcmComboBox->currentText();
    }
    if (akodeRadioButton->isChecked()) {
        playerOption = 3;
        sinkName = akodeComboBox->currentText();
    }
    float audioStretchFactor = 1.0/(float(timeBox->value())/100.0);
    // kdDebug() << "KCMKttsMgr::configureTalker: playerOption = " << playerOption << " audioStretchFactor = " << audioStretchFactor << " sink name = " << sinkName << endl;
    TestPlayer* testPlayer = new TestPlayer(this, "ktts_testplayer", 
        playerOption, audioStretchFactor, sinkName);
    m_loadedTalkerPlugIn->setPlayer(testPlayer);
    // Display the dialog.
    m_configDlg->exec();
    // Done with Player object.
    if (m_loadedTalkerPlugIn)
    {
        delete testPlayer;
        m_loadedTalkerPlugIn->setPlayer(0);
    }
}

/**
* Display filter configuration dialog.  The plugin is assumed already loaded into
* memory referenced by m_loadedFilterPlugIn.
*/
void KCMKttsMgr::configureFilter()
{
    if (!m_loadedFilterPlugIn) return;
    m_configDlg = new KDialogBase(
        KDialogBase::Swallow,
        i18n("Filter Configuration"),
        KDialogBase::Help|KDialogBase::Default|KDialogBase::Ok|KDialogBase::Cancel,
        KDialogBase::Cancel,
        this,
        "configureFilter_dlg",
        true,
        true);
    m_configDlg->setInitialSize(QSize(600, 450), false);
    m_loadedFilterPlugIn->setMinimumSize(m_loadedFilterPlugIn->minimumSizeHint());
    m_loadedFilterPlugIn->show();
    m_configDlg->setMainWidget(m_loadedFilterPlugIn);
    m_configDlg->setHelp("configure-filter", "kttsd");
    m_configDlg->enableButtonOK(false);
    connect(m_loadedFilterPlugIn, SIGNAL( changed(bool) ), this, SLOT( slotConfigFilterDlg_ConfigChanged() ));
    connect(m_configDlg, SIGNAL( defaultClicked() ), this, SLOT( slotConfigFilterDlg_DefaultClicked() ));
    connect(m_configDlg, SIGNAL( cancelClicked() ), this, SLOT (slotConfigFilterDlg_CancelClicked() ));
    // Display the dialog.
    m_configDlg->exec();
}

/**
* Count number of configured Filters with the specified plugin name.
*/
int KCMKttsMgr::countFilterPlugins(const QString& filterPlugInName)
{
    int cnt = 0;
    for (int i = 0; i < m_filterListModel.rowCount(); ++i) {
        FilterItem fi = m_filterListModel.getRow(i);
        if (fi.plugInName == filterPlugInName) ++cnt;
    }
    for (int i = 0; i < m_sbdFilterListModel.rowCount(); ++i) {
        FilterItem fi = m_sbdFilterListModel.getRow(i);
        if (fi.plugInName == filterPlugInName) ++cnt;
    }
    return cnt;
}

void KCMKttsMgr::keepAudioCheckBox_toggled(bool checked)
{
    keepAudioPath->setEnabled(checked);
    configChanged();
}

// Basically the slider values are logarithmic (0,...,1000) whereas percent
// values are linear (50%,...,200%).
//
// slider = alpha * (log(percent)-log(50))
// with alpha = 1000/(log(200)-log(50))

int KCMKttsMgr::percentToSlider(int percentValue) {
    double alpha = 1000 / (log(200) - log(50));
    return (int)floor (0.5 + alpha * (log(percentValue)-log(50)));
}

int KCMKttsMgr::sliderToPercent(int sliderValue) {
    double alpha = 1000 / (log(200) - log(50));
    return (int)floor(0.5 + exp (sliderValue/alpha + log(50)));
}

void KCMKttsMgr::timeBox_valueChanged(int percentValue) {
    timeSlider->setValue (percentToSlider (percentValue));
}

void KCMKttsMgr::timeSlider_valueChanged(int sliderValue) {
    timeBox->setValue (sliderToPercent (sliderValue));
}

void KCMKttsMgr::slotConfigTalkerDlg_ConfigChanged()
{
    m_configDlg->enableButtonOK(!m_loadedTalkerPlugIn->getTalkerCode().isEmpty());
}

void KCMKttsMgr::slotConfigFilterDlg_ConfigChanged()
{
    m_configDlg->enableButtonOK( !m_loadedFilterPlugIn->userPlugInName().isEmpty() );
}

void KCMKttsMgr::slotConfigTalkerDlg_DefaultClicked()
{
    m_loadedTalkerPlugIn->defaults();
}

void KCMKttsMgr::slotConfigFilterDlg_DefaultClicked()
{
    m_loadedFilterPlugIn->defaults();
}

void KCMKttsMgr::slotConfigTalkerDlg_CancelClicked()
{
    delete m_loadedTalkerPlugIn;
    m_loadedTalkerPlugIn = 0;
}

void KCMKttsMgr::slotConfigFilterDlg_CancelClicked()
{
    delete m_loadedFilterPlugIn;
    m_loadedFilterPlugIn = 0;
}

/**
* This slot is called whenever user checks/unchecks item in Filters list.
*/
void KCMKttsMgr::slotFiltersView_stateChanged()
{
    // kdDebug() << "KCMKttsMgr::slotFiltersView_stateChanged: calling configChanged" << endl;
    configChanged();
}

/**
 * Uses KTrader to convert a translated Filter Plugin Name to DesktopEntryName.
 * @param name                   The translated plugin name.  From Name= line in .desktop file.
 * @return                       DesktopEntryName.  The name of the .desktop file (less .desktop).
 *                               QString::null if not found.
 */
QString KCMKttsMgr::FilterNameToDesktopEntryName(const QString& name)
{
    if (name.isEmpty()) return QString::null;
    KTrader::OfferList offers = KTrader::self()->query("KTTSD/FilterPlugin",
        QString("Name == '%1'").arg(name));

    if (offers.count() == 1)
        return offers[0]->desktopEntryName();
    else
        return QString::null;
}

/**
 * Uses KTrader to convert a DesktopEntryName into a translated Filter Plugin Name.
 * @param desktopEntryName       The DesktopEntryName.
 * @return                       The translated Name of the plugin, from Name= line in .desktop file.
 */
QString KCMKttsMgr::FilterDesktopEntryNameToName(const QString& desktopEntryName)
{
    if (desktopEntryName.isEmpty()) return QString::null;
    KTrader::OfferList offers = KTrader::self()->query("KTTSD/FilterPlugin",
        QString("DesktopEntryName == '%1'").arg(desktopEntryName));

    if (offers.count() == 1)
        return offers[0]->name();
    else
        return QString::null;
}

/**
 * Loads notify events from a file.  Clearing listview if clear is True.
 */
QString KCMKttsMgr::loadNotifyEventsFromFile( const QString& filename, bool clear)
{
    // Open existing event list.
    QFile file( filename );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        return i18n("Unable to open file.") + filename;
    }
    // QDomDocument doc( "http://www.kde.org/share/apps/kttsd/stringreplacer/wordlist.dtd []" );
    QDomDocument doc( "" );
    if ( !doc.setContent( &file ) ) {
        file.close();
        return i18n("File not in proper XML format.");
    }
    // kdDebug() << "StringReplacerConf::load: document successfully parsed." << endl;
    file.close();

    // Clear list view.
    if ( clear ) notifyListView->clear();

    // Event list.
    QDomNodeList eventList = doc.elementsByTagName("notifyEvent");
    const int eventListCount = eventList.count();
    for (int eventIndex = 0; eventIndex < eventListCount; ++eventIndex)
    {
        QDomNode eventNode = eventList.item(eventIndex);
        QDomNodeList propList = eventNode.childNodes();
        QString eventSrc;
        QString event;
        QString actionName;
        QString message;
        TalkerCode talkerCode;
        const int propListCount = propList.count();
        for (int propIndex = 0; propIndex < propListCount; ++propIndex)
        {
            QDomNode propNode = propList.item(propIndex);
            QDomElement prop = propNode.toElement();
            if (prop.tagName() == "eventSrc") eventSrc = prop.text();
            if (prop.tagName() == "event") event = prop.text();
            if (prop.tagName() == "action") actionName = prop.text();
            if (prop.tagName() == "message") message = prop.text();
            if (prop.tagName() == "talker") talkerCode = TalkerCode(prop.text(), false);
        }
        addNotifyItem(eventSrc, event, NotifyAction::action( actionName ), message, talkerCode);
    }
    // TODO: Need a way to get QTreeView to automatically adjust column widths to contents.
    // Setting QHeaderView::Stretch sorta works, but then user cannot manually resize columns,
    // and when QHeadeerView::Interactive is set, resizes to narrow columns. :/
    // notifyListView->header()->setResizeMode( QHeaderView::Stretch );
    // notifyListView->adjustSize();
    // notifyListView->header()->setResizeMode( QHeaderView::Interactive );
    return QString::null;
}

/**
 * Saves notify events to a file.
 */
QString KCMKttsMgr::saveNotifyEventsToFile(const QString& filename)
{
    QFile file( filename );
    if ( !file.open( QIODevice::WriteOnly ) )
        return i18n("Unable to open file ") + filename;

    QDomDocument doc( "" );

    QDomElement root = doc.createElement( "notifyEventList" );
    doc.appendChild( root );

    // Events.
    QTreeWidget* lv = notifyListView;
    const int lvCount = lv->topLevelItemCount();
    for (int i = 0; i < lvCount; ++i) {
        QTreeWidgetItem* topItem = lv->topLevelItem(i);
        const int itemCount = topItem->childCount();
        for (int j = 0; j < itemCount; ++j) {
            QTreeWidgetItem* item = topItem->child(j);

            QDomElement wordTag = doc.createElement( "notifyEvent" );
            root.appendChild( wordTag );

            QDomElement propTag = doc.createElement( "eventSrc" );
            wordTag.appendChild( propTag);
            QDomText t = doc.createTextNode( item->text(nlvcEventSrc) );
            propTag.appendChild( t );

            propTag = doc.createElement( "event" );
            wordTag.appendChild( propTag);
            t = doc.createTextNode( item->text(nlvcEvent) );
            propTag.appendChild( t );

            propTag = doc.createElement( "action" );
            wordTag.appendChild( propTag);
            t = doc.createTextNode( item->text(nlvcAction) );
            propTag.appendChild( t );

            if ( item->text(nlvcAction) == NotifyAction::actionName( NotifyAction::SpeakCustom ) )
            {
                propTag = doc.createElement( "message" );
                wordTag.appendChild( propTag);
                QString msg = item->text(nlvcActionName);
                int msglen = msg.length();
                msg = msg.mid( 1, msglen-2 );
                t = doc.createCDATASection( msg );
                propTag.appendChild( t );
            }

            propTag = doc.createElement( "talker" );
            wordTag.appendChild( propTag);
            t = doc.createCDATASection( item->text(nlvcTalker) );
            propTag.appendChild( t );
        }
    }

    // Write it all out.
    QTextStream ts( &file );
    ts.setEncoding( QTextStream::UnicodeUTF8 );
    ts << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ts << doc.toString();
    file.close();

    return QString::null;
}

void KCMKttsMgr::slotNotifyEnableCheckBox_toggled(bool checked)
{
    notifyExcludeEventsWithSoundCheckBox->setEnabled( checked );
    notifyGroup->setEnabled( checked );
    configChanged();
}

void KCMKttsMgr::slotNotifyPresentComboBox_activated(int index)
{
    QTreeWidgetItem* item = notifyListView->currentItem();
    if ( !item ) return;        // should not happen
    item->setText( nlvcEvent, NotifyPresent::presentName( index ) );
    item->setText( nlvcEventName, NotifyPresent::presentDisplayName( index ) );
    bool enableIt = ( index != NotifyPresent::None);
    notifyActionComboBox->setEnabled( enableIt );
    notifyTalkerButton->setEnabled( enableIt );
    if (!enableIt)
    {
        notifyTalkerLabel->clear();
    } else {
        if ( notifyTalkerLabel->text().isEmpty() )
        {
            notifyTalkerLabel->setText( i18n("default") );
        }
    }
    configChanged();
}

void KCMKttsMgr::slotNotifyListView_currentItemChanged()
{
    QTreeWidgetItem* item = notifyListView->currentItem();
    if ( item )
    {
        bool topLevel = !item->parent();
        if ( topLevel )
        {
            notifyPresentComboBox->setEnabled( false );
            notifyActionComboBox->setEnabled( false );
            notifyTestButton->setEnabled( false );
            notifyMsgLineEdit->setEnabled( false );
            notifyMsgLineEdit->clear();
            notifyTalkerButton->setEnabled( false );
            notifyTalkerLabel->clear();
            bool defaultItem = ( item->text(nlvcEventSrc) == "default" );
            notifyRemoveButton->setEnabled( !defaultItem );
        } else {
            bool defaultItem = ( item->parent()->text(nlvcEventSrc) == "default" );
            notifyPresentComboBox->setEnabled( defaultItem );
            if ( defaultItem )
                notifyPresentComboBox->setCurrentItem( NotifyPresent::present( item->text( nlvcEvent ) ) );
            notifyActionComboBox->setEnabled( true );
            int action = NotifyAction::action( item->text( nlvcAction ) );
            notifyActionComboBox->setCurrentItem( action );
            notifyTalkerButton->setEnabled( true );
            TalkerCode talkerCode( item->text( nlvcTalker ) );
            notifyTalkerLabel->setText( talkerCode.getTranslatedDescription() );
            if ( action == NotifyAction::SpeakCustom )
            {
                notifyMsgLineEdit->setEnabled( true );
                QString msg = item->text( nlvcActionName );
                int msglen = msg.length();
                msg = msg.mid( 1, msglen-2 );
                notifyMsgLineEdit->setText( msg );
            } else {
                notifyMsgLineEdit->setEnabled( false );
                notifyMsgLineEdit->clear();
            }
            notifyRemoveButton->setEnabled( !defaultItem );
            notifyTestButton->setEnabled(
                action != NotifyAction::DoNotSpeak &&
                enableKttsdCheckBox->isChecked());
        }
    } else {
        notifyPresentComboBox->setEnabled( false );
        notifyActionComboBox->setEnabled( false );
        notifyTestButton->setEnabled( false );
        notifyMsgLineEdit->setEnabled( false );
        notifyMsgLineEdit->clear();
        notifyTalkerButton->setEnabled( false );
        notifyTalkerLabel->clear();
        notifyRemoveButton->setEnabled( false );
    }
}

void KCMKttsMgr::slotNotifyActionComboBox_activated(int index)
{
    QTreeWidgetItem* item = notifyListView->currentItem();
    if ( item )
        if ( !item->parent() ) item = 0;
    if ( !item ) return;  // This shouldn't happen.
    item->setText( nlvcAction, NotifyAction::actionName( index ) );
    item->setText( nlvcActionName, NotifyAction::actionDisplayName( index ) );
    if ( index == NotifyAction::SpeakCustom )
        item->setText( nlvcActionName, "\"" + notifyMsgLineEdit->text() + "\"" );
    if ( index == NotifyAction::DoNotSpeak )
        item->setIcon( nlvcActionName, SmallIcon( "nospeak" ) );
    else
        item->setIcon( nlvcActionName, SmallIcon( "speak" ) );
    slotNotifyListView_currentItemChanged();
    configChanged();
}

void KCMKttsMgr::slotNotifyMsgLineEdit_textChanged(const QString& text)
{
    QTreeWidgetItem* item = notifyListView->currentItem();
    if ( item )
        if ( item->parent() ) item = 0;
    if ( !item ) return;  // This shouldn't happen.
    if ( notifyActionComboBox->currentItem() != NotifyAction::SpeakCustom) return;
    item->setText( nlvcActionName, "\"" + text + "\"" );
    notifyTestButton->setEnabled(
        !text.isEmpty() && enableKttsdCheckBox->isChecked());
    configChanged();
}

void KCMKttsMgr::slotNotifyTestButton_clicked()
{
    QTreeWidgetItem* item = notifyListView->currentItem();
    if (item)
    {
        QString msg;
        int action = NotifyAction::action(item->text(nlvcAction));
        switch (action)
        {
            case NotifyAction::SpeakEventName:
                msg = item->text(nlvcEventName);
                break;
            case NotifyAction::SpeakMsg:
                msg = i18n("sample notification message");
                break;
            case NotifyAction::SpeakCustom:
                msg = notifyMsgLineEdit->text();
                msg.replace("%a", i18n("sample application"));
                msg.replace("%e", i18n("sample event"));
                msg.replace("%m", i18n("sample notification message"));
                break;
        }
        if (!msg.isEmpty()) sayMessage(msg, item->text(nlvcTalker));
    }
}

void KCMKttsMgr::slotNotifyTalkerButton_clicked()
{
    QTreeWidgetItem* item = notifyListView->currentItem();
    if ( item )
        if ( !item->parent() ) item = 0;
    if ( !item ) return;  // This shouldn't happen.
    QString talkerCode = item->text( nlvcTalker );
    SelectTalkerDlg dlg( this, "selecttalkerdialog", i18n("Select Talker"), talkerCode, true );
    int dlgResult = dlg.exec();
    if ( dlgResult != KDialogBase::Accepted ) return;
    item->setText( nlvcTalker, dlg.getSelectedTalkerCode() );
    QString talkerName = dlg.getSelectedTranslatedDescription();
    item->setText( nlvcTalkerName, talkerName );
    notifyTalkerLabel->setText( talkerName );
    configChanged();
}

/**
 * Adds an item to the notify listview.
 * message is only needed if action = naSpeakCustom.
 */
QTreeWidgetItem* KCMKttsMgr::addNotifyItem(
    const QString& eventSrc,
    const QString& event,
    int action,
    const QString& message,
    TalkerCode& talkerCode)
{
    QTreeWidget* lv = notifyListView;
    QTreeWidgetItem* item = 0;
    QString iconName;
    QString eventSrcName;
    if (eventSrc == "default")
        eventSrcName = i18n("Default (all other events)");
    else
        eventSrcName = NotifyEvent::getEventSrcName(eventSrc, iconName);
    QString eventName;
    if (eventSrc == "default")
        eventName = NotifyPresent::presentDisplayName( event );
    else
    {
        if (event == "default")
            eventName = i18n("All other %1 events").arg(eventSrcName);
        else
            eventName = NotifyEvent::getEventName(eventSrc, event);
    }
    QString actionName = NotifyAction::actionName( action );
    QString actionDisplayName = NotifyAction::actionDisplayName( action );
    if (action == NotifyAction::SpeakCustom) actionDisplayName = "\"" + message + "\"";
    QString talkerName = talkerCode.getTranslatedDescription();
    if (!eventSrcName.isEmpty() && !eventName.isEmpty() && !actionName.isEmpty() && !talkerName.isEmpty())
    {
        QTreeWidgetItem* parentItem = findTreeWidgetItem(lv, eventSrcName, nlvcEventSrcName);
        if (!parentItem)
        {
            parentItem = new QTreeWidgetItem(lv);
            parentItem->setText( nlvcEventSrcName, eventSrcName );
            parentItem->setText( nlvcEventSrc, eventSrc );
            if ( !iconName.isEmpty() )
                parentItem->setIcon( nlvcEventSrcName, SmallIcon( iconName ) );
        }
        // No duplicates.
        item = findTreeWidgetItem( lv, event, nlvcEvent );
        if ( !item || item->parent() != parentItem ) {
            item = new QTreeWidgetItem(parentItem);
            item->setText( nlvcEventName, eventName );
            item->setText( nlvcActionName, actionDisplayName );
            item->setText( nlvcTalkerName, talkerName );
            item->setText( nlvcEventSrc, eventSrc );
            item->setText( nlvcEvent, event );
            item->setText( nlvcAction, actionName );
            item->setText( nlvcTalker, talkerCode.getTalkerCode() );
        }
        if ( action == NotifyAction::DoNotSpeak )
            item->setIcon( nlvcActionName, SmallIcon( "nospeak" ) );
        else
            item->setIcon( nlvcActionName, SmallIcon( "speak" ) );
    }
    return item;
}

/**
* A convenience method that finds an item in a TreeViewWidget, assuming there is at most
* one occurrence of the item.  Returns 0 if not found.
* @param tw                     The TreeViewWidget to search.
* @param sought                 The string sought.
* @param col                    Column of the TreeViewWidget to search.
* @return                       The item of the TreeViewWidget found or null if not found.
*
* An exact match is performed.
*/
QTreeWidgetItem* KCMKttsMgr::findTreeWidgetItem(QTreeWidget* tw, const QString& sought, int col)
{
    QList<QTreeWidgetItem *> twList = tw->findItems(sought, Qt::MatchExactly, col);
    if (twList.isEmpty()) return 0;
    return twList[0];
}

void KCMKttsMgr::slotNotifyAddButton_clicked()
{
    QTreeWidget* lv = notifyListView;
    QTreeWidgetItem* item = lv->currentItem();
    QString eventSrc;
    if ( item ) eventSrc = item->text( nlvcEventSrc );
    SelectEvent* selectEventWidget = new SelectEvent( this, "SelectEvent_widget", 0, eventSrc );
    KDialogBase* dlg = new KDialogBase(
        KDialogBase::Swallow,
        i18n("Select Event"),
        KDialogBase::Help|KDialogBase::Ok|KDialogBase::Cancel,
        KDialogBase::Cancel,
        this,
        "SelectEvent_dlg",
        true,
        true);
    dlg->setMainWidget( selectEventWidget );
    dlg->setInitialSize( QSize(500, 400) );
    // dlg->setHelp("select-plugin", "kttsd");
    int dlgResult = dlg->exec();
    eventSrc = selectEventWidget->getEventSrc();
    QString event = selectEventWidget->getEvent();
    delete dlg;
    if ( dlgResult != QDialog::Accepted ) return;
    if ( eventSrc.isEmpty() || event.isEmpty() ) return;
    // Use Default action, message, and talker.
    QString actionName;
    int action;
    QString msg;
    TalkerCode talkerCode;
    item = findTreeWidgetItem( lv, "default", nlvcEventSrc );
    if ( item )
    {
        if ( item->childCount() > 0 ) item = item->child(0);
        if ( item )
        {
            actionName = item->text( nlvcAction );
            action = NotifyAction::action( actionName );
            talkerCode = TalkerCode( item->text( nlvcTalker ) );
            if (action == NotifyAction::SpeakCustom )
            {
                msg = item->text(nlvcActionName);
                int msglen = msg.length();
                msg = msg.mid( 1, msglen-2 );
            }
        }
    }
    item = addNotifyItem( eventSrc, event, action, msg, talkerCode );
    lv->scrollToItem( item );
    lv->setCurrentItem( item );
    // TODO: This this needed?
    // slotNotifyListView_currentItemChanged();
    configChanged();
}

void KCMKttsMgr::slotNotifyClearButton_clicked()
{
    QTreeWidget* lv = notifyListView;
    lv->clear();
    TalkerCode talkerCode( QString::null );
    QTreeWidgetItem* item = addNotifyItem(
        QString("default"),
        NotifyPresent::presentName(NotifyPresent::Passive),
        NotifyAction::SpeakEventName,
        QString::null,
        talkerCode );
    lv->scrollToItem( item );
    lv->setCurrentItem( item );
    // TODO: Is this needed?
    // slotNotifyListView_currentItemChanged();
    configChanged();
}

void KCMKttsMgr::slotNotifyRemoveButton_clicked()
{
    QTreeWidgetItem* item = notifyListView->currentItem();
    if (!item) return;
    QTreeWidgetItem* parentItem = item->parent();
    delete item;
    if (parentItem)
    {
        if (parentItem->childCount() == 0) delete parentItem;
    }
    // TODO: Is this needed?
    slotNotifyListView_currentItemChanged();
    configChanged();
}

void KCMKttsMgr::slotNotifyLoadButton_clicked()
{
    QStringList dataDirs = KGlobal::dirs()->findAllResources("data", "kttsd/notify/");
    QString dataDir;
    if (!dataDirs.isEmpty()) dataDir = dataDirs.last();
    QString filename = KFileDialog::getOpenFileName(
        dataDir,
        "*.xml|" + i18n("file type", "Notification Event List") + " (*.xml)",
        this,
        "event_loadfile");
    if ( filename.isEmpty() ) return;
    QString errMsg = loadNotifyEventsFromFile( filename, true );
    slotNotifyListView_currentItemChanged();
    if ( !errMsg.isEmpty() )
        KMessageBox::sorry( this, errMsg, i18n("Error Opening File") );
    else
        configChanged();
}

void KCMKttsMgr::slotNotifySaveButton_clicked()
{
    QString filename = KFileDialog::getSaveFileName(
        KGlobal::dirs()->saveLocation( "data" ,"kttsd/notify/", false ),
        "*.xml|" + i18n("file type", "Notification Event List") + " (*.xml)",
        this,
        "event_savefile");
    if ( filename.isEmpty() ) return;
    QString errMsg = saveNotifyEventsToFile( filename );
    slotNotifyListView_currentItemChanged();
    if ( !errMsg.isEmpty() )
        KMessageBox::sorry( this, errMsg, i18n("Error Opening File") );
}
