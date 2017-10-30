/**
 ******************************************************************************
 *
 * @file       main.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @brief      Main() file
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   app GCS main application group
 * @{
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#define COMMAND_LINE_OVERRIDE "override"
#define COMMAND_LINE_CLEAN_CONFIG "reset-config"
#define COMMAND_LINE_NO_LOAD "no-load"
#define COMMAND_LINE_TEST "test"
#define COMMAND_LINE_MULTIPLE_OK "multiple-ok"
#define COMMAND_LINE_PLUGIN_OPTION "plugin-option"

#include "utils/xmlconfig.h"
#include "utils/pathutils.h"
#include <runguard/runguard.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/iplugin.h>

#include "libcrashreporter-qt/libcrashreporter-handler/Handler.h"

#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtCore/QSettings>
#include <QtCore/QRegularExpression>
#include <QtCore/QVariant>

#include <QMessageBox>
#include <QApplication>
#include <QMainWindow>
#include <QPixmap>
#include "customsplash.h"
#include <QBitmap>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QLoggingCategory>

#include <iostream>

#include GCS_VERSION_INFO_FILE

#define USE_CRASHREPORTING
#ifdef Q_OS_WIN
#ifndef _MSC_VER
#undef USE_CRASHREPORTING
#endif
#endif

enum { OptionIndent = 4, DescriptionIndent = 24 };

static const char *corePluginNameC = "Core";

typedef QList<ExtensionSystem::PluginSpec *> PluginSpecSet;

// Helpers for displaying messages. Note that there is no console on Windows.
#ifdef Q_OS_WIN
// Format as <pre> HTML
static inline void toHtml(QString &t)
{
    t.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    t.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    t.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    t.insert(0, QLatin1String("<html><pre>"));
    t.append(QLatin1String("</pre></html>"));
}

static void displayHelpText(QString t) // No console on Windows.
{
    toHtml(t);
    QMessageBox::information(0, GCS_PROJECT_BRANDING_PRETTY, t);
}

static void displayError(const QString &t) // No console on Windows.
{
    QMessageBox::critical(0, GCS_PROJECT_BRANDING_PRETTY, t);
}

#else

static void displayHelpText(const QString &t)
{
    qWarning("%s", qPrintable(t));
}

static void displayError(const QString &t)
{
    qCritical("%s", qPrintable(t));
}

#endif

static void printVersion(const ExtensionSystem::PluginSpec *coreplugin,
                         const ExtensionSystem::PluginManager &pm)
{
    QString version;
    QTextStream str(&version);
    str << '\n'
        << GCS_PROJECT_BRANDING_PRETTY " GCS " << coreplugin->version() << " based on Qt "
        << qVersion() << "\n\n";
    pm.formatPluginVersions(str);
    str << '\n' << coreplugin->copyright() << '\n';
    displayHelpText(version);
}

static void printHelp(const QString &a0, const ExtensionSystem::PluginManager &pm)
{
    QString help;
    QTextStream str(&help);
    str << a0 << '\n' << "Plugin Options:";
    pm.formatPluginOptions(str, OptionIndent, DescriptionIndent);
    displayHelpText(help);
}

static inline QString msgCoreLoadFailure(const QString &why)
{
    return QCoreApplication::translate("Application", "Failed to load core: %1").arg(why);
}

static inline QStringList getPluginPaths()
{
    QStringList rc;
    // Figure out root:  Up one from 'bin'
    QDir rootDir = QApplication::applicationDirPath();
    rootDir.cdUp();
    const QString rootDirPath = rootDir.canonicalPath();
    // 1) "plugins" (Win/Linux)
    QString pluginPath = rootDirPath;
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String(GCS_LIBRARY_BASENAME);
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("plugins");
    rc.push_back(pluginPath);
    // 2) "plugins" in build tree (Win/Linux)
    pluginPath = rootDirPath;
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String(GCS_LIBRARY_BASENAME);
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String(GCS_PROJECT_BRANDING);
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("plugins");
    rc.push_back(pluginPath);
    // 3) "PlugIns" (OS X)
    pluginPath = rootDirPath;
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("Plugins");
    rc.push_back(pluginPath);
    return rc;
}

#ifdef Q_OS_MAC
#define SHARE_PATH "/../Resources"
#else
#define SHARE_PATH "/../share"
#endif

static void overrideSettings(QCommandLineParser &parser, QSettings *settings)
{
    QMap<QString, QString> settingOptions;
    // Options like -DMy/setting=test
    QRegExp rx("([^=]+)=(.*)");

    foreach (QString option, parser.values(COMMAND_LINE_OVERRIDE)) {
        if (rx.indexIn(option) > -1) {
            settingOptions.insert(rx.cap(1), rx.cap(2));
        }
    }
    if (parser.isSet(COMMAND_LINE_CLEAN_CONFIG)) {
        settings->clear();
    }

    QList<QString> keys = settingOptions.keys();
    foreach (QString key, keys) {
        settings->setValue(key, settingOptions.value(key));
    }
    settings->sync();
}

int main(int argc, char **argv)
{
#ifdef Q_OS_MAC
    // increase the number of file that can be opened in GCS
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit(RLIMIT_NOFILE, &rl);
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);

    if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_8) {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");

        QSettings tmpSettings;

        bool value = true;

        tmpSettings.setValue(QString("NSAppSleepDisabled"), value);
    }
#endif

    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(GCS_PROJECT_BRANDING_PRETTY " GCS");
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();
    QCommandLineOption showVersionOption("version",
                                         QCoreApplication::translate("main", "Show GCS version"));
    parser.addOption(showVersionOption);

    QCommandLineOption overrideOption(
        QStringList() << "o" << COMMAND_LINE_OVERRIDE,
        QCoreApplication::translate("main", "Overrides a configuration setting."),
        QCoreApplication::translate("main", "setting=value"), "");
    parser.addOption(overrideOption);
    QCommandLineOption cleanConfig(
        QStringList() << "r" << COMMAND_LINE_CLEAN_CONFIG,
        QCoreApplication::translate("main", "Resets the configuration file to default values."));
    parser.addOption(cleanConfig);
    QCommandLineOption noLoadOption(
        QStringList() << "n" << COMMAND_LINE_NO_LOAD,
        QCoreApplication::translate("main", "Skips loading of the plugin."),
        QCoreApplication::translate("main", "plugin name"), "");
    parser.addOption(noLoadOption);
    QCommandLineOption doTestsOption(
        QStringList() << "t" << COMMAND_LINE_TEST,
        QCoreApplication::translate("main", "Runs tests on the plugin. (Requires compiling with -D "
                                            "WITH_TESTS, look in the uploader plugin for example "
                                            "usage). Special argument 'all' will run all tests."),
        QCoreApplication::translate("main", "plugin name"), "");
    parser.addOption(doTestsOption);
    QCommandLineOption pluginOption(
        QStringList() << "p" << COMMAND_LINE_PLUGIN_OPTION,
        QCoreApplication::translate("main", "Passes an option to a plugin."),
        QCoreApplication::translate("main", "option_name=option_value"), "");
    QCommandLineOption multipleInstancesOption(
        QStringList() << "m" << COMMAND_LINE_MULTIPLE_OK,
        QCoreApplication::translate("main", "Allows multiple instances to run at once"));
    parser.addOption(multipleInstancesOption);

    // The options are passed to the plugin init as a QStringList i.e. >> iplugin::initialize(const
    // QStringList &arguments, QString *errorString)
    // These options need to be set in the pluginspec file (look in the Core.pluginspec file for an
    // example)
    parser.addOption(pluginOption);
    parser.addPositionalArgument(
        "config", QCoreApplication::translate("main", "Use the specified configuration file."),
        QCoreApplication::translate("main", "config file"));

    if (!parser.parse(QCoreApplication::arguments())) {
        displayError(parser.errorText());
        displayHelpText(parser.helpText());
        return 1;
    }

#ifdef USE_CRASHREPORTING
    // Only use crash reporter if we're not doing unit tests
    // This means we won't hang CI, and QtTest will give us a nice stack trace
    if (parser.values(doTestsOption).isEmpty()) {
        QString dirName(GCS_REVISION_PRETTY);
        dirName = dirName.replace("%@%", "_");
        // Limit to alphanumerics plus dots, because this will be a filename
        // component.
        dirName = dirName.replace(QRegularExpression("[^A-Za-z0-9.]+"), "_");
        dirName = QDir::tempPath() + QDir::separator() + GCS_PROJECT_BRANDING + "_" + dirName;
        QDir().mkdir(dirName);
        new CrashReporter::Handler(dirName, true, "crashreporterapp");
    }
#endif

    RunGuard &guard = RunGuard::instance(QStringLiteral(GCS_PROJECT_BRANDING)
                                         + QStringLiteral("_gcs_run_guard"));

    if (!guard.tryToRun()) {
        std::cerr << "GCS already running!" << std::endl;

        if (!parser.isSet(multipleInstancesOption)) {
            return 1;
        }
    }

    // suppress SSL warnings generated during startup (handy if you want to use QT_FATAL_WARNINGS)
    QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");

    // disable QML disk caching, this caches paths in qmlc files which break when the GCS
    // installation is moved. TODO: see if we can work around that
    qputenv("QML_DISABLE_DISK_CACHE", "true");

    QString locale = QLocale::system().name();

    // Must be done before any QSettings class is created
    QSettings::setPath(XmlConfig::XmlSettingsFormat, QSettings::SystemScope,
                       QCoreApplication::applicationDirPath() + QLatin1String(SHARE_PATH));

    QString settingsFilename;
    QStringList positionalArguments = parser.positionalArguments();
    {
        QSettings *settings;
        if (positionalArguments.length() > 0) {
            if (QFile::exists(positionalArguments.at(0)))
                settings = new QSettings(positionalArguments.at(0), XmlConfig::XmlSettingsFormat);
            else {
                displayError(
                    QString("Error loading configuration file:%0").arg(positionalArguments.at(0)));
                return 1;
            }
        } else
            settings = new QSettings(XmlConfig::XmlSettingsFormat, QSettings::UserScope,
                                     QLatin1String(GCS_PROJECT_BRANDING),
                                     QLatin1String(GCS_PROJECT_BRANDING "_config"));
        if (settings->status() != QSettings::NoError) {
            displayError(
                QString("Error parsing the configuration file:%0").arg(settings->fileName()));
            return 1;
        }
        overrideSettings(parser, settings);
        locale = settings->value("General/OverrideLanguage", locale).toString();
        settingsFilename = settings->fileName();
        delete settings;
    }
    QTranslator translator;
    QTranslator qtTranslator;

    QPixmap pixmap(":/images/gcs_splashscreen.png");
    CustomSplash splash(pixmap);
    if (!parser.isSet(versionOption) && !parser.isSet(helpOption)) {
        splash.show();
        splash.showMessage("Loading translations");
        qApp->processEvents();
    }
    const QString &gcsTranslationsPath =
        QCoreApplication::applicationDirPath() + QLatin1String(SHARE_PATH "/translations");
    if (translator.load(QLatin1String("dronin_") + locale, gcsTranslationsPath)) {
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &qtTrFile = QLatin1String("qt_") + locale;
        // Binary installer puts Qt tr files into creatorTrPath
        if (qtTranslator.load(qtTrFile, qtTrPath)
            || qtTranslator.load(qtTrFile, gcsTranslationsPath)) {
            QCoreApplication::installTranslator(&translator);
            QCoreApplication::installTranslator(&qtTranslator);
        } else {
            translator.load(QString()); // unload()
        }
    }
    app.setProperty("qtc_locale", locale); // Do we need this?

    // Load
    ExtensionSystem::PluginManager pluginManager;
    Utils::PathUtils().setSettingsFilename(settingsFilename);
    pluginManager.setFileExtension(QLatin1String("pluginspec"));

    const QStringList pluginPaths = getPluginPaths();
    pluginManager.setPluginPaths(pluginPaths);

    splash.showMessage("Parsing command line options");
    qApp->processEvents();
    QStringList parsingErrors = pluginManager.parseOptions(
        parser.values(pluginOption), parser.values(doTestsOption), parser.values(noLoadOption));
    if (!parsingErrors.isEmpty()) {
        displayError("Plugin options parsing failed with the following errors:");
        foreach (QString str, parsingErrors) {
            displayError(str);
        }
        return 1;
    }
    const PluginSpecSet plugins = pluginManager.plugins();
    ExtensionSystem::PluginSpec *coreplugin = 0;
    foreach (ExtensionSystem::PluginSpec *spec, plugins) {
        if (spec->name() == QLatin1String(corePluginNameC)) {
            coreplugin = spec;
            break;
        }
    }
    if (!parser.isSet(versionOption) && !parser.isSet(helpOption)) {
        splash.showMessage(QLatin1String("Checking core plugin"));
        qApp->processEvents();
    }
    if (!coreplugin) {
        const QString reason =
            QCoreApplication::translate("Application", "Could not find 'Core.pluginspec' in %1")
                .arg(pluginPaths.join(QLatin1String(",")));
        displayError(msgCoreLoadFailure(reason));
        return 1;
    }
    if (coreplugin->hasError()) {
        displayError(msgCoreLoadFailure(coreplugin->errorString()));
        return 1;
    }
    if (parser.isSet(versionOption)) {
        printVersion(coreplugin, pluginManager);
        return 0;
    }

    if (parser.isSet(helpOption)) {
        printHelp(parser.helpText(), pluginManager);
        return 0;
    }

    QObject::connect(&pluginManager, &ExtensionSystem::PluginManager::splashMessages, &splash,
                     &CustomSplash::showMessage);
    QObject::connect(&pluginManager, &ExtensionSystem::PluginManager::hideSplash, &splash,
                     &CustomSplash::hide);
    QObject::connect(&pluginManager, &ExtensionSystem::PluginManager::showSplash, &splash,
                     &CustomSplash::show);

    pluginManager.loadPlugins();
    {
        QStringList errors, plugins;
        for (const auto p : pluginManager.plugins()) {
            if (p->hasError()) {
                errors.append(p->errorString());
                plugins.append(p->name());
            }
        }

        if (!errors.isEmpty()) {
            QString msg(QCoreApplication::tr(
                        "Some problems were encountered whilst loading the following plugins: "));
            msg.append(plugins.join(QStringLiteral(", ")));
            QMessageBox msgBox(QMessageBox::Warning,
                               QCoreApplication::tr("Plugin loader messages"),
                               msg, QMessageBox::Ok);
            msgBox.setDetailedText(errors.join(QStringLiteral("\n\n")));
            msgBox.exec();
        }
    }

    QTimer::singleShot(100, &pluginManager, SLOT(startTests()));
    splash.close();
    return app.exec();
}
