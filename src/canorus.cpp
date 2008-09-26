/*!
	Copyright (c) 2007, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.

	Licensed under the GNU GENERAL PUBLIC LICENSE. See COPYING for details.
*/

// Python.h needs to be loaded first!
#include "scripting/swigpython.h"
#include "canorus.h"

#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QTranslator>
#include <QLocale>
#include <QMetaMethod>

#include "interface/rtmididevice.h"
#include "ui/settingsdialog.h"
#include "scripting/swigruby.h"
#include "core/sheet.h"
#include "core/settings.h"
#include "core/undo.h"

// define private static members
QApplication *CACanorus::_mainApp;
QList<CAMainWin*> CACanorus::_mainWinList;
CASettings *CACanorus::_settings;
QString CACanorus::_settingsPath;
CAAutoRecovery *CACanorus::_autoRecovery;
CAMidiDevice *CACanorus::_midiDevice;
CAUndo *CACanorus::_undo;
QList<QString> CACanorus::_recentDocumentList;
QString CACanorus::_prevPath;

/*!
	Locates a resource named fileName (relative path) and returns its absolute path of the file
	if found in the following order:
		- passed path as an argument to exe
		- path in user's config file
		- current dir
		- exe dir
		- DEFAULT_DATA_DIR set by compiler

	\sa locateResourceDir()
*/
QList<QString> CACanorus::locateResource(const QString fileName) {
	QList<QString> paths;
	//! \todo Config file implementation
	//! \todo Application path argument
	QString curPath;

	// Try absolute path
	curPath = fileName;
	if (QFileInfo(curPath).exists())
		paths << QFileInfo(curPath).absoluteFilePath();

	// Try current working directory
	curPath = QDir::currentPath() + "/" + fileName;
	if (QFileInfo(curPath).exists())
		paths << QFileInfo(curPath).absoluteFilePath();

	// Try application exe directory
	curPath = QCoreApplication::applicationDirPath() + "/" + fileName;
	if (QFileInfo(curPath).exists())
		paths << QFileInfo(curPath).absoluteFilePath();

#ifdef DEFAULT_DATA_DIR
	// Try compiler defined DEFAULT_DATA_DIR constant (useful on Unix OSes)
	curPath = QString(DEFAULT_DATA_DIR) + "/" + fileName;
	if (QFileInfo(curPath).exists())
		paths << QFileInfo(curPath).absoluteFilePath();

#endif

	// Try the source ui/ directory (images etc.)
	curPath = QCoreApplication::applicationDirPath() + "/ui/" + fileName;
	if (QFileInfo(curPath).exists())
			paths << QFileInfo(curPath).absoluteFilePath();

	// Remove duplicates. Is there a faster way to do this?
	return paths.toSet().toList();
}

/*!
	Finds the resource named fileName and returns its absolute directory.

	\sa locateResource()
*/
QList<QString> CACanorus::locateResourceDir(const QString fileName) {
	QList<QString> paths = CACanorus::locateResource(fileName);
	for (int i=0; i<paths.size(); i++)
		paths[i] = paths[i].left(paths[i].lastIndexOf("/"));

	// Remove duplicates. Is there a faster way to do this?
	return paths.toSet().toList();
}

/*!
	Initializes application properties like application name, home page etc.
*/
void CACanorus::initMain( int argc, char *argv[] ) {
	_autoRecovery = 0;

	// Init main application properties
	QCoreApplication::setOrganizationName("Canorus");
	QCoreApplication::setOrganizationDomain("canorus.org");
	QCoreApplication::setApplicationName("Canorus");
}

void CACanorus::initCommonGUI() {
	// Initialize main window's load/save/import/export dialogs
	CAMainWin::uiSaveDialog = new QFileDialog(0, QObject::tr("Choose a file to save"), settings()->documentsDirectory().absolutePath());
	CAMainWin::uiSaveDialog->setFileMode(QFileDialog::AnyFile);
	CAMainWin::uiSaveDialog->setAcceptMode( QFileDialog::AcceptSave );
	CAMainWin::uiSaveDialog->setFilters( QStringList() << CAFileFormats::CANORUSML_FILTER );
	CAMainWin::uiSaveDialog->setFilters( CAMainWin::uiSaveDialog->filters() << CAFileFormats::CAN_FILTER );
	CAMainWin::uiSaveDialog->selectFilter( CAFileFormats::getFilter( settings()->defaultSaveFormat() ) );

	CAMainWin::uiOpenDialog = new QFileDialog(0, QObject::tr("Choose a file to open"), settings()->documentsDirectory().absolutePath());
	CAMainWin::uiOpenDialog->setFileMode( QFileDialog::ExistingFile );
	CAMainWin::uiOpenDialog->setAcceptMode( QFileDialog::AcceptOpen );
	CAMainWin::uiOpenDialog->setFilters( QStringList() << CAFileFormats::CANORUSML_FILTER ); // clear the * filter
	CAMainWin::uiOpenDialog->setFilters( CAMainWin::uiOpenDialog->filters() << CAFileFormats::CAN_FILTER );
	QString allFilters; // generate list of all files
	for (int i=0; i<CAMainWin::uiOpenDialog->filters().size(); i++) {
		QString curFilter = CAMainWin::uiOpenDialog->filters()[i];
		int left = curFilter.indexOf('(')+1;
		allFilters += curFilter.mid( left, curFilter.size()-left-1 ) + " ";
	}
	allFilters.chop(1);
	CAMainWin::uiOpenDialog->setFilters( QStringList() << QString(QObject::tr("All supported formats (%1)").arg(allFilters)) << CAMainWin::uiOpenDialog->filters() );

	CAMainWin::uiExportDialog = new QFileDialog(0, QObject::tr("Choose a file to export"), settings()->documentsDirectory().absolutePath());
	CAMainWin::uiExportDialog->setFileMode(QFileDialog::AnyFile);
	CAMainWin::uiExportDialog->setAcceptMode( QFileDialog::AcceptSave );
	CAMainWin::uiExportDialog->setFilters( QStringList() << CAFileFormats::LILYPOND_FILTER );
	CAMainWin::uiExportDialog->setFilters( CAMainWin::uiExportDialog->filters() << CAFileFormats::MIDI_FILTER );

	CAMainWin::uiImportDialog = new QFileDialog(0, QObject::tr("Choose a file to import"), settings()->documentsDirectory().absolutePath());
	CAMainWin::uiImportDialog->setFileMode( QFileDialog::ExistingFile );
	CAMainWin::uiImportDialog->setAcceptMode( QFileDialog::AcceptOpen );
}

/*!
	Initializes playback devices.
*/
void CACanorus::initPlayback() {
	qRegisterMetaType< QVector< unsigned char > >( "QVector< unsigned char >" );
	setMidiDevice( new CARtMidiDevice() );
}

/*!
	Opens Canorus config file and loads the settings.
	Config file is always INI file in user's home directory.
	No native formats are used (Windows registry etc.) - this is provided for easier transition of settings between the platforms.

	\sa settings()
*/
CASettingsDialog::CASettingsPage CACanorus::initSettings() {
#ifdef Q_WS_WIN	// M$ is of course an exception
	_settingsPath = QDir::homePath()+"/Application Data/Canorus";
#else	// POSIX systems use the same config file path
	_settingsPath = QDir::homePath()+"/.config/Canorus";
#endif

	_settings = new CASettings( settingsPath()+"/canorus.ini", QSettings::IniFormat );

	return static_cast<CASettingsDialog::CASettingsPage>(_settings->readSettings());
}

/*!
	Initializes scripting and plugins subsystem.
*/
void CACanorus::initScripting() {
#ifdef USE_RUBY
	CASwigRuby::init();
#endif
#ifdef USE_PYTHON
	CASwigPython::init();
#endif
}

/*!
	Initializes recovery saving.
*/
void CACanorus::initAutoRecovery() {
	_autoRecovery = new CAAutoRecovery();
}

void CACanorus::initUndo() {
	_undo = new CAUndo();
}

void CACanorus::insertRecentDocument( QString filename ) {
	if ( recentDocumentList().contains(filename) )
		removeRecentDocument(filename);

	recentDocumentList().prepend(filename);

	if ( recentDocumentList().size() > settings()->maxRecentDocuments() )
		recentDocumentList().removeLast();

	settings()->writeSettings();
}

void CACanorus::addRecentDocument( QString filename ) {
	recentDocumentList() << filename;
}

void CACanorus::removeRecentDocument( QString filename ) {
	recentDocumentList().removeAll(filename);
}

/*!
	Free resources before quitting
*/
void CACanorus::cleanUp()
{
	delete _settings;
	delete CAMainWin::uiSaveDialog;
	delete CAMainWin::uiOpenDialog;
	delete CAMainWin::uiExportDialog;
	delete CAMainWin::uiImportDialog;
	delete _midiDevice;
	autoRecovery()->cleanupRecovery();
	delete _autoRecovery;
	delete _undo;
}

/*!
	Parses the switches and settings command line arguments to application.
	This function sets any settings passed in command line.

	Returns True, if application should resume with loading or False, if such
	a switch was passed.

	\sa parseOpenFileArguments()
*/
bool CACanorus::parseSettingsArguments(int argc, char *argv[]) {
	for (int i=1; i<argc; i++) {
		if ( QString(argv[i])=="--version" ) {
			std::cout << "Canorus - The next generation music score editor" << std::endl
			          << "Version " << CANORUS_VERSION << std::endl;

			return false;
		}
	}

	return true;
}

/*!
	This function parses any arguments which doesn't look like switch or a setting.
	It creates a new main window and opens a file if a file is passed in the command line.
*/
void CACanorus::parseOpenFileArguments(int argc, char *argv[]) {
	for (int i=1; i<argc; i++) {
		if (argv[i][0]!='-') { // automatically treat any argument which doesn't start with '-' to be a file name - \todo
			// passed is not the switch but a file name
			if (!CACanorus::locateResource(argv[i]).size())
				continue;
			QString fileName = CACanorus::locateResource(argv[i]).at(0);

			CAMainWin *mainWin = new CAMainWin();
			mainWin->openDocument(fileName);
			mainWin->show();
		}
	}
}

/*!
	\fn int CACanorus::mainWinCount()

	Returns the number of all main windows.
*/

/*!
	Returns number of main windows which have the given document opened.
*/
int CACanorus::mainWinCount(CADocument *doc) {
	int count=0;
	for (int i=0; i<_mainWinList.size(); i++)
		if (_mainWinList[i]->document()==doc)
			count++;

	return count;
}

/*!
	Rebuilds main windows with the given \a document and its viewports showing the given \a sheet.
	Rebuilds all viewports if no sheet is null.

	\sa rebuildUI(CADocument*), CAMainWin::rebuildUI()
*/
void CACanorus::rebuildUI( CADocument *document, CASheet *sheet ) {
	for (int i=0; i<mainWinCount(); i++)
		if ( mainWinAt(i)->document()==document )
			mainWinAt(i)->rebuildUI(sheet);
}

/*!
	Rebuilds main windows with the given \a document.
	Rebuilds all main windows, if \a document is not given or null.

	\sa rebuildUI(CADocument*, CASheet*), CAMainWin::rebuildUI()
*/
void CACanorus::rebuildUI( CADocument *document ) {
	for (int i=0; i<mainWinCount(); i++) {
		if ( document && mainWinAt(i)->document()==document ) {
			mainWinAt(i)->rebuildUI();
		} else if ( !document )
			mainWinAt(i)->rebuildUI();
	}
}

/*!
	Finds and returns a list of main windows containing the given document.
*/
QList<CAMainWin*> CACanorus::findMainWin(CADocument *document) {
	QList<CAMainWin*> mainWinList;
	for (int i=0; i<mainWinCount(); i++)
		if (mainWinAt(i)->document()==document)
			mainWinList << mainWinAt(i);

	return mainWinList;
}

/*!
	\function void CACanorus::addMainWin( CAMainWin *w )
	Adds an already created main window to the global main window list.

	\sa removeMainWin(), mainWinAt()
*/

/*!
	Searches recursively for all child objects of the given object, and connects matching signals from them to slots of object that follow the following form:
	void on_<widget name>_<signal name>(<signal parameters>);
	Let's assume our object has a child object of type QPushButton with the object name 	button1. The slot to catch the button's clicked() signal would be:
	void on_button1_clicked();

	This enhanced function allows to precisely define in which object the signal (pOS)
	is defined and in which object the slot (poR) is defined.

	\sa QObject::setObjectName()
	\sa QMetaObject::connectSlotsByName(QObject *o)
*/
void CACanorus::connectSlotsByName(QObject *pOS, const QObject *pOR)
{
    if (!pOS || !pOR)
        return;
    const QMetaObject *mo = pOR->metaObject();
    Q_ASSERT(mo);
    const QObjectList list = qFindChildren<QObject *>(pOS, QString());
    for (int i = 0; i < mo->methodCount(); ++i) {
        const char *slot = mo->method(i).signature();
        Q_ASSERT(slot);
        if (slot[0] != 'o' || slot[1] != 'n' || slot[2] != '_')
            continue;
        bool foundIt = false;
        for(int j = 0; j < list.count(); ++j) {
            const QObject *co = list.at(j);
            QByteArray objName = co->objectName().toAscii();
            int len = objName.length();
            if (!len || qstrncmp(slot + 3, objName.data(), len) || slot[len+3] != '_')
                continue;
            const QMetaObject *smo = co->metaObject();
            int sigIndex = smo->indexOfMethod(slot + len + 4);
            if (sigIndex < 0) { // search for compatible signals
                int slotlen = qstrlen(slot + len + 4) - 1;
                for (int k = 0; k < co->metaObject()->methodCount(); ++k) {
                    if (smo->method(k).methodType() != QMetaMethod::Signal)
                        continue;

                    if (!qstrncmp(smo->method(k).signature(), slot + len + 4, slotlen)) {
                        sigIndex = k;
                        break;
                    }
                }
            }
            if (sigIndex < 0)
                continue;
            if (QMetaObject::connect(co, sigIndex, pOR, i)) {
                foundIt = true;
                break;
            }
        }
        if (foundIt) {
            // we found our slot, now skip all overloads
            while (mo->method(i + 1).attributes() & QMetaMethod::Cloned)
                  ++i;
        } else if (!(mo->method(i).attributes() & QMetaMethod::Cloned)) {
            qWarning("QMetaObject::connectSlotsByName: No matching signal for %s", slot);
        }
    }
}

/*!
	This function sets the current path to the directory containing the
	/images folder and stores the current path. It looks the current path,
	canorus exe, data path and source dir.

	This function is usually called when initializing the GUI where icons
	and other GUI resources should be located relative to the current directory.

	\sa CACanorus::restorePath(), CACanorus::locateResourceDir()
 */
bool CACanorus::setImagesPath() {
	_prevPath = QDir::currentPath();

	QList<QString> resourcesLocations = CACanorus::locateResourceDir(QString("images"));
	if (!resourcesLocations.size()) // when Canorus not installed, search the source path
		resourcesLocations = CACanorus::locateResourceDir(QString("ui/images"));

	if ( resourcesLocations.size() ) {
		QDir::setCurrent( resourcesLocations[0] );
		return true;
	} else {
		return false;
	}
}

/*!
	This function restores the path stored when calling setImagesPath().

	\sa CACanorus::setImagesPath(), CACanorus::locateResourceDir()
 */
void CACanorus::restorePath() {
	QDir::setCurrent( _prevPath );
}
