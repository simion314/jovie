/***************************************************** vim:set ts=4 sw=4 sts=4:
  Generic Talker Chooser Filter Configuration class.
  -------------------
  Copyright:
  (C) 2005 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: Gary Cramblitt <garycramblitt@comcast.net>
 ******************************************************************************/

/******************************************************************************
 *                                                                            *
 *    This program is free software; you can redistribute it and/or modify    *
 *    it under the terms of the GNU General Public License as published by    *
 *    the Free Software Foundation; version 2 of the License.                 *
 *                                                                            *
 ******************************************************************************/

#ifndef _TALKERCHOOSERPROC_H_
#define _TALKERCHOOSERPROC_H_

// KTTS includes.
#include "filterproc.h"

class TalkerChooserProc : virtual public KttsFilterProc
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    TalkerChooserProc( QObject *parent, const char *name, const QStringList &args = QStringList() );

    /**
     * Destructor.
     */
    virtual ~TalkerChooserProc();

    /**
     * Initialize the filter.
     * @param config          Settings object.
     * @param configGroup     Settings Group.
     * @return                False if filter is not ready to filter.
     *
     * Note: The parameters are for reading from kttsdrc file.  Plugins may wish to maintain
     * separate configuration files of their own.
     */
    virtual bool init(KConfig *config, const QString &configGroup);

     /**
      * Returns True if the plugin supports asynchronous processing,
      * i.e., supports asyncConvert method.
      * @return                        True if this plugin supports asynchronous processing.
      *
      * If the plugin returns True, it must also implement @ref getState .
      * It must also emit @ref filteringFinished when filtering is completed.
      * If the plugin returns True, it must also implement @ref stopFiltering .
      * It must also emit @ref filteringStopped when filtering has been stopped.
      */
    virtual bool supportsAsync();

    /**
     * Convert input, returning output.  Runs synchronously.
     * @param inputText         Input text.
     * @param talkerCode        TalkerCode structure for the talker that KTTSD intends to
     *                          use for synthing the text.  Useful for extracting hints about
     *                          how to filter the text.  For example, languageCode.
     * @param appId             The DCOP appId of the application that queued the text.
     *                          Also useful for hints about how to do the filtering.
     */
    virtual QString convert(const QString& inputText, TalkerCode* talkerCode, const QCString& appId);

private:

    QString m_re;
    QStringList m_appIdList;
    QString m_languageCode;
    QString m_synth;
    QString m_gender;
    QString m_volume;
    QString m_rate;

};

#endif      // _TALKERCHOOSERPROC_H_