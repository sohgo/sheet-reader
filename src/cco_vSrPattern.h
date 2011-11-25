/* * Shinsai FaxOCR * * Copyright (C) 2009-2011 National Institute of
Public Health, Japan. * All rights Reserved. * * This program is free
software; you can redistribute it and/or modify * it under the terms
of the GNU General Public License as published by * the Free Software
Foundation * * This program is distributed in the hope that it will be
useful, * but WITHOUT ANY WARRANTY; without even the implied warranty
of * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the *
GNU General Public License for more details. * * You should have
received a copy of the GNU General Public License * along with this
program; if not, write to the Free Software * Foundation, Inc., 59
Temple Place, Suite 330, Boston, MA  02111-1307  USA * */

#ifndef CCO_VSRPATTERN_H_
#define CCO_VSRPATTERN_H_

#include <cluscore/cco_v.h>
#include <cluscore/cco_arraylist.h>

#define CCO_VSRPATTERN_PROPERTIES \
	float vSrPattern_x;\
	float vSrPattern_y;\
	float vSrPattern_width;\
	float vSrPattern_height;

typedef struct cco_vSrPattern cco_vSrPattern;

struct cco_vSrPattern {
	CCO_PROPERTIES
	CCO_V_PROPERTIES
	CCO_VSRPATTERN_PROPERTIES
};

cco_vSrPattern *cco_vSrPattern_baseNew(int size);
void cco_vSrPattern_baseRelease(void *cco);
void cco_vSrPattern_baseInitialize(cco_vSrPattern *cco);
void cco_vSrPattern_baseFinalize(cco_vSrPattern *cco);
cco_vSrPattern *cco_vSrPattern_new();
void cco_vSrPattern_release(void *cco);

int cco_vSrPattern_compere(void *cco_v1, void *cco_v2);
int cco_vSrPattern_hash(void *cco_v, int salt);
char *cco_vSrPattern_getCstring(void *cco_v);

int cco_vSrPattern_set(cco_vSrPattern *cco_vSrPattern, float x, float y, float width, float height);
int cco_vSrPattern_setInInt(cco_vSrPattern *cco_vSrPattern, int x, int y, int width, int height);
int cco_vSrPattern_isInside(cco_vSrPattern *rect_parent, cco_vSrPattern *rect_child);
int cco_vSrPattern_isSquare(cco_vSrPattern *obj);
int cco_vSrPattern_isRectangle1to2(cco_vSrPattern *obj);
int cco_vSrPattern_isPattern(cco_vSrPattern *obj, cco_arraylist *insideboxs);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_V_PROPERTIES
CCOINHERITANCE:CCO_VSRPATTERN_PROPERTIES
*/

#endif /* CCO_VSRPATTERN_H_ */
