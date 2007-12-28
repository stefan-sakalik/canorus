class QThread {
public:
	bool isRunning();
	bool wait ( unsigned long time = ULONG_MAX );
protected:
	void run()=0;
};

class QXmlDefaultHandler {
};

class QObject {
};

// core
%include "scripting/context.i"
%include "scripting/document.i"
%include "scripting/muselement.i"
%include "scripting/sheet.i"
%include "scripting/voice.i"
%include "scripting/playable.i"
%include "scripting/staff.i"
%include "scripting/note.i"
%include "scripting/slur.i"
%include "scripting/rest.i"
%include "scripting/keysignature.i"
%include "scripting/timesignature.i"
%include "scripting/clef.i"
%include "scripting/barline.i"

%include "scripting/mark.i"

%include "scripting/lyricscontext.i"
%include "scripting/syllable.i"

%include "scripting/functionmarking.i"
%include "scripting/functionmarkingcontext.i"

// io
%include "scripting/importexport.i"

// playback
%include "scripting/playback.i"
%include "scripting/mididevice.i"
%include "scripting/rtmididevice.i"
