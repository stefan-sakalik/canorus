/*!
	Copyright (c) 2007, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.

	Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
*/

#ifndef IMPORT_H_
#define IMPORT_H_

#include "core/file.h"

class CADocument;
class CASheet;
class CAStaff;
class CAVoice;
class CALyricsContext;
class CAFunctionMarkContext;

class CAImport : public CAFile {
#ifndef SWIG
	Q_OBJECT
#endif
public:
	CAImport( QTextStream *stream=0 );
	CAImport( const QString stream );
	virtual ~CAImport();

	void setStreamFromFile( const QString filename );
	QString fileName();

	virtual const QString readableStatus();
	void importDocument();
	void importSheet();
	void importStaff();
	void importVoice();
	void importLyricsContext();
	void importFunctionMarkContext();

	inline CADocument *importedDocument() { return _importedDocument; }
	inline CASheet *importedSheet()       { return _importedSheet; }
	inline CAStaff *importedStaff()       { return _importedStaff; }
	inline CAVoice *importedVoice()       { return _importedVoice; }
	inline CALyricsContext *importedLyricsContext() { return _importedLyricsContext; }
	inline CAFunctionMarkContext *importedFunctionMarkContext() { return _importedFunctionMarkContext; }

#ifndef SWIG
signals:
	void documentImported( CADocument* );
	void sheetImported( CASheet* );
	void staffImported( CAStaff* );
	void voiceImported( CAVoice* );
	void lyricsContextImported( CALyricsContext* );
	void functionMarkContextImported( CAFunctionMarkContext* );

	void importDone( int status );
#endif

protected:
	virtual CADocument      *importDocumentImpl()      { setStatus(0); return 0; }
	virtual CASheet         *importSheetImpl()         { setStatus(0); return 0; }
	virtual CAStaff         *importStaffImpl()         { setStatus(0); return 0; }
	virtual CAVoice         *importVoiceImpl()         { setStatus(0); return 0; }
	virtual CALyricsContext *importLyricsContextImpl() { setStatus(0); return 0; }
	virtual CAFunctionMarkContext *importFunctionMarkContextImpl() { setStatus(0); return 0; }
#ifndef SWIG
	QTextStream& in() { return *stream(); }
#else
	QTextStream& _in() { return *stream(); }
#endif

protected:
	QString _fileName;

private:
	inline void setImportedDocument( CADocument *doc ) { _importedDocument = doc; }
	inline void setImportedSheet( CASheet *sheet ) { _importedSheet = sheet; }
	inline void setImportedStaff( CAStaff *staff ) { _importedStaff = staff; }
	inline void setImportedVoice( CAVoice *voice ) { _importedVoice = voice; }
	inline void setImportedLyricsContext( CALyricsContext *lc ) { _importedLyricsContext = lc; }
	inline void setImportedFunctionMarkContext( CAFunctionMarkContext *fmc ) { _importedFunctionMarkContext = fmc; }

	CADocument      *_importedDocument;
	CASheet         *_importedSheet;
	CAStaff         *_importedStaff;
	CAVoice         *_importedVoice;
	CALyricsContext *_importedLyricsContext;
	CAFunctionMarkContext *_importedFunctionMarkContext;

	enum CAImportPart {
		Undefined,
		Document,
		Sheet,
		Staff,
		Voice,
		LyricsContext,
		FunctionMarkContext
	};

	void run();
	inline void setImportPart( CAImportPart part ) { _importPart = part; }
	inline CAImportPart importPart() { return _importPart; }

	CAImportPart _importPart;
};

#endif /* IMPORT_H_ */
