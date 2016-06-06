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

#ifndef CCO_SROCRKOCR_H_
#define CCO_SROCRKOCR_H_

#include "cco_srOcr.h"

#define CCO_SROCRKOCR_PROPERTIES \
	IplImage *srOcrKocr_image;\
	char *srOcrKocr_option;\
	char *srOcrKocr_db;\
	char *srOcrKocr_aoption;\
	char *srOcrKocr_dustsize;

typedef struct cco_srOcrKocr cco_srOcrKocr;

struct cco_srOcrKocr {
	CCO_PROPERTIES
	CCO_SROCR_PROPERTIES
	CCO_SROCRKOCR_PROPERTIES
};

cco_srOcrKocr *cco_srOcrKocr_baseNew(int size);
void cco_srOcrKocr_baseRelease(void *cco);
void cco_srOcrKocr_baseInitialize(cco_srOcrKocr *cco);
void cco_srOcrKocr_baseFinalize(cco_srOcrKocr *cco);
cco_srOcrKocr *cco_srOcrKocr_new();
void cco_srOcrKocr_release(void *cco);

CCOSROCR_STATUS cco_srOcrKocr_initialize(void *cco_srOcr, char *configfile);
CCOSROCR_STATUS cco_srOcrKocr_setImage(void *cco_srOcr, IplImage *image);
CCOSROCR_STATUS cco_srOcrKocr_setOption(void *obj, char *option);
CCOSROCR_STATUS cco_srOcrKocr_getRecognizeString(void *cco_srOcr, cco_vString **recognizedString);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SROCR_PROPERTIES
CCOINHERITANCE:CCO_SROCRKOCR_PROPERTIES
*/

#endif /* CCO_SROCRKOCR_H_ */
