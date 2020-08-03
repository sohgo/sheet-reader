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

#ifndef CCO_SRANALYZER_H_
#define CCO_SRANALYZER_H_

#include <cluscore/cco.h>
#include <cluscore/cco_vString.h>
#include <cluscore/cco_redblacktree.h>
#include <cluscore/cco_vXml.h>
#include <opencv/cv.h>
#include "cco_vSrPattern.h"
#include "cco_srMl.h"
#include "cco_srConf.h"
#include "cco_srOcr.h"

#define CCO_SRANALYZER_PROPERTIES \
	IplImage *srAnalyzer_img;\
	cco_srConf *srAnalyzer_srconf;\
	cco_srMl *srAnalyzer_srml;\
	cco_vSrPattern *srAnalyzer_pattern_upperleft;\
	cco_vSrPattern *srAnalyzer_pattern_bottomleft;\
	cco_vSrPattern *srAnalyzer_pattern_upperright;\
	cco_vString *srAnalyzer_uid;\
	cco_vString *srAnalyzer_sid;\
	cco_vString *srAnalyzer_sender;\
	cco_vString *srAnalyzer_receiver;\
	cco_vString *srAnalyzer_save_prefix;\
	cco_vString *srAnalyzer_output_directory;\
	cco_vString *srAnalyzer_backup_image;\
	char *srAnalyzer_date_string;\
	cco_vString *srAnalyzer_pid;\
	cco_vString *srAnalyzer_hostname;\
	cco_vString *srAnalyzer_outXml;\
	cco_vString *srAnalyzer_outSql;\
	cco_vString *srAnalyzer_outProp;\
	cco_vString *srAnalyzer_out;\
	cco_vString *srAnalyzer_ocr_type;\
	cco_srOcr *srAnalyzer_ocr_obj;\
	cco_vXml *srAnalyzer_analyzedSrml;\
	cco_redblacktree *srAnalyzer_analyzedData;\
	int srAnalyzer_debug;

typedef struct cco_srAnalyzer cco_srAnalyzer;

struct cco_srAnalyzer {
	CCO_PROPERTIES
	CCO_SRANALYZER_PROPERTIES
};

enum CCOSRANALYZER_STATUS {
	CCOSRANALYZER_STATUS_SUCCESS = 0,
	CCOSRANALYZER_STATUS_NOT_FOUND_ADJUSTPATTERN,
	CCOSRANALYZER_STATUS_MANY_FOUND_ADJUSTPATTERN,
	CCOSRANALYZER_STATUS_NOT_LOAD_IMAGE,
	CCOSRANALYZER_STATUS_NOT_CREATE_TMPFILE,
	CCOSRANALYZER_STATUS_DISTORTED_IMAGE,
	CCOSRANALYZER_STATUS_NOT_FOUND_SRML,
	CCOSRANALYZER_STATUS_NOT_READ_SRML,
	CCOSRANALYZER_STATUS_NOT_READ_SRCONF,
	CCOSRANALYZER_STATUS_CANNOT_SAVE_IMAGE,
	CCOSRANALYZER_STATUS_UNSUPPORTED_OCR_ENGINE,
	CCOSRANALYZER_STATUS_NOT_FOUND_ID,
	CCOSRANALYZER_STATUS_ERROR
};

typedef enum CCOSRANALYZER_STATUS CCOSRANALYZER_STATUS;

#define CCOSRANALYZER_PATTERN_STYLE 2
#define CCOSRANALYZER_PATTERN_DISTORTED_RATE 0.8
#define CCOSRANALYZER_IMAGE_DISTORTED_RATE 0.98

cco_srAnalyzer *cco_srAnalyzer_baseNew(int size);
void cco_srAnalyzer_baseRelease(void *cco);
void cco_srAnalyzer_baseInitialize(cco_srAnalyzer *cco);
void cco_srAnalyzer_baseFinalize(cco_srAnalyzer *cco);
cco_srAnalyzer *cco_srAnalyzer_new();
void cco_srAnalyzer_release(void *cco);

int cco_srAnalyzer_countSheets(cco_srAnalyzer *obj);
int cco_srAnalyzer_readSrconf(cco_srAnalyzer *obj, char *file);

CCOSRANALYZER_STATUS cco_srAnalyzer_readImage(cco_srAnalyzer *obj, char *file);
CCOSRANALYZER_STATUS cco_srAnalyzer_adjustImage(cco_srAnalyzer *cco_srAnalyzer);
CCOSRANALYZER_STATUS cco_srAnalyzer_ocr(cco_srAnalyzer *obj);
CCOSRANALYZER_STATUS cco_srAnalyzer_outXml(cco_srAnalyzer *obj);
CCOSRANALYZER_STATUS cco_srAnalyzer_outSql(cco_srAnalyzer *obj);
CCOSRANALYZER_STATUS cco_srAnalyzer_backupImage(cco_srAnalyzer *obj);
void cco_srAnalyzer_resultPrint(cco_srAnalyzer *obj, char *mode, char *sr_result);
CCOSRANALYZER_STATUS cco_srAnalyzer_setMarkerSampleImageFile(char *filename);

int cco_srAnalyzer_showImage_createWindow(cco_srAnalyzer *obj, char* windowname, IplImage *img);
int cco_srAnalyzer_showImage_destroyWindow(cco_srAnalyzer *obj, char* windowname);
int cco_srAnalyzer_showImageNow_withImage(cco_srAnalyzer *obj, char* windowname, IplImage *img);
int cco_srAnalyzer_showShrinkedImageNow_withImage(cco_srAnalyzer *obj, char* windowname, IplImage *img, int shrink);
int cco_srAnalyzer_showImageNow(cco_srAnalyzer *cco_srAnalyzer, char* windowname);
int cco_srAnalyzer_showShrinkedImageNow(cco_srAnalyzer *obj, char* windowname, int shrink);

#define cvSaveImage(x, y) cvSaveImage(x, y, 0)

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SRANALYZER_PROPERTIES
*/

#endif /* CCO_SRANALYZER_H_ */
