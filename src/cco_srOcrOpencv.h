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

#ifndef CCO_SROCROPENCV_H_
#define CCO_SROCROPENCV_H_

#include "cco_srOcr.h"
#include <opencv/cv.h>

#define CCO_SROCROPENCV_PROPERTIES \
	IplImage *srOcrOpencv_img;\
	int srOcrOpencv_debug;\
    CvSeq *srOcrOpencv_conturs;\
    CvSeq *srOcrOpencv_conturs_approx;\
    CvMemStorage *srOcrOpencv_strage;\
    CvMemStorage *srOcrOpencv_strage_approx;

typedef struct cco_srOcrOpencv cco_srOcrOpencv;

struct cco_srOcrOpencv {
	CCO_PROPERTIES
	CCO_SROCR_PROPERTIES
	CCO_SROCROPENCV_PROPERTIES
};

cco_srOcrOpencv *cco_srOcrOpencv_baseNew(int size);
void cco_srOcrOpencv_baseRelease(void *cco);
void cco_srOcrOpencv_baseInitialize(cco_srOcrOpencv *cco);
void cco_srOcrOpencv_baseFinalize(cco_srOcrOpencv *cco);
cco_srOcrOpencv *cco_srOcrOpencv_new();
void cco_srOcrOpencv_release(void *cco);

CCOSROCR_STATUS cco_srOcrOpencv_initialize(void *cco_srOcr, char *configfile);
CCOSROCR_STATUS cco_srOcrOpencv_setImage(void *cco_srOcr, char *imagefile);
CCOSROCR_STATUS cco_srOcrOpencv_getRecognizeString(void *cco_srOcr, cco_vString **recognizedString);

CCOSROCR_STATUS cco_srOcrOpencv_readImage(cco_srOcrOpencv *cco_srOcr, char *file);
CCOSROCR_STATUS cco_srOcrOpencv_recognizeImage(cco_srOcrOpencv *cco_srOcr);
int cco_srOcrOpencv_showImageNow_withImage(cco_srOcrOpencv *obj, char* windowname, IplImage *img);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SROCR_PROPERTIES
CCOINHERITANCE:CCO_SROCROPENCV_PROPERTIES
*/

#endif /* CCO_SROCROPENCV_H_ */
