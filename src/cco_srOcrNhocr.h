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

#ifndef CCO_SROCRNHOCR_H_
#define CCO_SROCRNHOCR_H_

#include "cco_srOcr.h"

#define CCO_SROCRNHOCR_PROPERTIES \
	char *srOcrNhocr_filename;\
	char *srOcrNhocr_option;

typedef struct cco_srOcrNhocr cco_srOcrNhocr;

struct cco_srOcrNhocr {
	CCO_PROPERTIES
	CCO_SROCR_PROPERTIES
	CCO_SROCRNHOCR_PROPERTIES
};

cco_srOcrNhocr *cco_srOcrNhocr_baseNew(int size);
void cco_srOcrNhocr_baseRelease(void *cco);
void cco_srOcrNhocr_baseInitialize(cco_srOcrNhocr *cco);
void cco_srOcrNhocr_baseFinalize(cco_srOcrNhocr *cco);
cco_srOcrNhocr *cco_srOcrNhocr_new();
void cco_srOcrNhocr_release(void *cco);

CCOSROCR_STATUS cco_srOcrNhocr_initialize(void *cco_srOcr, char *configfile);
CCOSROCR_STATUS cco_srOcrNhocr_setImage(void *cco_srOcr, char *imagefile);
CCOSROCR_STATUS cco_srOcrNhocr_setOption(void *obj, char *option);
CCOSROCR_STATUS cco_srOcrNhocr_getRecognizeString(void *cco_srOcr, cco_vString **recognizedString);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SROCR_PROPERTIES
CCOINHERITANCE:CCO_SROCRNHOCR_PROPERTIES
*/

#endif /* CCO_SROCRNHOCR_H_ */
