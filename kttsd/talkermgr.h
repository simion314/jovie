/***************************************************** vim:set ts=4 sw=4 sts=4:
  Manages all the Talker (synth) plugins.
  -------------------
  Copyright:
  (C) 2004-2005 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: Gary Cramblitt <garycramblitt@comcast.net>
 ******************************************************************************/

/******************************************************************************
 *                                                                            *
 *    This program is free software; you can redistribute it and/or modify    *
 *    it under the terms of the GNU General Public License as published by    *
 *    the Free Software Foundation; either version 2 of the License.          *
 *                                                                            *
 ******************************************************************************/

#ifndef _TALKERMGR_H_
#define _TALKERMGR_H_

// Qt includes.
#include <qstring.h>
#include <qstringlist.h>
#include <qvaluevector.h>
#include <qmap.h>
#include <qptrlist.h>

// KTTS includes.
#include "talkercode.h"
#include "pluginproc.h"

class TalkerMgr: public QObject
{
public:

    /**
     * Structure containing information for a talker (plugin).
     */
    struct TalkerInfo{
        PlugInProc* plugIn;                  /* Instance of the plugin, i.e., the Talker. */
        QString talkerID;                    /* ID of the talker. */
        QString talkerCode;                  /* The Talker's Talker Code in XML format. */
        TalkerCode parsedTalkerCode;         /* The Talker's Talker Code parsed into individual attributes. */
    };

    /**
     * Constructor.
     */
    TalkerMgr(QObject *parent = 0, const char *name = 0);

    /**
     * Destructor.
     */
    ~TalkerMgr();

    /**
     * Load all the configured plug ins populating loadedPlugIns
     */
    int loadPlugIns(KConfig* config);

    /**
     * Get a list of the talkers configured in KTTS.
     * @return               A QStringList of fully-specified talker codes, one
     *                       for each talker user has configured.
     */
    QStringList getTalkers();

    /**
     * Returns a list of all the loaded plugins.
     */
    QPtrList<PlugInProc> getLoadedPlugIns();

    /**
     * Given a talker code, returns pointer to the closest matching plugin.
     * @param talker          The talker (language) code.
     * @return                Index to m_loadedPlugins array of Talkers.
     *
     * If a plugin has not been loaded to match the talker, returns the default
     * plugin.
     */
    int talkerToPluginIndex(const QString& talker) const;

    /**
     * Given a talker code, returns pointer to the closest matching plugin.
     * @param talker          The talker (language) code.
     * @return                Pointer to closest matching plugin.
     *
     * If a plugin has not been loaded to match the talker, returns the default
     * plugin.
     */
    PlugInProc* talkerToPlugin(const QString& talker) const;

    /**
     * Given a talker code, returns the parsed TalkerCode of the closest matching Talker.
     * @param talker          The talker (language) code.
     * @return                Parsed TalkerCode structure.
     *
     * If a plugin has not been loaded to match the talker, returns the default
     * plugin.
     *
     * The returned TalkerCode is a copy and should be destroyed by caller.
     *
     * TODO: When picking a talker, %KTTSD will automatically determine if text contains
     * markup and pick a talker that supports that markup, if available.  This
     * overrides all other attributes, i.e, it is treated as an automatic "top priority"
     * attribute.
     */
    TalkerCode* talkerToTalkerCode(const QString& talker);

    /**
     * Given a Talker Code, returns the Talker ID of the talker that would speak
     * a text job with that Talker Code.
     * @param talkerCode     Talker Code.
     * @return               Talker ID of the talker that would speak the text job.
     */
    QString talkerCodeToTalkerId(const QString& talkerCode);

    /**
     * Get the user's default talker.
     * @return               A fully-specified talker code.
     *
     * @see talkers
     * @see getTalkers
     */
    QString userDefaultTalker() const;

    /**
     * Determine whether the currently-configured speech plugin supports a speech markup language.
     * @param talker         Code for the talker to do the speaking.  Example "en".
     *                       If NULL, defaults to the user's default talker.
     * @param markupType     The kttsd code for the desired speech markup language.
     * @return               True if the plugin currently configured for the indicated
     *                       talker supports the indicated speech markup language.
     * @see kttsdMarkupType
     */
    bool supportsMarkup(const QString& talker, const uint markupType) const;

private:

    /**
     * Array of the loaded plug ins for different Talkers.
     */
    QValueVector<TalkerInfo> m_loadedPlugIns;

    /**
     * Cache of talker codes and index of closest matching Talker.
     */
    QMap<QString,int> m_talkerToPlugInCache;
};

#endif      // _TALKERMGR_H_