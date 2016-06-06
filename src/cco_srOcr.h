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

#ifndef CCO_SROCR_H_
#define CCO_SROCR_H_

#include <opencv/cv.h>
#include <cluscore/cco.h>
#include <cluscore/cco_vString.h>

enum CCOSROCR_STATUS {
	CCOSROCR_STATUS_SUCCESS = 0,
	CCOSROCR_STATUS_EMPTYCLASS,
	CCOSROCR_STATUS_UNSUPPORTIMAGE,
	CCOSROCR_STATUS_DONOTRECOGNIZE,
	CCOSROCR_STATUS_DONOTFINDCHARACTER,
	CCOSROCR_STATUS_DBDONOTEXIST
};

typedef enum CCOSROCR_STATUS CCOSROCR_STATUS;

#define CCO_SROCR_PROPERTIES \
	CCOSROCR_STATUS (*srOcr_initialize)(void *cco_srOcr, char *configfile);\
	CCOSROCR_STATUS (*srOcr_setImage)(void *cco_srOcr, IplImage *image);\
	CCOSROCR_STATUS (*srOcr_getRecognizeString)(void *cco_srOcr, cco_vString **recognizedString);\
	CCOSROCR_STATUS (*srOcr_setOption)(void *cco_srOcr, char *option);

typedef struct cco_srOcr cco_srOcr;

struct cco_srOcr {
	CCO_PROPERTIES
	CCO_SROCR_PROPERTIES
};

cco_srOcr *cco_srOcr_baseNew(int size);
void cco_srOcr_baseRelease(void *cco);
void cco_srOcr_baseInitialize(cco_srOcr *cco);
void cco_srOcr_baseFinalize(cco_srOcr *cco);
cco_srOcr *cco_srOcr_new();
void cco_srOcr_release(void *cco);

CCOSROCR_STATUS cco_srOcr_initialize(void *cco_srOcr, char *configfile);
CCOSROCR_STATUS cco_srOcr_setImage(void *cco_srOcr, IplImage *imagefile);
CCOSROCR_STATUS cco_srOcr_getRecognizeString(void *cco_srOcr, cco_vString **recognizedString);
CCOSROCR_STATUS cco_srOcr_setOption(void *obj, char *option);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SROCR_PROPERTIES
*/

#endif /* CCO_SROCR_H_ */
