/** @file engraver.cpp
 * 
 * Copyright (c) 2006, Matevž Jekovec, Canorus development team
 * All Rights Reserved. See AUTHORS for a complete list of authors.
 * 
 * Licensed under the GNU GENERAL PUBLIC LICENSE. See COPYING for details.
 */

#include <QList>
#include <QMap>
#include <iostream>
#include "engraver.h"

#include "scoreviewport.h"
#include "drawablestaff.h"
#include "drawableclef.h"
#include "drawablenote.h"

#include "sheet.h"
#include "staff.h"
#include "voice.h"

#define INITIAL_X_OFFSET 20
#define MINIMUM_SPACE 10

void CAEngraver::reposit(CAScoreViewPort *v) {
	int i;
	CASheet *sheet = v->sheet();
	
	//list of all the music element lists (ie. streams) taken from all the contexts
	QList<QList<CAMusElement*>*> musStreamList; 

	int dy = 50;
	QMap<CAContext*, CADrawableContext *> drawableContextMap;
	
	for (int i=0; i < sheet->contextList()->size(); i++, dy+=200) {
		if (sheet->contextList()->at(i)->contextType() == CAContext::Staff) {
			CAStaff *staff = ((CAStaff*)(sheet->contextList()->at(i)));
			drawableContextMap[staff] = new CADrawableStaff(staff, 0, dy);
			v->addCElement(drawableContextMap[staff]);
			
			//add all the voices lists to the common list
			for (int j=0; j < staff->voiceCount(); j++) {
				musStreamList << staff->voiceAt(j)->musElementList();
			}
			
		}
	}
	
	int streams = musStreamList.size();
	int streamsIdx[streams]; for (int i=0; i<streams; i++) streamsIdx[i] = 0;
	int streamsX[streams]; for (int i=0; i<streams; i++) streamsX[i] = INITIAL_X_OFFSET;
	CAClef *lastClef[streams]; for (int i=0; i<streams; i++) lastClef[i] = 0;

	int timeStart = 0;
	bool done = false;
	while (!done) {
		//Synchronize minimum X-es between the contexts
		int maxX = 0;
		for (int i=0; i<streams; i++) maxX = (streamsX[i] > maxX) ? streamsX[i] : maxX;
		for (int i=0; i<streams; i++) streamsX[i] = maxX;
		
		//if all the indices are at the end of the streams, finish.
		int idx;
		for (idx=0; (idx < streams) && (streamsIdx[idx] == musStreamList[idx]->size()); idx++);
		if (idx==streams) { done=true; continue; }

		//go through all the streams and remember the time of the soonest element that will happen
		for (int idx=0; idx < streams; idx++) {
			if ( (musStreamList[idx]->size() > streamsIdx[idx]) && ((!timeStart) || (musStreamList[idx]->at(streamsIdx[idx])->timeStart() < timeStart)) )
				timeStart = musStreamList[idx]->at(streamsIdx[idx])->timeStart();
		}
		//timeStart now holds the nearest next time we're going to draw
		
		
		//go through all the streams and check if the following element has this time
		CAMusElement *elt;
		CADrawableContext *drawableContext;
		for (int i=0; i < streams; i++) {
			//loop until the element has come, which has bigger timeStart
			//multiple elements can have the same start time. eg. Clef + Key signature + Time signature + First note
			while ( (musStreamList[i]->size() > streamsIdx[i]) &&
			        ((elt = musStreamList[i]->at(streamsIdx[i]))->timeStart() == timeStart)
			      ) {
				drawableContext = drawableContextMap[elt->context()];
				switch ( elt->musElementType() ) {
					case CAMusElement::Note:
						CADrawableNote *note;
						note = new CADrawableNote(
							(CANote*)elt,
							drawableContext,
							streamsX[i],
							((CADrawableStaff*)drawableContext)->calculateCenterYCoord((CANote*)elt, lastClef[i])
						);

						v->addMElement(note);

						streamsX[i] += (note->neededWidth() + MINIMUM_SPACE);
						break;
						
					case CAMusElement::Clef:
						CADrawableClef *clef = new CADrawableClef(
							(CAClef*)elt,
							drawableContext,
							streamsX[i],
							drawableContext->yPos()
						);
						
						v->addMElement(clef);
						lastClef[i] = clef->clef();
						
						streamsX[i] += (clef->neededWidth() + MINIMUM_SPACE);
						break;
						
				}
				
				streamsIdx[i] = streamsIdx[i] + 1;
			}
		}
	}
}
