/*
 * Sheet-reader for Shinsai FaxOCR
 *
 * Copyright (C) 2009-2013 National Institute of Public Health, Japan.
 * All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef CCO_SRMLSHEET_H_
#define CCO_SRMLSHEET_H_

#include <cluscore/cco.h>
#include <cluscore/cco_vXml.h>
#include <cluscore/cco_arraylist.h>

#define CCO_SRMLSHEET_PROPERTIES \
	cco_vXml *srMlSheet_xml;\
	cco_vString *srMlSheet_id;\
	int srMlSheet_blockWidth;\
	int srMlSheet_blockHeight;\
	cco_arraylist *srMlSheet_cellWidth_list;\
	cco_arraylist *srMlSheet_cellHeight_list;\

typedef struct cco_srMlSheet cco_srMlSheet;

struct cco_srMlSheet {
	CCO_PROPERTIES
	CCO_SRMLSHEET_PROPERTIES
};

cco_srMlSheet *cco_srMlSheet_baseNew(int size);
void cco_srMlSheet_baseRelease(void *cco);
void cco_srMlSheet_baseInitialize(cco_srMlSheet *cco);
void cco_srMlSheet_baseFinalize(cco_srMlSheet *cco);
cco_srMlSheet *cco_srMlSheet_new();
void cco_srMlSheet_release(void *cco);

int cco_srMlSheet_setXml(cco_srMlSheet *obj, cco_vXml *xml);
int cco_srMlSheet_setId(cco_srMlSheet *obj, cco_vString *str);
int cco_srMlSheet_setWidth(cco_srMlSheet *obj, cco_vString *str);
int cco_srMlSheet_setHeight(cco_srMlSheet *obj, cco_vString *str);
int cco_srMlSheet_setCellWidth(cco_srMlSheet *obj, cco_vString *str, int index);
int cco_srMlSheet_setCellHeight(cco_srMlSheet *obj, cco_vString *str, int index);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SRMLSHEET_PROPERTIES
*/

#endif /* CCO_SRMLSHEET_H_ */
