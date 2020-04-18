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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <opencv/highgui.h>
#include <opencv/cv.h>

#ifdef KOCR
#include "cco_srOcrKocr.h"

#ifdef USE_CNN
extern void *kocr_cnn_init(char *);			/* fake declaration: Network* -> void* */
extern char *kocr_recognize_image(void *, char *);	/* fake declaration: Network* -> void* */
extern char *kocr_recognize_Image(void *, IplImage *);	/* fake declaration: Network* -> void* */

void *db_num;
void *db_mbs;
void *db_lowercase;
void *db_uppercase;
void *db_ocrb;

#define DB_FILE_NUM "cnn-num.txt"
#define DB_FILE_MBS "cnn-mbscpn.txt"
#define DB_FILE_LOWERCASE "cnn-lowercase.txt"
#define DB_FILE_UPPERCASE "cnn-uppercase.txt"
#define DB_FILE_OCRB "cnn-num.txt"

#elif defined(USE_SVM)
extern void *kocr_svm_init(char *);			/* fake declaration: CvSVM* -> void* */
extern char *kocr_recognize_image(void *, char *);	/* fake declaration: CvSVM* -> void* */

void *db_num;
void *db_mbs;
void *db_ocrb;

#define DB_FILE_NUM "list-num.xml"
#define DB_FILE_MBS "list-mbscz.xml"
#define DB_FILE_OCRB "list-ocrb.xml"

#else	/* USE_SVM */

#include "kocr.h"
#include "subr.h"

feature_db *db_num;
feature_db *db_mbs;
feature_db *db_ocrb;

#define DB_FILE_NUM "list-num.db"
#define DB_FILE_MBS "list-mbscz.db"
#define DB_FILE_OCRB "list-ocrb.db"

#endif	/* USE_SVM */

extern char *ocrdb_dir;

/*
char recognize(feature_db *a, char *b)
{
  return 0;
}
*/

#define cvSaveImage(x, y) cvSaveImage(x, y, 0)

cco_defineClass(cco_srOcrKocr);

cco_srOcrKocr *cco_srOcrKocr_baseNew(int size)
{
	cco_srOcrKocr *o = NULL;
	do {
		if (size < sizeof(cco_srOcrKocr))
		{
			break;
		}
		o = (cco_srOcrKocr *)cco_srOcr_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srOcrKocr);
		cco_srOcrKocr_baseInitialize(o);
		cco_srOcrKocr_initialize(o, NULL);
	} while (0);
	return o;
}

void cco_srOcrKocr_baseRelease(void *o)
{
	cco_srOcrKocr_baseFinalize(o);
	cco_srOcr_baseRelease(o);
}

void cco_srOcrKocr_baseInitialize(cco_srOcrKocr *o)
{
	o->baseRelease = &cco_srOcrKocr_baseRelease;
	o->srOcr_initialize = &cco_srOcrKocr_initialize;
	o->srOcr_setImage = &cco_srOcrKocr_setImage;
	o->srOcr_setOption = &cco_srOcrKocr_setOption;
	o->srOcr_getRecognizeString = &cco_srOcrKocr_getRecognizeString;
	o->srOcrKocr_image = NULL;
	o->srOcrKocr_option = strdup("0-9");
	o->srOcrKocr_db = strdup("handwrite_09db");
	o->srOcrKocr_dustsize = strdup("50");
	o->srOcrKocr_aoption = strdup("1");
	return;
}

void cco_srOcrKocr_baseFinalize(cco_srOcrKocr *o)
{
	if (o->srOcrKocr_image != NULL)
	{
		cvReleaseImage(&o->srOcrKocr_image);
		o->srOcrKocr_image = NULL;
		free(o->srOcrKocr_option);
		o->srOcrKocr_option = NULL;
		free(o->srOcrKocr_db);
		o->srOcrKocr_db = NULL;
		free(o->srOcrKocr_dustsize);
		o->srOcrKocr_dustsize = NULL;
		free(o->srOcrKocr_aoption);
		o->srOcrKocr_aoption = NULL;
	}
	return;
}

cco_srOcrKocr *cco_srOcrKocr_new()
{
	return cco_srOcrKocr_baseNew(sizeof(cco_srOcrKocr));
}

void cco_srOcrKocr_release(void *o)
{
	cco_release(o);
}

void cco_srOcrKocr_getRecognizeString_currn(void *obj, char*cstring);

char *dircat(char *dst, char *src)
{
	char *ret;
	int len_src, len_dst;

	if (!dst)
		return src;
	len_dst = strlen(dst);

	if (!src)
		return dst;
	len_src = strlen(src);

	ret = (char *) malloc(len_dst + len_src + 10);
	strcpy(ret, dst);

	if (dst[len_dst - 1] != '/') {
		strcat(ret, "/");
	}

	strcat(ret, src);

	return ret;
}

CCOSROCR_STATUS cco_srOcrKocr_initialize(void *obj, char *configfile)
{
	cco_srOcrKocr *ocr;
	ocr = obj;

	/* load databases */
	if (db_num == NULL)
#ifdef USE_CNN
		db_num = kocr_cnn_init(dircat(ocrdb_dir, DB_FILE_NUM));
#elif defined(USE_SVM)
		db_num = kocr_svm_init(dircat(ocrdb_dir, DB_FILE_NUM));
#else
		db_num = kocr_init(dircat(ocrdb_dir, DB_FILE_NUM));
#endif

	if (db_mbs == NULL)
#ifdef USE_CNN
		db_mbs = kocr_cnn_init(dircat(ocrdb_dir, DB_FILE_MBS));
#elif defined(USE_SVM)
		db_mbs = kocr_svm_init(dircat(ocrdb_dir, DB_FILE_MBS));
#else
		db_mbs = kocr_init(dircat(ocrdb_dir, DB_FILE_MBS));
#endif

#ifdef USE_CNN
	if (db_lowercase == NULL)
		db_mbs = kocr_cnn_init(dircat(ocrdb_dir, DB_FILE_LOWERCASE));
	if (db_uppercase == NULL)
		db_mbs = kocr_cnn_init(dircat(ocrdb_dir, DB_FILE_UPPERCASE));
#endif

	if (db_ocrb == NULL)
#ifdef USE_CNN
		db_ocrb = kocr_cnn_init(dircat(ocrdb_dir, DB_FILE_OCRB));
#elif defined(USE_SVM)
		db_ocrb = kocr_svm_init(dircat(ocrdb_dir, DB_FILE_OCRB));
#else
		db_ocrb = kocr_init(dircat(ocrdb_dir, DB_FILE_OCRB));
#endif

	if (db_num == NULL || db_mbs == NULL || db_ocrb == NULL)
		return CCOSROCR_STATUS_DBDONOTEXIST;

	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrKocr_setImage(void *obj, IplImage *image)
{
	cco_srOcrKocr *ocr;
	ocr = obj;
	IplImage *tmp_img;

	if (ocr->srOcrKocr_image != NULL)
	{
		cvReleaseImage(&ocr->srOcrKocr_image);
	}
	/* set a new image according to ROI */
	tmp_img = cvCreateImage(cvSize(image->roi->width, image->roi->height), image->depth, image->nChannels);
	cvCopy(image, tmp_img, NULL);
	ocr->srOcrKocr_image = tmp_img;
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrKocr_setOption(void *obj, char *option)
{
	cco_srOcrKocr *ocr;
	ocr = obj;

	if (strcmp(option, "number") == 0)
	{
		if (ocr->srOcrKocr_option != NULL)
			free(ocr->srOcrKocr_option);
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = (char *) db_num;
	}
	else if (strcmp(option, "rating") == 0)
	{
		if (ocr->srOcrKocr_option != NULL)
			free(ocr->srOcrKocr_option);
		ocr->srOcrKocr_option = strdup("mbscz");
		ocr->srOcrKocr_db = (char *) db_mbs;
	}
#ifdef USE_CNN
	else if (strcmp(option, "lowercase") == 0)
	{
		if (ocr->srOcrKocr_option != NULL)
			free(ocr->srOcrKocr_option);
		ocr->srOcrKocr_option = strdup("a-z");
		ocr->srOcrKocr_db = (char *) db_lowercase;
	}
	else if (strcmp(option, "uppercase") == 0)
	{
		if (ocr->srOcrKocr_option != NULL)
			free(ocr->srOcrKocr_option);
		ocr->srOcrKocr_option = strdup("A-Z");
		ocr->srOcrKocr_db = (char *) db_uppercase;
	}
#endif
	else if (strcmp(option, "ids") == 0)
	{
		if (ocr->srOcrKocr_option != NULL)
			free(ocr->srOcrKocr_option);
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = (char *) db_ocrb;
	}
	else if (strcmp(option, "idocrbs") == 0)
	{
		if (ocr->srOcrKocr_option != NULL)
			free(ocr->srOcrKocr_option);
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = (char *) db_ocrb;
	} else {
		if (ocr->srOcrKocr_option != NULL)
			free(ocr->srOcrKocr_option);
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = (char *) db_num;
	}

	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrKocr_getRecognizeString(void *obj, cco_vString **recognizedString)
{
	CCOSROCR_STATUS result = CCOSROCR_STATUS_SUCCESS;
	cco_srOcrKocr *ocrobj;
	cco_vString *tmp1_string = NULL;

	ocrobj = obj;

	do
	{
		if (recognizedString == NULL)
		{
			result = CCOSROCR_STATUS_DONOTRECOGNIZE;
			break;
		}
		if (ocrobj->srOcrKocr_image == NULL)
		{
			result = CCOSROCR_STATUS_UNSUPPORTIMAGE;
			break;
		}

		/*
		 * OCR processing
		 */
		{
#if defined(USE_CNN) || defined (USE_SVM)
			char *rc = kocr_recognize_Image(ocrobj->srOcrKocr_db, ocrobj->srOcrKocr_image);
#else
			char *rc = kocr_recognize_Image((feature_db *) ocrobj->srOcrKocr_db, ocrobj->srOcrKocr_image);
#endif
			cco_release(*recognizedString); /* needed? */
			if (strcmp(ocrobj->srOcrKocr_option, "mbscz") == 0 && rc) {
				tmp1_string = cco_vString_new(rc);
				cco_vString_replaceWithCstring(tmp1_string, "m", "○");
				cco_vString_replaceWithCstring(tmp1_string, "b", "×");
				cco_vString_replaceWithCstring(tmp1_string, "s", "△");
				cco_vString_replaceWithCstring(tmp1_string, "c", "✓");
				cco_vString_replaceWithCstring(tmp1_string, "p", "+");
				cco_vString_replaceWithCstring(tmp1_string, "n", "-");
				cco_vString_replaceWithCstring(tmp1_string, "z", " ");
				*recognizedString =
				  cco_vString_getReplacedStringWithCstring(tmp1_string,
									   " ", "");
				cco_release(tmp1_string);
			} else {
				*recognizedString = cco_vString_new(rc);
			}
		}
	} while (0);

	return result;
}

void cco_srOcrKocr_getRecognizeString_currn(void *obj, char*cstring)
{
	int i;
	int len;

	if (cstring != NULL)
	{
		len = strlen(cstring);
		for (i = 0; i < len; i++)
		{
			if (cstring[i] == '\n' || cstring[i] == '\r')
			{
				cstring[i] = 0;
				break;
			}
		}
	}
	return;
}

#endif

