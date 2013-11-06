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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <opencv/highgui.h>
#include <opencv/cv.h>

#ifdef KOCR
#include "cco_srOcrKocr.h"

#ifdef USE_SVM
extern void *kocr_svm_init(char *);			/* fake declaration: CvSVM* -> void* */
extern char *kocr_recognize_image(void *, char *);	/* fake declaration: CvSVM* -> void* */

void *db_num;
void *db_mbs;
void *db_ocrb;

#define DB_FILE_NUM "list-num.xml"
#define DB_FILE_MBS "list-mbs.xml"
#define DB_FILE_OCRB "list-ocrb.xml"

#else	/* USE_SVM */

#include "kocr.h"
#include "subr.h"

feature_db *db_num;
feature_db *db_mbs;
feature_db *db_ocrb;

#define DB_FILE_NUM "list-num.db"
#define DB_FILE_MBS "list-mbs.db"
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
#ifdef KOCR
		cco_srOcrKocr_initialize(o, NULL);
#endif
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
	o->srOcrKocr_filename = NULL;
	o->srOcrKocr_option = strdup("0-9");
	o->srOcrKocr_db = strdup("handwrite_09db");
	o->srOcrKocr_dustsize = strdup("50");
	o->srOcrKocr_aoption = strdup("1");
	return;
}

void cco_srOcrKocr_baseFinalize(cco_srOcrKocr *o)
{
	if (o->srOcrKocr_filename != NULL)
	{
		free(o->srOcrKocr_filename);
		o->srOcrKocr_filename = NULL;
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

#ifdef KOCR
	/* load databases */
	if (db_num == NULL)
#ifdef USE_SVM
		db_num = kocr_svm_init(dircat(ocrdb_dir, DB_FILE_NUM));
#else
		db_num = kocr_init(dircat(ocrdb_dir, DB_FILE_NUM));
#endif

	if (db_mbs == NULL)
#ifdef USE_SVM
		db_mbs = kocr_svm_init(dircat(ocrdb_dir, DB_FILE_MBS));
#else
		db_mbs = kocr_init(dircat(ocrdb_dir, DB_FILE_MBS));
#endif

	if (db_ocrb == NULL)
#ifdef USE_SVM
		db_ocrb = kocr_svm_init(dircat(ocrdb_dir, DB_FILE_OCRB));
#else
		db_ocrb = kocr_init(dircat(ocrdb_dir, DB_FILE_OCRB));
#endif

	if (db_num == NULL || db_mbs == NULL || db_ocrb == NULL)
		return CCOSROCR_STATUS_DBDONOTEXIST;
#endif

	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrKocr_setImage(void *obj, char *imagefile)
{
	cco_srOcrKocr *ocr;
	ocr = obj;
	if (ocr->srOcrKocr_filename != NULL)
	{
		free(ocr->srOcrKocr_filename);
	}
	ocr->srOcrKocr_filename = strdup(imagefile);
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrKocr_setOption(void *obj, char *option)
{
	cco_srOcrKocr *ocr;
	ocr = obj;

#ifdef KOCR
	if (strcmp(option, "number") == 0)
	{
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = (char *) db_num;
	}
	else if (strcmp(option, "rating") == 0)
	{
		ocr->srOcrKocr_option = strdup("mbs");
		ocr->srOcrKocr_db = (char *) db_mbs;
	}
	else if (strcmp(option, "ids") == 0)
	{
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = (char *) db_ocrb;
	}
	else if (strcmp(option, "idocrbs") == 0)
	{
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = (char *) db_ocrb;
	} else {
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = (char *) db_num;
	}
#else
	if (ocr->srOcrKocr_option != NULL)
	{
		free(ocr->srOcrKocr_option);
	}
	if (ocr->srOcrKocr_db != NULL)
	{
		free(ocr->srOcrKocr_db);
	}
	if (ocr->srOcrKocr_dustsize != NULL)
	{
		free(ocr->srOcrKocr_dustsize);
	}
	if (ocr->srOcrKocr_aoption != NULL)
	{
		free(ocr->srOcrKocr_aoption);
	}

	if (strcmp(option, "number") == 0)
	{
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = strdup("handwrite_09db");
		ocr->srOcrKocr_dustsize = strdup("80");
		ocr->srOcrKocr_aoption = strdup("1");
	}
	else if (strcmp(option, "rating") == 0)
	{
		ocr->srOcrKocr_option = strdup("mbs");
		ocr->srOcrKocr_db = strdup("handwrite_mbsdb");
		ocr->srOcrKocr_dustsize = strdup("80");
		ocr->srOcrKocr_aoption = strdup("1");
	}
	else if (strcmp(option, "ids") == 0)
	{
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = strdup("iddb");
		ocr->srOcrKocr_dustsize = strdup("10");
		ocr->srOcrKocr_aoption = strdup("1");
	}
	else if (strcmp(option, "idocrbs") == 0)
	{
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = strdup("idocrbdb");
		ocr->srOcrKocr_dustsize = strdup("50");
		ocr->srOcrKocr_aoption = strdup("1");
	} else {
		ocr->srOcrKocr_option = strdup("0-9");
		ocr->srOcrKocr_db = strdup("handwrite_09db");
		ocr->srOcrKocr_dustsize = strdup("80");
		ocr->srOcrKocr_aoption = strdup("1");
	}
#endif /* KOCR */

	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrKocr_getRecognizeString(void *obj, cco_vString **recognizedString)
{
	CCOSROCR_STATUS result = CCOSROCR_STATUS_SUCCESS;
	cco_srOcrKocr *ocrobj;
	cco_vString *tmpppm_string = NULL;
	cco_vString *tmpbmp_string = NULL;
	cco_vString *tmptxt_string = NULL;
	cco_vString *cmdppmtobmp_string = NULL;
	cco_vString *cmdkocr_string = NULL;
	cco_vString *tmp1_string = NULL;
	char *tmp1_cstring;
	IplImage *ppm_img = NULL;
	FILE *fp = NULL;

	ocrobj = obj;

	do
	{
		if (recognizedString == NULL)
		{
			result = CCOSROCR_STATUS_DONOTRECOGNIZE;
			break;
		}
		if (ocrobj->srOcrKocr_filename == NULL)
		{
			result = CCOSROCR_STATUS_UNSUPPORTIMAGE;
			break;
		}

		/*
		 * ppm to bmp conversion (what's the original format here?)
		 */
		tmpppm_string = cco_vString_newWithFormat("%s.ppm", ocrobj->srOcrKocr_filename);
		tmpbmp_string = cco_vString_newWithFormat("%s.bmp", ocrobj->srOcrKocr_filename);
		tmptxt_string = cco_vString_newWithFormat("%s.txt", ocrobj->srOcrKocr_filename);

		tmp1_cstring = tmpppm_string->v_getCstring(tmpppm_string);
		ppm_img = cvLoadImage(ocrobj->srOcrKocr_filename, CV_LOAD_IMAGE_COLOR);
		cvResetImageROI(ppm_img);
		cvSaveImage(tmp1_cstring, ppm_img);
		free(tmp1_cstring);
		tmp1_cstring = NULL;

		cmdppmtobmp_string = cco_vString_newWithFormat("ppmtobmp --quiet %@ > %@", tmpppm_string, tmpbmp_string);
		tmp1_cstring = cmdppmtobmp_string->v_getCstring(cmdppmtobmp_string);
		system(tmp1_cstring);
		free(tmp1_cstring);
		tmp1_cstring = NULL;

		/*
		 * OCR processing
		 */
#ifdef KOCR
		{
			char *fn = tmpbmp_string->v_getCstring(tmpbmp_string);
#ifdef USE_SVM
			char *rc = kocr_recognize_image(ocrobj->srOcrKocr_db, (char *) fn);
#else
			char *rc = kocr_recognize_image((feature_db *) ocrobj->srOcrKocr_db, (char *) fn);
#endif
			cco_release(*recognizedString); /* needed? */
			if (rc) {
				tmp1_string = cco_vString_new(rc);
				cco_vString_replaceWithCstring(tmp1_string, "m", "○");
				cco_vString_replaceWithCstring(tmp1_string, "b", "×");
				cco_vString_replaceWithCstring(tmp1_string, "s", "△");
				*recognizedString =
				  cco_vString_getReplacedStringWithCstring(tmp1_string,
									   " ", "");
				cco_release(tmp1_string);
			} else {
				*recognizedString = cco_vString_new(rc);
			}
		}
#else
		int read_length;
		char recognize_buff[124];
		recognize_buff[0] = 0;
		cmdkocr_string = cco_vString_newWithFormat(
				"kocr -d %s -C %s -m 258 -a %s "
				"-p %s/etc/sheetreader/kocrdbs/%s/ "
				"-o %@ %@ 1> /dev/null 2> /dev/null",
				ocrobj->srOcrKocr_dustsize,
				ocrobj->srOcrKocr_option,
				ocrobj->srOcrKocr_aoption,
				PREFIX,
				ocrobj->srOcrKocr_db,
				tmptxt_string,
				tmpbmp_string);

		tmp1_cstring = cmdkocr_string->v_getCstring(cmdkocr_string);
		system(tmp1_cstring);
		free(tmp1_cstring);
		tmp1_cstring = NULL;

		tmp1_cstring = tmptxt_string->v_getCstring(tmptxt_string);
		read_length = 0;
		do {
			fp = fopen(tmp1_cstring, "r");
			if (fp == NULL)
			{
				result = CCOSROCR_STATUS_DONOTRECOGNIZE;
				break;
			}
			read_length = fread(recognize_buff, 1, sizeof(recognize_buff) - 1, fp);
			if (read_length < 0)
			{
				result = CCOSROCR_STATUS_DONOTRECOGNIZE;
				break;
			}
		} while (0);
		recognize_buff[read_length] = 0;
		free(tmp1_cstring);
		tmp1_cstring = NULL;
		cco_release(*recognizedString);
		cco_srOcrKocr_getRecognizeString_currn(ocrobj, recognize_buff);
		tmp1_string = cco_vString_new(recognize_buff);
		cco_vString_replaceWithCstring(tmp1_string, "m", "○");
		cco_vString_replaceWithCstring(tmp1_string, "b", "×");
		cco_vString_replaceWithCstring(tmp1_string, "s", "△");
		*recognizedString = cco_vString_getReplacedStringWithCstring(tmp1_string, " ", "");
		cco_release(tmp1_string);
#endif
	} while (0);
	if (tmp1_cstring != NULL)
	{
		free(tmp1_cstring);
	}
	if (fp != NULL)
	{
		fclose(fp);
	}
	/* Deletes files.. */
	tmp1_cstring = tmpppm_string->v_getCstring(tmpppm_string);
	remove(tmp1_cstring);
	free(tmp1_cstring);
	tmp1_cstring = tmpbmp_string->v_getCstring(tmpbmp_string);
	remove(tmp1_cstring);
	free(tmp1_cstring);
	tmp1_cstring = tmptxt_string->v_getCstring(tmptxt_string);
	remove(tmp1_cstring);
	free(tmp1_cstring);

	cco_release(tmpppm_string);
	cco_release(tmpbmp_string);
	cco_release(tmptxt_string);
	cco_release(cmdppmtobmp_string);
	cco_release(cmdkocr_string);
	cvReleaseImage(&ppm_img);
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

