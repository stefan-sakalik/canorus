/*****************************************************************************/
/*									     */
/* This program is free software; you can redistribute it and/or modify it   */
/* under the terms of the GNU General Public License as published by the     */ 
/* Free Software Foundation; version 2 of the License.	                     */
/*									     */
/* This program is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */ 
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General  */
/* Public License for more details.                                          */
/*									     */
/* You should have received a copy of the GNU General Public License along   */
/* with this program; (See "LICENSE.GPL"). If not, write to the Free         */
/* Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA        */
/* 02111-1307, USA.				                             */
/*									     */
/*---------------------------------------------------------------------------*/
/*									     */
/*		Reinhard Katzmann, GERMANY			             */
/*		reinhard@suamor.de					     */
/*									     */
/*									     */
/*****************************************************************************/

#include <QtGui/QtGui>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>
#include <QKeyEvent>
#include <QString>
#include <iostream>

using namespace std;

#include "mainwin.h"

#include "rtmididevice.h"
#include "playback.h"

#include "scrollwidget.h"
#include "engraver.h"
#include "scoreviewport.h"
#include "drawablecontext.h"
#include "drawablestaff.h"

#include "sheet.h"
#include "staff.h"
#include "clef.h"
#include "note.h"

// Constructor
CAMainWin::CAMainWin(QMainWindow *oParent)
  : QMainWindow( oParent )
{
	oMainWin.setupUi( this );
	
	initMidi();
	
	_currentMode = SelectMode;
	_playback = 0;
	newDocument();
}

CAMainWin::~CAMainWin() {
	delete _midiOut;
}

void CAMainWin::newDocument() {
	_document.clear();	//clear the logical part
	clearUI();			//clear the UI part
	
	addSheet();			//add a new empty sheet
	on_actionNew_staff_activated();	//add a new empty 5-line staff to the sheet
}

void CAMainWin::addSheet() {
	CASheet *s = _document.addSheet(QString("Sheet ") + QString::number(_document.sheetCount()+1));
	CAScoreViewPort *v = new CAScoreViewPort(s, 0);
	
	connect(v, SIGNAL(CAMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)));
	connect(v, SIGNAL(CAMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)));
	connect(v, SIGNAL(CAWheelEvent(QWheelEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortWheelEvent(QWheelEvent *, QPoint, CAViewPort *)));
	connect(v, SIGNAL(CAKeyPressEvent(QKeyEvent *, CAViewPort *)), this, SLOT(viewPortKeyPressEvent(QKeyEvent *, CAViewPort *)));
	
	_viewPortList.append(v);
	
	oMainWin.tabWidget->addTab(new CAScrollWidget(v, 0), s->name());
	oMainWin.tabWidget->setCurrentIndex(oMainWin.tabWidget->count()-1);
	
	_activeViewPort = v;
}

void CAMainWin::clearUI() {
	for (int i=0; i<_viewPortList.size(); i++)
		delete _viewPortList[i];

	_viewPortList.clear();
	
	while (oMainWin.tabWidget->count()) {
		delete _currentScrollWidget;
		oMainWin.tabWidget->removeTab(oMainWin.tabWidget->currentIndex());
	}

}

void CAMainWin::on_tabWidget_currentChanged(int idx) {
	_activeViewPort = _currentScrollWidget->lastUsedViewPort();
}

void CAMainWin::on_action_Fullscreen_toggled(bool checked) {
	if (checked)
		this->showFullScreen();
	else
		this->showNormal();
}

void CAMainWin::on_actionSplit_horizontally_activated() {
	CAViewPort *v = (CAViewPort *)_currentScrollWidget->splitHorizontally();
	
	if (v->viewPortType() == CAViewPort::ScoreViewPort) {
		connect((CAScoreViewPort*)v, SIGNAL(CAMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAWheelEvent(QWheelEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortWheelEvent(QWheelEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAKeyPressEvent(QKeyEvent *, CAViewPort *)), this, SLOT(viewPortKeyPressEvent(QKeyEvent *, CAViewPort *)));
	}

	_viewPortList.append(v);
	setMode(_currentMode);	//updates the new viewport border settings
}

void CAMainWin::on_actionSplit_vertically_activated() {
	CAViewPort *v = (CAViewPort *)_currentScrollWidget->splitVertically();
	
	if (v->viewPortType() == CAViewPort::ScoreViewPort) {
		connect((CAScoreViewPort*)v, SIGNAL(CAMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAWheelEvent(QWheelEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortWheelEvent(QWheelEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAKeyPressEvent(QKeyEvent *, CAViewPort *)), this, SLOT(viewPortKeyPressEvent(QKeyEvent *, CAViewPort *)));
	}
	
	_viewPortList.append(v);
	setMode(_currentMode);	//updates the new viewport border settings
}

void CAMainWin::on_actionUnsplit_activated() {
	CAViewPort *v = _currentScrollWidget->unsplit();
	if (v)
		_viewPortList.removeAll(v);
}

void CAMainWin::on_actionNew_viewport_activated() {
	CAViewPort *v = (CAViewPort *)_currentScrollWidget->newViewPort();
	
	if (v->viewPortType() == CAViewPort::ScoreViewPort) {
		connect((CAScoreViewPort*)v, SIGNAL(CAMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAWheelEvent(QWheelEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortWheelEvent(QWheelEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAKeyPressEvent(QKeyEvent *, CAViewPort *)), this, SLOT(viewPortKeyPressEvent(QKeyEvent *, CAViewPort *)));
	}

	_viewPortList.append(v);
	setMode(_currentMode);	//updates the new viewport border settings
}

void CAMainWin::on_actionNew_activated() {
	newDocument();
}

void CAMainWin::on_actionNew_sheet_activated() {
	addSheet();
}

void CAMainWin::on_actionNew_staff_activated() {
	if (_activeViewPort->viewPortType() != CAViewPort::ScoreViewPort)
		return;
	
	CASheet *sheet = ((CAScoreViewPort*)_activeViewPort)->sheet();
	CAStaff *staff = sheet->addStaff();
	
	rebuildScoreViewPorts(sheet);
	
	((CAScoreViewPort*)_activeViewPort)->selectContext(staff);
	((CAScoreViewPort*)_activeViewPort)->repaint();
}

void CAMainWin::setMode(CAMode mode) {
	_currentMode = mode;
	
	QPen p;
	switch (mode) {
		case SelectMode:
			for (int i=0; i<_viewPortList.size(); i++) {
				if (_viewPortList[i]->viewPortType()==CAViewPort::ScoreViewPort) {
					if (((CAScoreViewPort*)_viewPortList[i])->playing()) continue;
					((CAScoreViewPort*)_viewPortList[i])->unsetBorder();
					((CAScoreViewPort*)_viewPortList[i])->setShadowNoteVisible(false);
					statusBar()->showMessage("");
					_insertMusElement = CAMusElement::None;
					((CAScoreViewPort*)_viewPortList[i])->repaint();
				}
			}
			break;
		case InsertMode:
			p.setColor(Qt::blue);
			p.setWidth(3);
			
			for (int i=0; i<_viewPortList.size(); i++) {
				if (_viewPortList[i]->viewPortType()==CAViewPort::ScoreViewPort) {
					if (((CAScoreViewPort*)_viewPortList[i])->playing()) continue;
					((CAScoreViewPort*)_viewPortList[i])->setBorder(p);
					if (_insertMusElement == CAMusElement::Note)
						((CAScoreViewPort*)_viewPortList[i])->setShadowNoteVisible(true);
					((CAScoreViewPort*)_viewPortList[i])->repaint();
				}
			}
			break;
	}
}

void CAMainWin::on_action_Clef_activated() {
	setMode(InsertMode);
	_insertMusElement = CAMusElement::Clef;
}

void CAMainWin::rebuildScoreViewPorts(CASheet *sheet, bool repaint) {
	for (int i=0; i<_viewPortList.size(); i++) {
		((CAScoreViewPort*)(_viewPortList[i]))->update();
		((CAScoreViewPort*)(_viewPortList[i]))->checkScrollBars();
		_viewPortList[i]->repaint();
	}
}

void CAMainWin::viewPortMousePressEvent(QMouseEvent *e, const QPoint coords, CAViewPort *viewPort) {
	_activeViewPort = viewPort;

	if (viewPort->viewPortType() == CAViewPort::ScoreViewPort) {
		CAScoreViewPort *v = (CAScoreViewPort*)viewPort;
		switch (_currentMode) {
			case SelectMode:
				if (e->modifiers()==Qt::ControlModifier) {
					CAMusElement *elt;
					if ( elt = v->removeMElement(coords.x(), coords.y()) )
						delete elt;
						rebuildScoreViewPorts(v->sheet());
						v->repaint();
				} else {
					if ( v->selectMElement(coords.x(), coords.y()) ||
					     v->selectCElement(coords.x(), coords.y()) )
						v->repaint();
				}
				break;
			
			case InsertMode:
				insertMusElementAt( coords, v );
				break;
		}
	}
}

void CAMainWin::insertMusElementAt(const QPoint coords, CAScoreViewPort* v) {
	CADrawableContext *context = v->selectCElement(coords.x(), coords.y());
	CAMusElement *right = v->nearestRightElement(coords.x(), coords.y());
	
	CAStaff *staff=0;
	CADrawableStaff *drawableStaff=0;
	CAClef *clef=0;
	CANote *note=0;
	switch (_insertMusElement) {
		case CAMusElement::Clef:
			if ( (!context) ||
			    (context->context()->contextType() != CAContext::Staff) )
				return;
			
			staff = (CAStaff*)context->context();
			clef = new CAClef(CAClef::Treble, staff, (right?right->timeStart():staff->lastTimeEnd()));
			staff->insertSignBefore(clef, right);
			
			rebuildScoreViewPorts(v->sheet(), true);
			v->selectMElement(clef);
			v->repaint();
			break;
			
		case CAMusElement::Note:
			if ( (!context) ||
			     (context->context()->contextType() != CAContext::Staff) )
				return;

			drawableStaff = (CADrawableStaff*)context;
			staff = drawableStaff->staff();
			note = new CANote(CANote::Quarter,
			                  staff->voiceAt(0),
			                  drawableStaff->calculatePitch(coords.x(), coords.y()),
			                  (right?right->timeStart():staff->lastTimeEnd())
			                 );
			staff->insertNoteBefore(note, right);

			rebuildScoreViewPorts(v->sheet(), true);
			v->selectMElement(note);
			v->repaint();
			break;
	}
}

void CAMainWin::viewPortMouseMoveEvent(QMouseEvent *e, QPoint coords, CAViewPort *v) {
	if ((_currentMode == InsertMode) &&
	    (_insertMusElement == CAMusElement::Note) &&
	    (v->viewPortType()==CAViewPort::ScoreViewPort)
	   ) {
		CAScoreViewPort *c = (CAScoreViewPort*)v;
		CADrawableStaff *s;
		if (c->currentContext()?(c->currentContext()->drawableContextType() == CADrawableContext::DrawableStaff):0)
			s = (CADrawableStaff*)c->currentContext(); 
		else
			return;
		
		//calculate the logical pitch out of absolute world coordinates and the current clef
		int pitch = s->calculatePitch(coords.x(), coords.y());
		
		//write into the main window's status bar the note pitch name
		statusBar()->showMessage(CANote::generateNoteName(pitch));
	}
}

void CAMainWin::viewPortWheelEvent(QWheelEvent *e, QPoint coords, CAViewPort *c) {
	_activeViewPort = c;
	CAScoreViewPort *v = (CAScoreViewPort*) c;

	int val;
	switch (e->modifiers()) {
		case Qt::NoModifier:			//scroll horizontally
			v->setWorldX( v->worldX() - (int)((0.5*e->delta()) / v->zoom()) );
			break;
		case Qt::AltModifier:			//scroll horizontally, fast
			v->setWorldX( v->worldX() - (int)(e->delta() / v->zoom()) );
			break;
		case Qt::ShiftModifier:			//scroll vertically
			v->setWorldY( v->worldY() - (int)((0.5*e->delta()) / v->zoom()) );
			break;
		case 0x0A000000://SHIFT+ALT		//scroll vertically, fast
			v->setWorldY( v->worldY() - (int)(e->delta() / v->zoom()) );
			break;
		case Qt::ControlModifier:		//zoom
			if (e->delta() > 0)
				v->setZoom( v->zoom()*1.1, coords.x(), coords.y() );
			else
				v->setZoom( v->zoom()/1.1, coords.x(), coords.y() );
			
			break;
	}

	v->repaint();
}

void CAMainWin::viewPortKeyPressEvent(QKeyEvent *e, CAViewPort *v) {
	keyPressEvent(e);
}

void CAMainWin::keyPressEvent(QKeyEvent *e) {
	switch (e->key()) {
		case Qt::Key_Escape:
			if ((currentMode()==SelectMode) && (_activeViewPort) && (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort)) {
				((CAScoreViewPort*)_activeViewPort)->selectMElement(0);
				((CAScoreViewPort*)_activeViewPort)->selectContext(0);
			}
			setMode(SelectMode);
			break;
		case Qt::Key_I:
			_insertMusElement = CAMusElement::Note;
			setMode(InsertMode);
			break;
	}
}

void CAMainWin::initMidi() {
	_midiOut = new CARtMidiDevice();
}

void CAMainWin::playbackFinished() {
	_playback->disconnect();
	delete _playback;
	oMainWin.actionPlay->setChecked(false);
}

void CAMainWin::on_actionPlay_toggled(bool checked) {
	if (checked && (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort)) {
		_playback = new CAPlayback((CAScoreViewPort*)_activeViewPort, _midiOut);
		connect(_playback, SIGNAL(done()), this, SLOT(playbackFinished()));
		_playback->start();
	} else {
		_playback->stop();
	}
}
