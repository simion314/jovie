/***************************************************** vim:set ts=4 sw=4 sts=4:
  KSpeech
  
  The KDE Text-to-Speech object.
  ------------------------------
  Copyright:
  (C) 2006 by Gary Cramblitt <garycramblitt@comcast.net>
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

#ifndef _KSPEECH_H_
#define _KSPEECH_H_

/**
 * @interface KSpeech
 *
 * kspeech - the KDE Text-to-Speech API.
 *
 * @version 2.0 Draft 1
 *
 * @since KDE 4.0
 *
 * This class defines the DBUS interface for applications desiring to speak text.
 * Applications may speak text by sending DBUS messages to application
 * "org.kde.kttsd", object path "/KSpeech", interface "org.kde.KSpeech".
 *
 * %KTTSD -- the KDE Text-to-Speech Daemon -- is the program that supplies the services
 * in the KDE Text-to-Speech API.
 *
 * @warning The KSpeech interface is still being developed and is likely to change in the future.
 *
 * @section Features
 *
 *   - Priority system for Screen Readers, warnings and messages, while still playing
 *     regular texts.
 *   - Long text is parsed into sentences.  User may backup by sentence,
 *     replay, pause, and stop playing.
 *   - Handles multiple speaking applications.  Speech requests are treated like print jobs.
 *     Jobs may be created, stopped, paused, resumed, and deleted.
 *   - Speak contents of clipboard.
 *   - Speak contents of a file.
 *   - Speak KDE notifications.
 *   - Plugin-based job filtering permits substitution for misspoken words,
 *     abbreviations, etc., transformation of XML or XHTML to SSML, and automatic
 *     choice of appropriate synthesis engine.
 *
 * @section Requirements
 *
 * You may build any KDE application to use KSpeech, since the interface is in kdelibs, but
 * the kdeaccessibility package must be installed for KTTS to function.
 *
 * You will need a speech synthesis engine, such as Festival.  See the KTTS Handbook
 * for the latest information on installing and configuring speech engines and voices
 * with KTTS.
 *
 * @section goals Design Goals
 *
 * The KDE Text-to-Speech API is designed with the following goals:
 *
 *   - Support the features enumerated above.
 *   - Plugin-based architecture for support of a wide variety of speech synthesis
 *     engines and drivers.
 *   - Permit generation of speech from the command line (or via shell scripts)
 *     using the KDE DBUS utilities.
 *   - Provide a lightweight and easily usable interface for applications to
 *     generate speech output.
 *   - Applications need not be concerned about contention over the speech device.
 *   - Provide limited support for speech markup languages, such as %Speech Markup Meta-language (SMML).
 *   - Provide limited support for embedded speech markers.
 *   - Asynchronous to prevent system blocking.
 *   - Plugin-based audio architecture.  Currently supports ALSA or Phonon.
 *
 * Architecturally, applications interface with %KTTSD, which performs queuing,
 * speech job management, plugin management and sentence parsing.  %KTTSD interfaces with a
 * %KTTSD speech plugin(s), which then interfaces with the speech engine(s) or driver(s).
 *
   @verbatim
         application
              ^
              |  via DBUS (the KDE Text-to-Speech API)
              v
            kttsd
              ^
              |  KTTSD plugin API
              v
         kttsd plugin
              ^
              |
              v
        speech engine
   @endverbatim
 *
 * The %KTTSD Plugin API is documented in PluginConf in the kdeaccessibility module.
 *
 * There is a separate GUI application, called kttsmgr, for providing %KTTSD
 * configuration and job management.
 *
 * When a request for speech is made, usally via the say method,
 * a speech job is queued.  The order by which jobs are spoken is determined
 * by their priority (in decreasing priority):
 *   - Screen Reader Output
 *   - Warnings
 *   - Messages
 *   - Text Jobs
 *
 * Screen Reader output.pre-empts any other speech in progress,
 * including other Screen Reader outputs, i.e., it is not a queue.
 * This is reserved for use by Screen Readers.
 *
 * Warnings take priority over Messages, which take priority
 * over text jobs.  Warnings and Messages are spoken when the currently-speaking
 * sentence of a text job is finished.
 *
 * Text Jobs are the lowest priority are used for long text or general
 * TTS.
 *
 * The priority of jobs is determined by the @ref setDefaultPriority method.
 * After setting the priority, all subsequent say commands are queued
 * at that priority.
 *
 * Within a job, the application (and user
 * via the kttsmgr GUI), may back up or advance by sentence, or rewind
 * to the beginning.
 * @see moveRelTextSentence.
 
 * All jobs may be paused, resumed or deleted (stopped) from the queue.
 * @see pause, resume, removeJob, and removeAllJobs.
 *
 * @section cmdline DBUS Command-line Interface
 *
 * Examples of using the KSpeech interface via command-line DBUS follow.
 *
 * To create a text job to be spoken
 *
   @verbatim
     dbus org.kde.kttsd "/KSpeech" say <text> <options>
   @endverbatim
 *
 * where \<text\> is the text to be spoken, and \<options\> is one of the
 * options defined in the @ref SayOptions enum.  Normally, this can be entered
 * as zero.
 *
 * Example.
 *
   @verbatim
     dbus org.kde.kttsd "/KSpeech" say "Hello World." 0
   @endverbatim
 *
 * To stop speaking and delete the last queued job.
 *
   @verbatim
     dbus org.kde.kttsd "/KSpeech" removeJob 0
   @endverbatim
 *
 * Depending upon the speech plugin used, speaking may not immediately stop.
 * The zero argument in this case is the job number to be removed.
 * Zero means "the current job".
 *
 * @section programming Calling KTTSD from a Program
 *
 * There are two methods of making DBUS calls from your application to %KTTSD.
 *
 *   - Manually code them using QDBusInterface object.  See kdebase/konqueror/kttsplugin/khtmlkttsd.cpp
 *     for an example.  This method is recommended if you want to make a few simple calls to %KTTSD.
 *   - Use KSpeechInterface as described below.  This method generates the marshalling code for you
 *     and is recommended for a more complex speech-enabled applications.  kcmkttsmgr in the
 *     kdeaccessibility module is an example that uses this method.
 *
 * @subsection manual_code Manual code
 *
 * Sending a text job to KSpeech is very simple.  Sample code:
 *
    @verbatim
       #include <QtDBus>
       
       QDBusInterface kspeech("org.kde.kttsd", "/KSpeech", "org.kde.KSpeech");
       // Send a string to KTTS and get back a job number.
       kspeech.call("setApplicationName", "MyApp");
       QDBusReply<int> reply = kspeech.call("say", "Hello World.", 0);
    @endverbatim
 *
 * Notice the call to @ref setApplicationName.  All applications should
 * do this before submitting any jobs so that a friendly name will appear
 * in kttsmgr.
 *
 * Here's a slightly more complicated sample that sets the job priority
 * to Message and specifies a language code ("en" or "de", for example):
 *
   @verbatim
     #include <QtDBus>
     #include <kspeech>

     bool kttsdSay (const QString &text, const QString &language) {
       // TODO: Would be better to save off this QDBusInterface pointer and
       // set applicationName and defaults only once.
       QDBusInterface kspeech("org.kde.kttsd", "/KSpeech", "org.kde.KSpeech");
       kspeech.call("setApplicationName", "KMouth");
       kspeech.call("setDefaultTalker", language);
       kspeech.call("setDefaultPriority", KSpeech::jpMessage);
       QDBusReply<int> reply = kspeech.call("say", text, 0);
	    return (reply != 0);
    }
   @endverbatim
 *
 * It is not necessary to call setDefaultTalker and setDefaultPriority
 * prior to each call to say.  These settings remain in effect for
 * all subsequent calls to say.
 *
 * @subsection kspeechinterface Using KSpeechInterface
 *
 * Begin by adding the following command to your CMakeLists.txt file so
 * that the build system will generate kspeechinterface.h and
 * kspeechinterface.cpp for you from the org.kde.KSpeech.xml interface
 * definition file using dbusidl2cpp utility.
 *
   @verbatim
     qt4_add_dbus_interfaces(myapp_SRCS org.kde.KSpeech.xml )
   @endverbatim
 *
 * Substitute your application's SRCS target for "myapp".
 *
 * TODO: At present, to make the command above work, you must copy
 * org.kde.KSpeech.xml from kdelibs/interfaces/kspeech to your source
 * directory, but this will change in the future.
 *
 * In your application's .h file, add the following code to declare
 * a variable to hold an instance of KSpeechInterface object.
 * You can also declare slots to receive signals from %KTTSD.
 * Typically, you will do this as part of a class.
 *
   @verbatim
    #include <QObject>
    #include "kspeechinterface.h"

    class MyClass : public QObject {
    
    Q_OBJECT
    
    public:
        MyClass(QObject *parent=0);
        ~MyClass();
         
    protected Q_SLOTS:
        Q_SCRIPTABLE void jobStateChanged(const QString &appId, int jobNum, int state);

    private:
       org::kde::KSpeech* m_kspeech;
    }
   @endverbatim
 *
 * In the .cpp file, determine if %KTTSD is running, create the
 * KSpeechInterface object, and connect signals to slots like this:
 *
   @verbatim
    MyClass::MyClass(QObject *parent) : QObject(parent), m_kspeech(0) { }
    
    bool MyClass::isKttsdRunning()
    {
        bool isRunning = (QDBus::sessionBus().interface()->isServiceRegistered("org.kde.kttsd"));
        if (isRunning) {
            if (!m_kspeech) {
                m_kspeech = new OrgKdeKSpeechInterface("org.kde.kttsd", "/KSpeech", QDBus::sessionBus());
                m_kspeech->setParent(this);
                m_kspeech->setApplicationName("MyApp");
                connect(m_kspeech, SIGNAL(jobStateChanged(const QString&, int, int)),
                     this, SLOT(jobStateChanged(const QString&, int, int)));
            }
        } else {
            delete m_kspeech;
            m_kspeech = 0;
        }
        return isRunning;
    }
   @endverbatim
 *
 * Notice that the application sets a friendly display name for itself.
 * If this is not done, the dbus connection name (example: ":1.16") will be shown
 * in kttsmgr.
 *
 * To submit a simple job of priority Text using the default talker:
 *
   @verbatim
    if (m_kspeech)
        int jobNum = m_kspeech->say("Hello World", 0);
   @endverbatim
 *
 * The second argument to "say" is used to give hints to %KTTSD about the
 * contents of the text.  See @ref SayOptions.
 *
 * Talkers determine the synthesizer and language that will be used for TTS.
 * To change the talker to a German-speaking one:
 *
   @verbatim
    if (m_kspeech)
        m_kspeech->setDefaultTalker("de");
   @endverbatim
 *
 * All subsequent calls to say will use this talker.  See @ref talkers.
 *
 * If you want to detect if %KTTSD is installed without starting it,
 * use this code.
 *
   @verbatim
     KTrader::OfferList offers = KTrader::self()->query("DCOP/Text-to-Speech", "Name == 'KTTSD'");
     if (offers.count() > 0)
     {
       // KTTSD is installed.
     }
   @endverbatim
 *
 * Typically, you would do this to hide a menu item or button if %KTTSD is not installed.
 *
 * If %KTTSD is not running, you can start it:
 *
 * // TODO: Use DBUS start service or KTrader?
 *
 * To detect if %KTTSD has exited, you can use the @ref kttsdExited signal, or you
 * connect the DBUS serviceUnregistered signal, like this:
 *
   @verbatim
     connect (QDBus::sessionBus().interface(), SIGNAL(serviceUnregistered(const QString&)),
        this, SLOT(slotServiceUnregistered(const QString&)));

    void MyClass::slotServiceUnregistered(const QString& serviceName)
    {
        if (serviceName == "org.kde.kttsd") {
            delete m_kspeech;
            m_kspeech = 0;
        }
    }
   @endverbatim
 *
 * @section signals Signals Emitted by KTTSD
 *
 * %KTTSD emits a number of DBUS signals, which provide information about sentences spoken,
 * jobs started, paused, interrupted, finished, or deleted and markers seen.
 * In general, these signals are broadcast to any application that connects to them.
 * Applications should check the appId argument to determine whether the signal belongs to
 * them or not.
 *
   @verbatim
    void MyClass::jobStateChanged(const QString &appId, int jobNum, int state)
    {
        if (appId != QDBus::sessionBus().baseService()) return;
        if (KSpeech::jsFinished == state)
            // jobNum has finished speaking.
    }

   @endverbatim
 *
 * @see jobStateChanged
 * @see marker
 * @see kttsdExited
 *
 * @section talkers Talkers, Talker Codes, and Plugins
 *
 * Many of the methods permit you to specify a desired "talker".  This
 * may be a simple language code, such as "en" for English, "es" for Spanish, etc.
 * Code as "" to use the default configured talker.
 *
 * Within KttsMgr, the user has the ability to configure more than one talker for each language,
 * with different voices, genders, volumes, and talking speeds.
 *
 * Talker codes serve two functions:
 * - They identify configured plugins, and
 * - They provide a way for applications to specify the desired speaking attributes
 *   that influence the choice of plugin to speak text.
 *
 * A Talker Code consists of a series of XML tags and attributes.
 * An example of a full Talker Code with all attributes specified is
 *
 *   <voice lang="en" name="kal" gender="male"/>
 *   <prosody volume="soft" rate="fast"/>
 *   <kttsd synthesizer="Festival" />
 *
 * (The @e voice and @e prosody tags are adapted from the W3C Speech Synthesis
 * Markup Language (SSML) and Java Speech Markup Language (JSML).
 * The @e kttsd tag is an extension to the SMML and JSML languages to support
 * named synthesizers and text encodings.)
 * %KTTS doesn't really care about the @e voice, @e prosody, and @e kttsd tags.  In fact,
 * they may be omitted and just the attributes specified.  The example above then
 * becomes
 *
 *   lang="en" name="kal" gender="male" volume="soft" rate="fast"
 *   synthesizer="Festival"
 *
 * The attributes may be specified in any order.
 *
 * For clarity, the rest of the discussion
 * will omit the @e voice, @e prosody, and @e kttsd tags.
 *
 * The attributes that make up a talker code are:
 *
 * - @e lang.         Language code and optional country code.
 *                    Examples: en, es, en_US, en_GB. Codes
 *                    are case in-sensitive and hyphen (-) or underscore (_) may be
 *                    used to separate the country code from the language code.
 * - @e synthesizer.  The name of the synthesizer (plugin) used to produce the speech.
 * - @e gender.       May be either "male", "female", or "neutral".
 * - @e name.         The name of the voice code.
 *                    The choice of voice codes is synthesizer-specific.
 * - @e volume.       May be "loud", "medium", or "quiet".  A synonym for "quiet" is
 *                    "soft".
 * - @e rate.         May be "fast", "medium", or "slow".
 *
 * Each plugin, once it has been configured by a user in kttsmgr, returns a
 * fully-specified talker code to identify itself.  If the plugin supports it,
 * the user may configure another instance of the plugin with a different set
 * of attributes.  This is the difference between a "plugin" and a "talker".
 * A talker is a configured instance of a plugin.  Each plugin (if it supports it)
 * may be configured as multiple talkers.
 *
 * When the user configures %KTTSD, she configures one or more talkers and then
 * places them in preferred order, top to bottom in kttsmgr.  In effect,
 * she specifies her preferences for each of the talkers.
 *
 * When applications specify a talker code, they need not (and typically do not)
 * give a full specification.  An example of a talker code with only some of the
 * attributes specified might be
 *
 *   lang="en" gender="female"
 *
 * If the talker code is not in XML attribute format, it assumed to be a @e lang
 * attribute.  So the talker code
 *
 *   en
 *
 * is interpreted as
 *
 *   lang="en"
 *
 * When a program requests a talker code in calls to @ref setDefaultTalker,
 * %KTTSD tries to match the requested talker code to the closest matching
 * configured talker.
 *
 * The @e lang attribute has highest priority (attempting to speak English with
 * a Spanish synthesizer would likely be unintelligible).  So the language
 * attribute is said to have "priority".
 * If an application does not specify a language attribute, a default one will be assumed.
 * The rest of the attributes are said to be "preferred".  If %KTTSD cannot find
 * a talker with the exact preferred attributes requested, the closest matching
 * talker will likely still be understandable.
 *
 * An application may specify that one or more of the attributes it gives in a talker
 * code have priority by preceding each priority attribute with an asterisk.
 * For example, the following talker code
 *
 *  lang="en" gender="*female" volume="soft"
 *
 * means that the application wants to use a talker that supports American English language
 * and Female gender.  If there is more than one such talker, one that supports
 * Soft volume would be preferred.  Notice that a talker configured as English, Male,
 * and Soft volume would not be picked as long as an English Female talker is
 * available.
 *
 * The algorithm used by %KTTSD to find a matching talker is as follows:
 *
 * - If language code is not specified by the application, assume default configured
 *   by user.  The primary language code automatically has priority.
 * - (Note: This is not yet implemented.)
 *   If there are no talkers configured in the language, %KTTSD will attempt
 *   to automatically configure one (see automatic configuraton discussion below)
 * - The talker that matches on the most priority attributes wins.
 * - If a tie, the one that matches on the most preferred attributes wins.
 * - If there is still a tie, the one nearest the top of the kttsmgr display
 *   (first configured) will be chosen.
 *
 * Language codes actually consist of two parts, a language code and an optional
 * country code.  For example, en_GB is English (United Kingdom).  The language code is
 * treated as a priority attribute, but the country code (if specified) is treated
 * as preferred.  So for example, if an application requests the following
 * talker code
 *
 *   lang="en_GB" gender="male" volume="medium"
 *
 * then a talker configured as lang="en" gender="male" volume="medium" would be
 * picked over one configured as lang="en_GB" gender="female" volume="soft",
 * since the former matches on two preferred attributes and the latter only on the
 * preferred attribute GB. An application can override this and make the country
 * code priority with an asterisk.  For example,
 *
 *   lang="*en_GB" gender="male" volume="medium"
 *
 * To specify that American English is priority, put an asterisk in front of
 * en_US, like this.
 *
 *   lang="*en_US" gender="male" volume="medium"
 *
 * Here the application is indicating that a talker that speaks American English
 * has priorty over one that speaks a different form of English.
 *
 * (Note: Not yet implemented).
 * If a language code is specified, and no plugin is currently configured
 * with a matching language code, %KTTSD will attempt to automatically
 * load and configure a plugin to support the requested language.  If
 * there is no such plugin, or there is a plugin but it cannot automatically
 * configure itself, %KTTSD will pick one of the configured plugins using the
 * algorithm given above.
 *
 * Notice that %KTTSD will always pick a talker, even if it is a terrible match.
 * (The principle is that something heard is better than nothing at all.  If
 * it sounds terrible, user will change his configuration.)
 * If an attribute is absolutely mandatory -- in other words the application
 * must speak with the attribute or not at all -- the application can determine if
 * there are any talkers configured with the attribute by calling getTalkers,
 * and if there are none, display an error message to the user.
 *
 * Applications can implement their own talker-matching algorithm by
 * calling @ref getTalkers, then finding the desired talker from the returned
 * list.  When the full talker code is passed in, %KKTSD will find an exact
 * match and use the specified talker.
 *
 * If an application requires a configuration that user has not created,
 * it should display a message to user instructing them to run kttsmgr and
 * configure the desired talker.  (This must be done interactively because
 * plugins often need user assistance locating voice files, etc.)
 *
 * The above scheme is designed to balance the needs
 * of applications against user preferences.  Applications are given the control
 * they @e might need, without unnecessarily burdening the application author.
 * If you are an application author, the above discussion might seem overly
 * complicated.  It isn't really all that complicated.  Here are rules of thumb:
 *
 *   - It is legitimate to not call @ref setDefaultTalker, in which case, the user's default
 *     talker will be used.
 *   - If you know the language code, give that in the talker code, otherwise
 *     leave it out.
 *   - If there is an attribute your application @e requires for proper functioning,
 *     specify that with an asterisk in front of it.  For example, your app might
 *     speak in two different voices, Male and Female.  (Since your
 *     app requires both genders, call getTalkers to determine if both genders
 *     are available, and if not, advise user to configure them.  Better yet,
 *     give the user a choice of available distinquishing attributes
 *     (loud/soft, fast/slow, etc.)
 *   - If there are other attributes you would prefer, specify those without an
 *     asterisk, but leave them out if it doesn't really make any difference
 *     to proper functioning of your application.  Let the user decide them
 *     when they configure %KTTS.
 *
 * One final note about talkers.  %KTTSD does talker matching for each sentence
 * spoken, just before the sentence is sent to a plugin for synthesis.  Therefore,
 * the user can change the effective talker in mid processing of a text job by
 * changing his preferences, or even deleting or adding new talkers to the configuration.
 *
 * @section markup Speech Markup
 *
 * Note: %Speech Markup is not yet fully implemented in %KTTSD.
 *
 * The text passed in a call to @ref say may contain speech markup,
 * provided that the plugin the user has configured supports that markup.  The markup
 * languages and plugins currently supported are:
 *
 *   - %Speech Synthesis Markup language (SSML): Festival and Hadifix.
 *
 * This may change in the future as synthesizers improve.
 *
 * Before including markup in the text sent to kttsd, the application should
 * query whether the currently-configured plugin
 * supports the markup language by calling @getTalkerCapabilities.
 *
 * It it does not support the markup, it will be stripped out of the text.
 *
 * @section markers Support for Markers
 *
 * Note: Markers are not yet fully implemented in %KTTSD.  At present,
 * only mtSentenceBegin and mtSentenceEnd are emitted.
 *
 * When using a speech markup language, such as SSML, the application may embed
 * named markers into the text.  If the user's chosen speech plugin supports markers, %KTTSD
 * will emit @ref marker signals when the speech engine encounters the marker.
 * Depending upon the speech engine and plugin, this may occur either when the speech engine
 * encounters the marker during synthesis from text to speech, or when the speech is actually
 * spoken on the audio device.  The calling application can call the @ref getTalkerCapabilities
 * method to determine if the currently configured plugin supports markers or not.
 *
 * @section sentenceparsing Sentence Parsing
 *
 * Not all speech engines provide robust capabilities for stopping synthesis that is in progress.
 * To compensate for this, %KTTSD parses jobs given to it by the @ref say
 * method into sentences and sends the sentences to the speech
 * plugin one at a time.  In this way, should the user wish to stop the speech
 * output, they can do so, and the worst that will happen is that the last sentence
 * will be completed.  This is called Sentence Boundary Detection (SBD).
 *
 * Sentence Boundary Detection also permits the user to rewind by sentences.
 *
 * The default sentence delimiter used for plain text is as follows:
 *
 *   - A period (.), question mark (?), exclamation mark (!), colon (:), or
 *     semi-colon (;) followed by whitespace (including newline), or
 *   - Two newlines in a row separated by optional whitespace, or
 *   - The end of the text.
 *
 * When given text containing speech markup, %KTTSD automatically determines the markup type
 * and parses based on the sentence semantics of the markup language.
 *
 * An application may change the sentence delimiter by calling @ref setSentenceDelimiter
 * prior to calling @ref say.  Changing the delimiter does not affect other
 * applications.
 *
 * Jobs of priority Screen Reader Output are not split into sentences.
 * For this reason, applicatons should avoid sending long messages
 * of priority jpScreenReaderOutput.
 *
 * Sentence Boundary Detection is implemented as a plugin SBD filter.  See
 * filters for more information.
 *
 * @section filters Filters
 *
 * Users may specify filters in the kttsmgr GUI.  Filters are plugins that modify the text
 * to be spoken or change other characteristics of jobs.  Currently, the following filter plugins
 * are available:
 *
 *   - String Replacer.  Permits users to substitute for mispoken words, or vocalize chat
 *     emoticons.
 *   - XML Transformer.  Given a particular XML or XHTML format, permits conversion of the
 *     XML to SSML (Speech Synthesis Markup Language) using XSLT (XML Style Language - Transforms)
 *     stylesheets.
 *   - Talker Chooser.  Permits users to redirect jobs from one configured Talker to another
 *     based on the contents of the job or application that sent it.
 *
 * Additional plugins may be available in the future.
 *
 * In additional to these regular filters, KTTS also implements Sentence Boundary Detection (SBD)
 * as a plugin filter.  See @ref sentenceparsing for more information.
 *
 * Applications may control filtering by calling @ref setFilteringOn.
 *
 * Note: SBD filters are never applied to Screen Reader jobs.
 *
 * @section authors Authors
 *
 * @author Gary Cramblitt <garycramblitt@comcast.net>
 *
 * @section maintainer Maintainer
 *
 * Gary Cramblitt <garycramblitt@comcast.net>
 */

// Qt includes
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>

class KSpeechPrivate;

/**
* KSpeech -- the KDE Text-to-Speech API.
* Note: Applications do not use the class directly.  Instead,
* use the KSpeechInterface object as described above.
* @see programming
*/
class KSpeech : public QObject
{
Q_OBJECT

public:
    /**
    * @enum JobPriority
    * Determines the priority of jobs submitted by @ref say.
    */
    enum JobPriority
    {
        jpAll                = 0,    /**< All priorities.  Used for information retrieval only. */
        jpScreenReaderOutput = 1,    /**< Screen Reader job. */
        jpWarning            = 2,    /**< Warning job. */
        jpMessage            = 3,    /**< Message job.*/
        jpText               = 4     /**< Text job. */
    };
    
    /**
    * @enum JobState
    * Job states returned by method @ref getJobState.
    */
    enum JobState
    {
        jsQueued      = 0,  /**< Job has been queued but is not yet speakable. */
        jsFiltering   = 1,  /**< Job is being filtered. */
        jsSpeakable   = 2,  /**< Job is speakable, but is not speaking. */
        jsSpeaking    = 3,  /**< Job is currently speaking. */
        jsPaused      = 4,  /**< Job is paused. */
        jsInterrupted = 5,  /**< Job is paused because it has been interrupted by another job. */
        jsFinished    = 6,  /**< Job is finished and is deleteable. */
        jsDeleted     = 7   /**< Job is deleted from the queue. */
    };
    
    /**
    * @enum SayOptions
    * Hints about text content when sending via @ref say.
    */
    enum SayOptions
    {
        soNone      = 0x0000,   /**< No options specified.  Autodetected. */
        soPlainText = 0x0001,   /**< The text contains plain text. */
        soHtml      = 0x0002,   /**< The text contains HTML markup. */
        soSsml      = 0x0004,   /**< The text contains SSML markup. */
        // FUTURE:
        soChar      = 0x0008,   /**< The text should be spoken as individual characters. */
        soKey       = 0x0010,   /**< The text contains a keyboard symbolic key name. */
        soSoundIcon = 0x0020    /**< The text is the name of a sound icon. */
    };
    
    /**
    * @enum TalkerCapabilities1
    * Flags for synthesizer/talker capabilities.
    * All items marked FALSE are hard-coded off at this time.
    */
    enum TalkerCapabilities1
    {
        tcCanListVoices                         = 0x00000001,
        tcCanSetVoiceByProperties               = 0x00000002,
        tcCanGetCurrentVoice                    = 0x00000004,
        tcCanSetRateRelative                    = 0x00000008, /**< FALSE */
        tcCanSetRateAbsolute                    = 0x00000010,
        tcCanGetRateDefault                     = 0x00000020,
        tcCanSetPitchRelative                   = 0x00000040, /**< FALSE */
        tcCanSetPitchAbsolute                   = 0x00000080,
        tcCanGetPitchDefault                    = 0x00000100,
        tcCanSetPitchRangeRelative              = 0x00000200, /**< FALSE */
        tcCanSetPitchRangeAbsolute              = 0x00000400, /**< FALSE */
        tcCanGetPitchRangeDefault               = 0x00000800, /**< FALSE */
        tcCanSetVolumeRelative                  = 0x00001000, /**< FALSE */
        tcCanSetVolumeAbsolute                  = 0x00002000,
        tcCanGetVolumeDefault                   = 0x00004000,
        tcCanSetPunctuationModeAll              = 0x00008000, /**< FALSE */
        tcCanSetPunctuationModeNone             = 0x00010000, /**< FALSE */
        tcCanSetPunctuationModeSome             = 0x00020000, /**< FALSE */
        tcCanSetPunctuationDetail               = 0x00040000, /**< FALSE */
        tcCanSetCapitalLettersModeSpelling      = 0x00080000, /**< FALSE */
        tcCanSetCapitalLettersModeIcon          = 0x00100000, /**< FALSE */
        tcCanSetCapitalLettersModePitch         = 0x00200000, /**< FALSE */
        tcCanSetNumberGrouping                  = 0x00400000, /**< FALSE */
        tcCanSayTextFromPosition                = 0x00800000, /**< FALSE */
        tcCanSayChar                            = 0x01000000, /**< FALSE */
        tcCanSayKey                             = 0x02000000, /**< FALSE */
        tcCanSayIcon                            = 0x04000000, /**< FALSE */
        tcCanSetDictionary                      = 0x08000000, /**< FALSE */
        tcCanRetrieveAudio                      = 0x10000000, /**< FALSE */
        tcCanPlayAudio                          = 0x20000000  /**< FALSE */
    };
    
    /**
    * @enum TalkerCapabilities2
    * All items marked FALSE are hard-coded off at this time.
    */
    enum TalkerCapabilities2
    {
        tcCanReportEventsBySentences            = 0x00000001,
        tcCanReportEventsByWords                = 0x00000002, /**< FALSE */
        tcCanReportCustomIndexMarks             = 0x00000004, /**< FALSE */
        tcHonorsPerformanceGuidelines1          = 0x00000008, /**< FALSE */
        tcHonorsPerformanceGuidelines2          = 0x00000010, /**< FALSE */
        tcHonorsPerformanceGuidelines           = 0x00000018, /**< FALSE */
        tcCanDeferMessage                       = 0x00000020, /**< FALSE */
        tcCanParseSsml                          = 0x00000040,
        tcSupportsMultilingualUtterances        = 0x00000080, /**< FALSE */
        tcCanParseHtml                          = 0x00000100
    };

    /**
    * @enum MarkerType
    * Types of markers emitted by @ref marker signal.
    */
    enum MarkerType
    {
        mtSentenceBegin = 0,
        mtSentenceEnd   = 1,
        mtWordBegin     = 2,
        mtPhonemeBegin  = 3,
        mtCustom        = 4
    };

    /**
    * Constructor.
    * Note: Applications do not create instances of KSpeech.
    * Instead create KSpeechInterface object.  See @programming above.
    */
    KSpeech(QObject *parent=0);

    /**
    * Destructor.
    */
    ~KSpeech();

public: // PROPERTIES
    Q_PROPERTY(bool isSpeaking READ isSpeaking)
    Q_PROPERTY(QString version READ version)

public Q_SLOTS: // METHODS
    /**
    * Returns true if KTTSD is currently speaking.
    * @return               True if currently speaking.
    */
    bool isSpeaking() const;

    /**
    * Returns the version number of KTTSD.
    * @return               Version number string.
    */
    QString version() const;
    /**
    * Returns the friendly display name for the application.
    * @return               Application display name.
    *
    * If application has not provided a friendly name, the DBUS connection name is returned.
    */
    QString applicationName();

    /**
    * Sets a friendly display name for the application.
    * @param applicationName    Friendly name for the application.
    */    
    void setApplicationName(const QString &applicationName);

    /**
    * Returns the default talker for the application.
    * @return                   Talker.
    *
    * The default is "", which uses the default talker configured by user.
    */
    QString defaultTalker();

    /**
    * Sets the default talker for the application.
    * @param defaultTalker      Default talker.  Example: "en".
    */
    void setDefaultTalker(const QString &defaultTalker);

    /**
    * Returns the default priority for speech jobs submitted by the application.
    * @return                   Default job priority.
    *
    * @see JobPriority
    */
    int defaultPriority();

    /**
    * Sets the default priority for speech jobs submitted by the application.
    * @param defaultPriority    Default job priority.
    *
    * @see JobPriority
    */
    void setDefaultPriority(int defaultPriority);

    /**
    * Returns the regular expression used to perform Sentence Boundary
    * Detection (SBD) for the application.
    * @return               Sentence delimiter regular expression.
    *
    * The default sentence delimiter is
      @verbatim
          ([\\.\\?\\!\\:\\;])(\\s|$|(\\n *\\n))
      @endverbatim
    *
    * Note that backward slashes must be escaped.
    *
    * @see sentenceparsing
    */
    QString sentenceDelimiter();

    /**
    * Sets the regular expression used to perform Sentence Boundary
    * Detection (SBD) for the application.
    * @param sentenceDelimiter  Sentence delimiter regular expression.
    */
    void setSentenceDelimiter(const QString &sentenceDelimiter);

    /**
    * Returns whether speech jobs for this application are filtered using configured
    * filter plugins.
    * @return               True if filtering is on.
    *
    * Filtering is on by default.
    */
    bool filteringOn();

    /**
    * Sets whether speech jobs for this application are filtered using configured
    * filter plugins.
    * @param filteringOn    True to set filtering on.
    */
    void setFilteringOn(bool filteringOn);

    /**
    * Returns whether KTTSD will automatically attempt to configure new
    * talkers to meet required talker attributes.
    * @return               True if KTTSD will autoconfigure talkers.
    *
    * @see defaultTalker
    */
    bool autoConfigureTalkersOn();
    
    /** Sets whether KTTSD will automatically attempt to configure new
    * talkers to meet required talker attributes.
    * @param autoConfigureTalkersOn True to enable auto configuration.
    */
    void setAutoConfigureTalkersOn(bool autoConfigureTalkersOn);

    /**
    * Returns whether application is paused.
    * @return               True if application is paused.
    */
    bool isApplicationPaused();
    
    /**
    * Returns the full path name to XSLT file used to convert HTML
    * markup to speakable form.
    * @return               XSLT filename.
    */
    QString htmlFilterXsltFile();

    /**
    * Sets the full path name to an XSLT file used to convert HTML
    * markup to speakable form.
    * @param htmlFilterXsltFile XSLT filename.
    */
    void setHtmlFilterXsltFile(const QString &htmlFilterXsltFile);

    /**
    * Returns the full path name to XSLT file used to convert SSML
    * markup to a speakable form.
    * @return                   XSLT filename.
    */    
    QString ssmlFilterXsltFile();

    /**
    * Sets the full path name to XSLT file used to convert SSML
    * markup to a speakable form.
    * @param ssmlFilterXsltFile XSLT filename.
    */
    void setSsmlFilterXsltFile(const QString &ssmlFilterXsltFile);

    /**
    * Returns whether this is a System Manager application.
    * @return                   True if the application is a System Manager.
    */
    bool isSystemManager();

    /**
    * Sets whether this is a System Manager application.
    * @param isSystemManager    True if this is a System Manager.
    *
    * System Managers are used to control and configure overall TTS output.
    * When True, many of the KSpeech methods alter their behavior.
    */
    void setIsSystemManager(bool isSystemManager);

    /**
    * Creates and starts a speech job.  The job is created at the application's
    * default job priority using the default talker.
    * @param text               The text to be spoken.
    * @param options            Speech options.
    * @return                   Job Number for the new job.
    *
    * @see JobPriority
    * @see SayOptions
    */
    int say(const QString &text, int options);

    /**
    * Creates and starts a speech job from a specified file.
    * @param filename           Full path name of the file.
    * @param encoding           The encoding of the file.  Default UTF-8.
    * @return                   Job Number for the new job.
    *
    * The job is spoken using application's default talker.
    * @see defaultTalker
    *
    * Plain text is parsed into individual sentences using the current sentence delimiter.
    * Call @ref setSentenceDelimiter to change the sentence delimiter prior to calling sayFile.
    * Call @ref getSentenceCount to retrieve the sentence count after calling sayFile.
    *
    * The text may contain speech mark language, such as SMML,
    * provided that the speech plugin/engine support it.  In this case,
    * sentence parsing follows the semantics of the markup language.
    */
    int sayFile(const QString &filename, const QString &encoding);

    /**
    * Submits a speech job from the contents of the clipboard.
    * The job is spoken using application's default talker.
    * @return                   Job Number for the new job.
    *
    * @see defaultTalker
    */
    int sayClipboard();

    /**
    * Pauses speech jobs belonging to the application.
    * When called by a System Manager, pauses all jobs of all applications.
    */
    void pause();

    /**
    * Resumes speech jobs belonging to the application.
    * When called by a System Manager, resumes all jobs of all applications.
    */
    void resume();

    /**
    * Removes the specified job.  If the job is speaking, it is stopped.
    * @param jobNum             Job Number.  If 0, the last job submitted by
    *                           the application.
    */
    void removeJob(int jobNum);

    /**
    * Removes all jobs belonging to the application.
    * When called from a System Manager, removes all jobs of all applications.
    */
    void removeAllJobs();

    /**
    * Returns the number of sentences in a job.
    * @param jobNum             Job Number.  If 0, the last job submitted by
    *                           the application.
    * @return                   Number of sentences in the job.
    */
    int getSentenceCount(int jobNum);

    /**
    * Returns the job number of the currently speaking job (any application).
    * @param                    Job Number
    */
    int getCurrentJob();

    /**
    * Returns the number of jobs belonging to the application
    * with the specified job priority.
    * @param priority           Job Priority.
    * @return                   Number of jobs.
    *
    * If priority is KSpeech::jpAll, returns the number of jobs belonging
    * to the application (all priorities).
    *
    * When called from a System Manager, returns count of all jobs of the
    * specified priority for all applications.
    *
    * @see JobPriority
    */
    int getJobCount(int priority);

    /**
    * Returns a list job numbers for the jobs belonging to the
    * application with the specified priority.
    * @param priority           Job Priority.
    * @return                   List of job numbers.  Note that the numbers
    *                           are strings.
    *
    * If priority is KSpeech::jpAll, returns the job numbers belonging
    * to the application (all priorities).
    *
    * When called from a System Manager, returns job numbers of the
    * specified priority for all applications.
    *
    * @see JobPriority
    */
    QStringList getJobNumbers(int priority);

    /**
    * Returns the state of a job.
    * @param jobNum             Job Number.  If 0, the last job submitted by
    *                           the application.
    * @return                   Job state.
    *
    * @see JobState
    */
    int getJobState(int jobNum);

    /**
    * Get information about a job.
    * @param jobNum             Job Number.  If 0, the last job submitted by
    *                           the application.
    * @return               A QDataStream containing information about the job.
    *                       Blank if no such job.
    *
    * The stream contains the following elements:
    *   - int priority      Job Type.
    *   - int state         Job state.
    *   - QString appId     DBUS senderId of the application that requested the speech job.
    *   - QString talker    Talker code as requested by application.
    *   - int sentenceNum   Current sentence being spoken.  Sentences are numbered starting at 1.
    *   - int sentenceCount Total number of sentences in the job.
    *   - QString applicationName Application's friendly name (if provided by app)
    *
    * If the job is currently filtering, waits for that to finish before returning.
    *
    * The following sample code will decode the stream:
            @verbatim
                QByteArray jobInfo = m_kspeech->getJobInfo(jobNum);
                if (jobInfo != QByteArray()) {
                    QDataStream stream(&jobInfo, QIODevice::ReadOnly);
                    qint32 priority;
                    qint32 state;
                    QString talker;
                    qint32 sentenceNum;
                    qint32 sentenceCount;
                    QString applicationName;
                    stream >> priority;
                    stream >> state;
                    stream >> appId;
                    stream >> talker;
                    stream >> sentenceNum;
                    stream >> sentenceCount;
                    stream >> applicationName;
                };
            @endverbatim
    */
    QByteArray getJobInfo(int jobNum);

    /**
    * Return a sentence of a job.
    * @param jobNum             Job Number.  If 0, the last job submitted by
    *                           the application.
    * @param sentenceNum    Sentence Number.  Sentence numbers start at 1.
    * @return               The specified sentence in the specified job.  If no such
    *                       job or sentence, returns "".
    */
    QString getJobSentence(int jobNum, int sentenceNum);

    /**
    * Return a list of full Talker Codes for configured talkers.
    * @return               List of Talker codes.
    */
    QStringList getTalkerCodes();

    /**
    * Given a talker, returns the Talker ID for the talker.
    * that will speak the job.
    * @param talker         Talker.  Example: "en".
    * @return               Talker ID.  A Talker ID is an internally-assigned
    *                       identifier for a talker.
    *
    * This method is normally used only by System Managers.
    */
    QString talkerToTalkerId(const QString &talker);

    /**
    * Returns a bitarray giving the capabilities of a talker.
    * @param talker         Talker.  Example: "en".
    * @return               A word with bits set according to the capabilities
    *                       of the talker.
    *
    * @see TalkerCapabilities1
    * @see getTalkerCapabilities2
    */
    int getTalkerCapabilities1(const QString &talker);

    /**
    * Returns a bitarray giving the capabilities of a talker.
    * @param talker         Talker.  Example: "en".
    * @return               A word with bits set according to the capabilities
    *                       of the talker.
    *
    * @see TalkerCapabilities2
    * @see getTalkerCapabilities1
    */
    int getTalkerCapabilities2(const QString &talker);

    /**
    * Return a list of the voice codes of voices available in the synthesizer
    * corresponding to a talker.
    * @param talker         Talker.  Example: "synthesizer='Festival'"
    * @return               List of voice codes.
    *
    * Voice codes are synthesizer specific.
    */
    QStringList getTalkerVoices(const QString &talker);

    /**
    * Change the talker of an already-submitted job.
    * @param jobNum             Job Number.  If 0, the last job submitted by
    *                           the application.
    * @param talker             Desired new talker.
    */
    void changeJobTalker(int jobNum, const QString &talker);

    /**
    * Move a job one position down in the queue so that it is spoken later.
    * If the job is already speaking, it is stopped and will resume
    * when processing next gets to it.
    * @param jobNum             Job Number.  If 0, the last job submitted by
    *                           the application.
    *
    * Since there is only one ScreenReaderOutput, this method is meaningless
    * for ScreenReaderOutput jobs.
    */    
    void moveJobLater(int jobNum);

    /**
    * Advance or rewind N sentences in a job.
    * @param jobNum             Job Number.  If 0, the last job submitted by
    *                           the application.
    * @param n              Number of sentences to advance (positive) or rewind (negative)
    *                       in the job.
    * @return               Number of the sentence actually moved to.  Sentence numbers
    *                       are numbered starting at 1.
    *
    * If no such job, does nothing and returns 0.
    * If n is zero, returns the current sentence number of the job.
    * Does not affect the current speaking/not-speaking state of the job.
    *
    * Since ScreenReaderOutput jobs are not split into sentences, this method
    * is meaningless for ScreenReaderOutput jobs.
    */    
    int moveRelSentence(int jobNum, int n);

    /**
    * Display the KttsMgr program so that user can configure KTTS options.
    * Only one instance of KttsMgr is displayed.
    */
    void showManagerDialog();

    /**
    * Shuts down KTTSD.  Do not call this!
    */
    void kttsdExit();
    
    /**
    * Cause KTTSD to re-read its configuration.
    */
    void reinit();
    
    /** Called by DBusAdaptor so that KTTSD knows the application that
    * called it.
    * @param appId              DBUS connection name that called KSpeech.
    */
    void setCallingAppId(const QString& appId);
        
Q_SIGNALS: // SIGNALS
    /**
    * This signal is emitted when KTTSD starts.
    */
    void kttsdStarted();
    
    /**
    * This signal is emitted just before KTTS exits.
    */
    void kttsdExiting();

    /**
    * This signal is emitted each time the state of a job changes.
    * @param appId              The DBUS connection name of the application that
    *                           submitted the job.
    * @param jobNum             Job Number.
    * @param state              Job state.  @ref JobState.
    */
    void jobStateChanged(const QString &appId, int jobNum, int state);

    /**
    * This signal is emitted when a marker is processed.
    * Currently only emits mtSentenceBegin and mtSentenceEnd.
    * @param appId         The DBUS connection name of the application that submitted the job.
    * @param jobNum        Job Number of the job emitting the marker.
    * @param markerType    The type of marker.
    *                      Currently either mtSentenceBegin or mtSentenceEnd.
    * @param markerData    Data for the marker.
    *                      Currently, this is the sentence number of the sentence
    *                      begun or ended.  Sentence numbers begin at 1.
    */
    void marker(const QString &appId, int jobNum, int markerType, const QString &markerData);

protected:

    /**
    * This signal is emitted by KNotify when a notification event occurs.
    */
    void notificationSignal(const QString &event, const QString &fromApp,
                            const QString &text, const QString &sound, const QString &file,
                            const int present, const int level, const int winId, const int eventId );

private slots:
    void slotJobStateChanged(const QString& appId, int jobNum, KSpeech::JobState state);
    void slotMarker(const QString& appId, int jobNum, KSpeech::MarkerType markerType, const QString& markerData);
    void slotFilteringFinished();
    
private:
    /**
    * The DBUS connection name of the last application that called KTTSD.
    */
    QString callingAppId();
    
    /*
    * Checks if KTTSD is ready to speak and at least one talker is configured.
    * If not, user is prompted to display the configuration dialog.
    */
    bool ready();
    
    /**
    * Create and initialize the Configuration Data object.
    */
    bool initializeConfigData();

    /*
    * Create and initialize the SpeechData object.
    */
    bool initializeSpeechData();

    /*
    * Create and initialize the TalkerMgr object.
    */
    bool initializeTalkerMgr();

    /*
    * Create and initialize the speaker.
    */
    bool initializeSpeaker();

    /*
    * If a job number is 0, returns the default job number for a command.
    * Returns the job number of the last job queued by the application, or if
    * no such job, the current job number.
    * @param jobNum          The job number passed in the DBUS message.  May be 0.
    * @return                Default job number.  0 if no such job.
    */
    int applyDefaultJobNum(int jobNum);

    /*
    * Announces an event to kDebug.
    */
    void announceEvent(const QString& slotName, const QString& eventName);
    void announceEvent(const QString& slotName, const QString& eventName, const QString& appId,
        int jobNum, MarkerType markerType, const QString& markerData);
    void announceEvent(const QString& slotName, const QString& eventName, const QString& appId,
        int jobNum, JobState state);

private:
    KSpeechPrivate* d;
};

#endif // _KSPEECH_H_