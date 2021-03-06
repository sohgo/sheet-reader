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

#include "cco_srOcrGocr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define HAVE_OPENCV_IMGCODECS
#include <opencv/highgui.h>
#include <opencv/cv.h>

#include "utility.h"

#define cvSaveImage(x, y) cvSaveImage(x, y, 0)

cco_defineClass(cco_srOcrGocr);

cco_srOcrGocr *cco_srOcrGocr_baseNew(int size)
{
	cco_srOcrGocr *o = NULL;
	do {
		if (size < sizeof(cco_srOcrGocr))
		{
			break;
		}
		o = (cco_srOcrGocr *)cco_srOcr_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srOcrGocr);
		cco_srOcrGocr_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srOcrGocr_baseRelease(void *o)
{
	cco_srOcrGocr_baseFinalize(o);
	cco_srOcr_baseRelease(o);
}

void cco_srOcrGocr_baseInitialize(cco_srOcrGocr *o)
{
	o->baseRelease = &cco_srOcrGocr_baseRelease;
	o->srOcr_initialize = &cco_srOcrGocr_initialize;
	o->srOcr_setImage = &cco_srOcrGocr_setImage;
	o->srOcr_setOption = &cco_srOcrGocr_setOption;
	o->srOcr_getRecognizeString = &cco_srOcrGocr_getRecognizeString;
	o->srOcrGocr_image = NULL;
	o->srOcrGocr_option = strdup("0-9");
	o->srOcrGocr_db = strdup("handwrite_09db");
	o->srOcrGocr_dustsize = strdup("50");
	o->srOcrGocr_aoption = strdup("1");
	return;
}

void cco_srOcrGocr_baseFinalize(cco_srOcrGocr *o)
{
	if (o->srOcrGocr_image != NULL)
	{
		cvReleaseImage(&o->srOcrGocr_image);
		o->srOcrGocr_image = NULL;
		free(o->srOcrGocr_option);
		o->srOcrGocr_option = NULL;
		free(o->srOcrGocr_db);
		o->srOcrGocr_db = NULL;
		free(o->srOcrGocr_dustsize);
		o->srOcrGocr_dustsize = NULL;
		free(o->srOcrGocr_aoption);
		o->srOcrGocr_aoption = NULL;
	}
	return;
}

cco_srOcrGocr *cco_srOcrGocr_new()
{
	return cco_srOcrGocr_baseNew(sizeof(cco_srOcrGocr));
}

void cco_srOcrGocr_release(void *o)
{
	cco_release(o);
}

void cco_srOcrGocr_getRecognizeString_currn(void *obj, char*cstring);

CCOSROCR_STATUS cco_srOcrGocr_initialize(void *obj, char *configfile)
{
	cco_srOcrGocr *ocr;
	ocr = obj;
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrGocr_setImage(void *obj, IplImage *image)
{
	cco_srOcrGocr *ocr;
	ocr = obj;
	if (ocr->srOcrGocr_image != NULL)
	{
		cvReleaseImage(&ocr->srOcrGocr_image);
	}
	ocr->srOcrGocr_image = cvCloneImage(image);
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrGocr_setOption(void *obj, char *option)
{
	cco_srOcrGocr *ocr;
	ocr = obj;

	if (ocr->srOcrGocr_option != NULL)
	{
		free(ocr->srOcrGocr_option);
	}
	if (ocr->srOcrGocr_db != NULL)
	{
		free(ocr->srOcrGocr_db);
	}
	if (ocr->srOcrGocr_dustsize != NULL)
	{
		free(ocr->srOcrGocr_dustsize);
	}
	if (ocr->srOcrGocr_aoption != NULL)
	{
		free(ocr->srOcrGocr_aoption);
	}

	if (strcmp(option, "number") == 0)
	{
		ocr->srOcrGocr_option = strdup("0-9");
		ocr->srOcrGocr_db = strdup("handwrite_09db");
		ocr->srOcrGocr_dustsize = strdup("80");
		ocr->srOcrGocr_aoption = strdup("1");
	}
	else if (strcmp(option, "rating") == 0)
	{
		ocr->srOcrGocr_option = strdup("mbs");
		ocr->srOcrGocr_db = strdup("handwrite_mbsdb");
		ocr->srOcrGocr_dustsize = strdup("80");
		ocr->srOcrGocr_aoption = strdup("1");
	}
	else if (strcmp(option, "ids") == 0)
	{
		ocr->srOcrGocr_option = strdup("0-9");
		ocr->srOcrGocr_db = strdup("iddb");
		ocr->srOcrGocr_dustsize = strdup("10");
		ocr->srOcrGocr_aoption = strdup("1");
	}
	else if (strcmp(option, "idocrbs") == 0)
	{
		ocr->srOcrGocr_option = strdup("0-9");
		ocr->srOcrGocr_db = strdup("idocrbdb");
		ocr->srOcrGocr_dustsize = strdup("50");
		ocr->srOcrGocr_aoption = strdup("1");
	} else {
		ocr->srOcrGocr_option = strdup("0-9");
		ocr->srOcrGocr_db = strdup("handwrite_09db");
		ocr->srOcrGocr_dustsize = strdup("80");
		ocr->srOcrGocr_aoption = strdup("1");
	}
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrGocr_getRecognizeString(void *obj, cco_vString **recognizedString)
{
	CCOSROCR_STATUS result = CCOSROCR_STATUS_SUCCESS;
	cco_srOcrGocr *ocrobj;
	cco_vString *tmppgm_string = NULL;
	cco_vString *tmptxt_string = NULL;
	cco_vString *cmdnhocr_string = NULL;
	cco_vString *tmp1_string = NULL;
	char *tmp1_cstring;
	FILE *fp = NULL;
	int read_length;
	char recognize_buff[124];
	char *tmpfile_prefix = NULL;

	ocrobj = obj;
	recognize_buff[0] = 0;

	do
	{
		if (recognizedString == NULL)
		{
			result = CCOSROCR_STATUS_DONOTRECOGNIZE;
			break;
		}
		if (ocrobj->srOcrGocr_image == NULL)
		{
			result = CCOSROCR_STATUS_UNSUPPORTIMAGE;
			break;
		}

		tmpfile_prefix = utility_get_tmp_file("srgocr");
		if (tmpfile_prefix == NULL)
		{
			fprintf(stderr, "ERROR: Cannot create a temporary file in %s.\n", __func__);
			result = CCOSROCR_STATUS_DONOTRECOGNIZE;
			break;
		}
		tmppgm_string = cco_vString_newWithFormat("%s.pgm", tmpfile_prefix);
		tmptxt_string = cco_vString_newWithFormat("%s.txt", tmpfile_prefix);

		tmp1_cstring = tmppgm_string->v_getCstring(tmppgm_string);
		cvSaveImage(tmp1_cstring, ocrobj->srOcrGocr_image);
		free(tmp1_cstring);
		tmp1_cstring = NULL;

		/* do GOCR. */
		cmdnhocr_string = cco_vString_newWithFormat(
				"gocr -d %s -C %s -m 258 -a %s "
				"-p %s/etc/sheetreader/gocrdbs/%s/ "
				"-o %@ %@ 1> /dev/null 2> /dev/null",
				ocrobj->srOcrGocr_dustsize,
				ocrobj->srOcrGocr_option,
				ocrobj->srOcrGocr_aoption,
				PREFIX,
				ocrobj->srOcrGocr_db,
				tmptxt_string,
				tmppgm_string);

		tmp1_cstring = cmdnhocr_string->v_getCstring(cmdnhocr_string);
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
		cco_srOcrGocr_getRecognizeString_currn(ocrobj, recognize_buff);
		tmp1_string = cco_vString_new(recognize_buff);
		cco_vString_replaceWithCstring(tmp1_string, "m", "○");
		cco_vString_replaceWithCstring(tmp1_string, "b", "×");
		cco_vString_replaceWithCstring(tmp1_string, "s", "△");
		*recognizedString = cco_vString_getReplacedStringWithCstring(tmp1_string, " ", "");
		/* *recognizedString = cco_vString_getReplacedStringWithCstring(tmp2_string, "[ijflI]", "1"); */
		cco_release(tmp1_string);
		/* cco_release(tmp2_string); */
	} while (0);
	if (tmp1_cstring != NULL)
	{
		free(tmp1_cstring);
	}
	if (fp != NULL)
	{
		fclose(fp);
	}
	if (tmpfile_prefix != NULL)
	{
		free(tmpfile_prefix);
	}
	/* Deletes files.. */
	tmp1_cstring = tmppgm_string->v_getCstring(tmppgm_string);
	remove(tmp1_cstring);
	free(tmp1_cstring);
	tmp1_cstring = tmptxt_string->v_getCstring(tmptxt_string);
	remove(tmp1_cstring);
	free(tmp1_cstring);

	cco_release(tmppgm_string);
	cco_release(tmptxt_string);
	cco_release(cmdnhocr_string);
	return result;
}

void cco_srOcrGocr_getRecognizeString_currn(void *obj, char*cstring)
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
