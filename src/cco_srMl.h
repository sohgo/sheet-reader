/*
 * Sheet-reader for Shinsai FaxOCR
 *
 * Copyright (C) 2009-2011 National Institute of Public Health, Japan.
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

#ifndef CCO_SRML_H_
#define CCO_SRML_H_

#include <cluscore/cco.h>
#include <cluscore/cco_vXml.h>
#include <cluscore/cco_redblacktree.h>
#include "cco_srMlSheet.h"

#define CCO_SRML_PROPERTIES \
	cco_vXml *srMl_xml;\
	cco_redblacktree *srMl_sheets;\

typedef struct cco_srMl cco_srMl;

struct cco_srMl {
	CCO_PROPERTIES
	CCO_SRML_PROPERTIES
};

typedef enum CCOSRML_STATUS CCOSRML_STATUS;

enum CCOSRML_STATUS {
	CCOSRML_STATUS_SUCCESS = 0,
	CCOSRML_STATUS_ERROR,
	CCOSRML_STATUS_NOTREADEDXML,
	CCOSRML_STATUS_NOTFOUNDUID,
	CCOSRML_STATUS_NOTFOUNDSID
};

cco_srMl *cco_srMl_baseNew(int size);
void cco_srMl_baseRelease(void *cco);
void cco_srMl_baseInitialize(cco_srMl *cco);
void cco_srMl_baseFinalize(cco_srMl *cco);
cco_srMl *cco_srMl_new();
void cco_srMl_release(void *cco);

cco_srMlSheet * cco_srMl_getSheet(cco_srMl *xml, cco_vString *id);
CCOSRML_STATUS cco_srMl_readFile(cco_srMl *xml, char *filenaame);
int cco_srMl_readDirectory(cco_srMl *obj, char *directorynaame, int depth);
int cco_srMl_countSheets(cco_srMl *obj);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SRML_PROPERTIES
*/

#endif /* CCO_SRML_H_ */
