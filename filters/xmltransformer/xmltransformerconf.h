/***************************************************** vim:set ts=4 sw=4 sts=4:
  Generic XML Transformation Filter Configuration class.
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

#ifndef _XMLTRANSFORMERCONF_H_
#define _XMLTRANSFORMERCONF_H_

// Qt includes.
#include <qwidget.h>

// KDE includes.
#include <kconfig.h>
#include <kdebug.h>

// KTTS includes.
#include "filterconf.h"

// XmlTransformer includes.
#include "xmltransformerconfwidget.h"

class XmlTransformerConf : public KttsFilterConf
{
    Q_OBJECT

    public:
        /**
        * Constructor 
        */
        XmlTransformerConf( QWidget *parent, const char *name, const QStringList &args = QStringList() );

        /**
        * Destructor 
        */
        virtual ~XmlTransformerConf();

        /**
        * This method is invoked whenever the module should read its 
        * configuration (most of the times from a config file) and update the 
        * user interface. This happens when the user clicks the "Reset" button in 
        * the control center, to undo all of his changes and restore the currently 
        * valid settings.  Note that KTTSMGR calls this when the plugin is
        * loaded, so it not necessary to call it in your constructor.
        * The plugin should read its configuration from the specified group
        * in the specified config file.
        * @param config      Pointer to a KConfig object.
        * @param configGroup Call config->setGroup with this argument before
        *                    loading your configuration.
        *
        * When a plugin is first added to KTTSMGR, @e load will be called with
        * a Null @e configGroup.  In this case, the plugin will not have
        * any instance-specific parameters to load, but it may still wish
        * to load parameters that apply to all instances of the plugin.
        */
        virtual void load(KConfig *config, const QString &configGroup);

        /**
        * This function gets called when the user wants to save the settings in 
        * the user interface, updating the config files or wherever the 
        * configuration is stored. The method is called when the user clicks "Apply" 
        * or "Ok". The plugin should save its configuration in the specified
        * group of the specified config file.
        * @param config      Pointer to a KConfig object.
        * @param configGroup Call config->setGroup with this argument before
        *                    saving your configuration.
        */
        virtual void save(KConfig *config, const QString &configGroup);

        /** 
        * This function is called to set the settings in the module to sensible
        * default values. It gets called when hitting the "Default" button. The 
        * default values should probably be the same as the ones the application 
        * uses when started without a config file.  Note that defaults should
        * be applied to the on-screen widgets; not to the config file.
        */
        virtual void defaults();

        /**
         * Indicates whether the plugin supports multiple instances.  Return
         * False if only one instance of the plugin can be configured.
         * @return            True if multiple instances are possible.
         */
        virtual bool supportsMultiInstance();

        /**
         * Returns the name of the plugin.  Displayed in Filters tab of KTTSMgr.
         * If there can be more than one instance of a filter, it should return
         * a unique name for each instance.  The name should be translated for
         * the user if possible.  If the plugin is not correctly configured,
         * return an empty string.
         * @return          Filter instance name.
         */
        virtual QString userPlugInName();

    private slots:

    private:
        // Configuration Widget.
        XmlTransformerConfWidget* m_widget;
};

#endif  //_XMLTRANSFORMERCONF_H_