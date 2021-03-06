/*!
	Copyright (c) 2009, Reinhard Katzmann, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.

	Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
*/

#ifndef _CASINGLEACTION_H_
#define _CASINGLEACTION_H_

#include <QAction>
#include <QShortcut>

class QString;

// One action based on QAction, Midi and QString. Contains all
// information (including description) for one control command
class CASingleAction
{
public:
	// Constructor / Desctructor
	CASingleAction( QObject * );
    virtual ~CASingleAction();

	// Getter methods for all single action parameters
    inline QString getCommandName(bool ampersand = false)     { return ampersand ? _oCommandNameNoAmpersand : _oCommandName; }
	inline QString getDescription() { return _oDescription; }
    inline QString getShortCutAsString() { return _oShortCut; }
    inline QString getMidiKeySequence() { return _oMidiKeySequence; }
    inline bool    getMidiShortCutCombined() { return _bMidiShortCutCombined; }
    // Application-specific Getter (Refs, so no Setter required)
    inline QAction      *getAction() { return _pAction; }
    inline QKeySequence &getSysShortCut() { return _oSysShortCut; }
    inline QList<int>   &getMidiKeyParameters() { return _oMidiKeyParameters; }
	
	// Setter methods
    void setCommandName( QString oCommandName );
	void setDescription( QString oDescription );
    void setShortCutAsString( QString oShortCut );
    void setMidiKeySequence( QString oMidiKeySequence, bool combined = false );
    //void setAction(QAction *pAction);

    QAction    *newAction(QObject *parent = 0);
    static void fromQAction(const QAction &action, CASingleAction &sAction);

protected:
    // Action parameters to be stored / loaded via Settings Dialog
    QString   _oCommandName;
    QString   _oCommandNameNoAmpersand;
    QString   _oDescription;
    QString   _oShortCut;
    QString   _oMidiKeySequence;
    bool      _bMidiShortCutCombined;
    QAction  *_pAction;
    // ShortCut, Midi Key Sequence for the application
    QKeySequence _oSysShortCut;
    QList<int>   _oMidiKeyParameters;
    bool      m_localCreated = false;
};

#endif // _CASINGLEACTION_H_
