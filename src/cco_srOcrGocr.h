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

#ifndef CCO_SROCRGOCR_H_
#define CCO_SROCRGOCR_H_

#include "cco_srOcr.h"

#define CCO_SROCRGOCR_PROPERTIES \
	IplImage *srOcrGocr_image;\
	char *srOcrGocr_option;\
	char *srOcrGocr_db;\
	char *srOcrGocr_aoption;\
	char *srOcrGocr_dustsize;

typedef struct cco_srOcrGocr cco_srOcrGocr;

struct cco_srOcrGocr {
	CCO_PROPERTIES
	CCO_SROCR_PROPERTIES
	CCO_SROCRGOCR_PROPERTIES
};

cco_srOcrGocr *cco_srOcrGocr_baseNew(int size);
void cco_srOcrGocr_baseRelease(void *cco);
void cco_srOcrGocr_baseInitialize(cco_srOcrGocr *cco);
void cco_srOcrGocr_baseFinalize(cco_srOcrGocr *cco);
cco_srOcrGocr *cco_srOcrGocr_new();
void cco_srOcrGocr_release(void *cco);

CCOSROCR_STATUS cco_srOcrGocr_initialize(void *cco_srOcr, char *configfile);
CCOSROCR_STATUS cco_srOcrGocr_setImage(void *cco_srOcr, IplImage *image);
CCOSROCR_STATUS cco_srOcrGocr_setOption(void *obj, char *option);
CCOSROCR_STATUS cco_srOcrGocr_getRecognizeString(void *cco_srOcr, cco_vString **recognizedString);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SROCR_PROPERTIES
CCOINHERITANCE:CCO_SROCRGOCR_PROPERTIES
*/

#endif /* CCO_SROCRGOCR_H_ */
