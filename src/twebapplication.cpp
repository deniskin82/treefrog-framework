/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QTextCodec>
#include <QDateTime>
#include <TWebApplication>
#include <TSystemGlobal>
#include <stdlib.h>

#define DEFAULT_INTERNET_MEDIA_TYPE   "text/plain"
#define DEFAULT_DATABASE_ENVIRONMENT  "product"


static QTextCodec *searchCodec(const char *name)
{
    QTextCodec *c = QTextCodec::codecForName(name);
    return (c) ? c : QTextCodec::codecForLocale();
}


/*!
  \class TWebApplication
  \brief The TWebApplication class provides an event loop for
  TreeFrog applications.
*/

/*!
  Constructor.
*/
TWebApplication::TWebApplication(int &argc, char **argv)
#ifdef TF_USE_GUI_MODULE
    : QApplication(argc, argv),
#else
    : QCoreApplication(argc, argv),
#endif
      dbEnvironment(DEFAULT_DATABASE_ENVIRONMENT),
      appSetting(0),
      dbSettings(0),
      loggerSetting(0),
      validationSetting(0),
      mediaTypes(0),
      codecInternal(0),
      codecHttp(0),
      mpm(Invalid)
{
#if defined(Q_OS_WIN) && QT_VERSION >= 0x050000
    installNativeEventFilter(new TNativeEventFilter);
#endif

    // parse command-line args
    webRootAbsolutePath = ".";
    QStringList args = arguments();
    args.removeFirst();
    for (QStringListIterator i(args); i.hasNext(); ) {
        const QString &arg = i.next();
        if (arg.startsWith('-')) {
            if (arg == "-e" && i.hasNext()) {
                dbEnvironment = i.next();
            }
        } else {
            if (QDir(arg).exists()) {
                webRootAbsolutePath = arg;
                if (!webRootAbsolutePath.endsWith(QDir::separator()))
                    webRootAbsolutePath += QDir::separator();
            }
        }
    }
    
    QDir webRoot(webRootAbsolutePath);
    if (webRoot.exists()) {
        webRootAbsolutePath = webRoot.absolutePath() + QDir::separator();
    }

    // Sets application name
    QString appName = QDir(webRootAbsolutePath).dirName();
    if (!appName.isEmpty()) {
        setApplicationName(appName);
    }
    
    // Creates settings objects
    appSetting = new QSettings(appSettingsFilePath(), QSettings::IniFormat, this);
    loggerSetting = new QSettings(configPath() + "logger.ini", QSettings::IniFormat, this);
    validationSetting = new QSettings(configPath() + "validation.ini", QSettings::IniFormat, this);
    mediaTypes = new QSettings(configPath() + "initializers" + QDir::separator() + "internet_media_types.ini", QSettings::IniFormat, this);

    // Gets codecs
    codecInternal = searchCodec(appSetting->value("InternalEncoding").toByteArray().trimmed().data());
    codecHttp = searchCodec(appSetting->value("HttpOutputEncoding").toByteArray().trimmed().data());

    // Sets codecs for INI files
    loggerSetting->setIniCodec(codecInternal);
    validationSetting->setIniCodec(codecInternal);
    mediaTypes->setIniCodec(codecInternal);

    // DB settings
    QStringList files = appSetting->value("DatabaseSettingsFiles", "database.ini").toString().trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts);
    for (QListIterator<QString> it(files); it.hasNext(); ) {
        const QString &f = it.next();
        QSettings *set = new QSettings(configPath() + f, QSettings::IniFormat, this);
        set->setIniCodec(codecInternal);
        dbSettings.append(set);
    }
    
    // sets a seed for random numbers
    Tf::srandXor128((QDateTime::currentDateTime().toTime_t() << 14) | (QCoreApplication::applicationPid() & 0x3fff));
}


TWebApplication::~TWebApplication()
{ }

/*!
  Enters the main event loop and waits until exit() is called. Returns the
  value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int TWebApplication::exec()
{
    resetSignalNumber();
#ifdef TF_USE_GUI_MODULE
    return QApplication::exec();
#else
    return QCoreApplication::exec();
#endif
}

/*!
  Returns true if the web root directory exists; otherwise returns false.
*/
bool TWebApplication::webRootExists() const
{
    return !webRootAbsolutePath.isEmpty() && QDir(webRootAbsolutePath).exists();
}

/*!
  Returns the absolute path of the public directory.
*/
QString TWebApplication::publicPath() const
{
    return webRootPath() + "public" + QDir::separator();
}

/*!
  Returns the absolute path of the config directory.
*/
QString TWebApplication::configPath() const
{
    return webRootPath() + "config" + QDir::separator();
}

/*!
  Returns the absolute path of the library directory.
*/
QString TWebApplication::libPath() const
{
    return webRootPath()+ "lib" + QDir::separator();
}

/*!
  Returns the absolute path of the log directory.
*/
QString TWebApplication::logPath() const
{
    return webRootPath() + "log" + QDir::separator();
}

/*!
  Returns the absolute path of the plugin directory.
*/
QString TWebApplication::pluginPath() const
{
    return webRootPath() + "plugin" + QDir::separator();
}

/*!
  Returns the absolute path of the tmp directory.
*/
QString TWebApplication::tmpPath() const
{
    return webRootPath() + "tmp" + QDir::separator();
}

/*!
  Returns true if the file of the application settings exists;
  otherwise returns false.
*/
bool TWebApplication::appSettingsFileExists() const
{
    return !appSetting->allKeys().isEmpty();
}

/*!
  Returns the absolute file path of the application settings.
*/
QString TWebApplication::appSettingsFilePath() const
{
    return configPath() + "application.ini";
}

/*!
  Returns a reference to the QSettings object for settings of the
  database \a databaseId.
*/
QSettings &TWebApplication::databaseSettings(int databaseId) const
{
    return *dbSettings[databaseId];
}

/*!
  Returns the number of database settings files set by the setting
  \a DatabaseSettingsFiles in the application.ini. 
*/
int TWebApplication::databaseSettingsCount() const
{
    return dbSettings.count();
}

/*!
  Returns true if all the database settings are valid; otherwise
  returns false.
*/
bool TWebApplication::isValidDatabaseSettings() const
{
    bool valid = false;
    for (int i = 0; i < dbSettings.count(); ++i) {
        QSettings *settings = dbSettings[i];
        settings->beginGroup(dbEnvironment);
        valid = !settings->childKeys().isEmpty();
        settings->endGroup();

        if (!valid)
            break;
    }
    return valid;
}

/*!
  Returns the internet media type associated with the file extension
  \a ext.
*/
QByteArray TWebApplication::internetMediaType(const QString &ext, bool appendCharset)
{
    if (ext.isEmpty())
        return QByteArray();

    QString type = mediaTypes->value(ext, DEFAULT_INTERNET_MEDIA_TYPE).toString();
    if (appendCharset && type.startsWith("text", Qt::CaseInsensitive)) {
        type += "; charset=" + appSetting->value("HtmlContentCharset").toString();
    }
    return type.toLatin1();
}

/*!
  Returns the error message for validation of the given \a rule. These messages
  are defined in the validation.ini. 
*/
QString TWebApplication::validationErrorMessage(int rule) const
{
    validationSetting->beginGroup("ErrorMessage");
    QString msg = validationSetting->value(QString::number(rule)).toString();
    validationSetting->endGroup();
    return msg;
}

/*!
  Returns the module name for multi-processing that is set by the setting
  \a MultiProcessingModule in the application.ini.
*/
TWebApplication::MultiProcessingModule TWebApplication::multiProcessingModule() const
{
    if (mpm == Invalid) {
        QString str = appSettings().value("MultiProcessingModule").toString().toLower();
        if (str == "thread") {
            mpm = Thread;
        } else if (str == "prefork") {
            mpm = Prefork;
        }
    }
    return mpm;
}

/*!
  Returns the maximum number of runnable servers, which is set in the
  application.ini.
*/
int TWebApplication::maxNumberOfServers() const
{
    QString mpm = appSettings().value("MultiProcessingModule").toString().toLower();
    int num = appSettings().value(QLatin1String("MPM.") + mpm + ".MaxServers").toInt();
    Q_ASSERT(num > 0);
    return num;
}

/*!
  Returns the absolute file path of the routes config.
*/
QString TWebApplication::routesConfigFilePath() const
{
    return configPath() + "routes.cfg";
}

/*!
  Returns the absolute file path of the system log, which is set by the
  setting \a SystemLog.FilePath in the application.ini.
*/
QString TWebApplication::systemLogFilePath() const
{
    QFileInfo fi(appSettings().value("SystemLog.FilePath", "log/treefrog.log").toString());
    return (fi.isAbsolute()) ? fi.absoluteFilePath() : webRootPath() + fi.filePath();
}

/*!
  Returns the absolute file path of the access log, which is set by the
  setting \a AccessLog.FilePath in the application.ini.
*/
QString TWebApplication::accessLogFilePath() const
{
    QFileInfo fi(appSettings().value("AccessLog.FilePath", "log/access.log").toString());
    return (fi.isAbsolute()) ? fi.absoluteFilePath() : webRootPath() + fi.filePath();
}

/*!
  Returns the absolute file path of the SQL query log, which is set by the
  setting \a SqlQueryLogFile in the application.ini.
*/
QString TWebApplication::sqlQueryLogFilePath() const
{
    QString path = appSettings().value("SqlQueryLogFile").toString();
    if (!path.isEmpty()) {
        QFileInfo fi(path);
        path = (fi.isAbsolute()) ? fi.absoluteFilePath() : webRootPath() + fi.filePath();
    }
    return path;
}


void TWebApplication::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timer.timerId()) {
        if (signalNumber() >= 0) {
            tSystemDebug("TWebApplication trapped signal  number:%d", signalNumber());
            exit(signalNumber());
        }
    } else {
#ifdef TF_USE_GUI_MODULE
        QApplication::timerEvent(event);
#else
        QCoreApplication::timerEvent(event);
#endif
    }
}


/*!
  \fn QString TWebApplication::webRootPath() const
  Returns the absolute path of the web root directory.
*/

/*!
  \fn QSettings &TWebApplication::appSettings() const
  Returns a reference to the QSettings object for settings of the
  web application, which file is the application.ini.
*/

/*!
  \fn QSettings &TWebApplication::loggerSettings () const
  Returns a reference to the QSettings object for settings of the
  logger, which file is logger.ini.
*/

/*!
  \fn QSettings &TWebApplication::validationSettings () const 
  Returns a reference to the QSettings object for settings of the
  validation, which file is validation.ini.
*/

/*!
  \fn QTextCodec *TWebApplication::codecForInternal() const
  Returns a pointer to the codec used internally, which is set by the
  setting \a InternalEncoding in the application.ini. This codec is used
  by QObject::tr() and toLocal8Bit() functions.
*/

/*!
  \fn QTextCodec *TWebApplication::codecForHttpOutput() const
  Returns a pointer to the codec of the HTTP output stream used by an
  action view, which is set by the setting \a HttpOutputEncoding in
  the application.ini. 
*/

/*!
  \fn QString TWebApplication::databaseEnvironment() const
  Returns the database environment, which string is used to load
  the settings in database.ini. 
  \sa setDatabaseEnvironment(const QString &environment)
*/

/*!
  \fn void TWebApplication::watchConsoleSignal();
  Starts watching console signals i.e.\ registers a routine to handle the
  console signals.
*/

/*!
  \fn void TWebApplication::ignoreConsoleSignal();
  Ignores console signals, i.e.\ delivery of console signals will have no effect
  on the process. 
*/

/*!
  \fn void TWebApplication::watchUnixSignal(int sig, bool watch);
  Starts watching the UNIX signal, i.e.\ registers a routine to handle the
  signal \a sig.
  \sa ignoreUnixSignal()
*/

/*!
  \fn void TWebApplication::ignoreUnixSignal(int sig, bool ignore)
  Ignores UNIX signals, i.e.\ delivery of the signal will have no effect on
  the process. 
  \sa watchUnixSignal()
*/

/*!
  \fn void TWebApplication::timerEvent(QTimerEvent *)
  Reimplemented from QObject::timerEvent().
*/

/*!
  \fn int TWebApplication::signalNumber()
  Returns the integral number of the received signal.
*/
