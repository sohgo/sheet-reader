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

#include "cco_srOcrNhocr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <opencv/highgui.h>
#include <opencv/cv.h>

#include "utility.h"

#define cvSaveImage(x, y) cvSaveImage(x, y, 0)

cco_defineClass(cco_srOcrNhocr);

cco_srOcrNhocr *cco_srOcrNhocr_baseNew(int size)
{
	cco_srOcrNhocr *o = NULL;
	do {
		if (size < sizeof(cco_srOcrNhocr))
		{
			break;
		}
		o = (cco_srOcrNhocr *)cco_srOcr_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srOcrNhocr);
		cco_srOcrNhocr_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srOcrNhocr_baseRelease(void *o)
{
	cco_srOcrNhocr_baseFinalize(o);
	cco_srOcr_baseRelease(o);
}

void cco_srOcrNhocr_baseInitialize(cco_srOcrNhocr *o)
{
	o->baseRelease = &cco_srOcrNhocr_baseRelease;
	o->srOcr_initialize = &cco_srOcrNhocr_initialize;
	o->srOcr_setImage = &cco_srOcrNhocr_setImage;
	o->srOcr_setOption = &cco_srOcrNhocr_setOption;
	o->srOcr_getRecognizeString = &cco_srOcrNhocr_getRecognizeString;
	o->srOcrNhocr_image = NULL;
	o->srOcrNhocr_option = strdup("ascii+:jpn");
	return;
}

void cco_srOcrNhocr_baseFinalize(cco_srOcrNhocr *o)
{
	if (o->srOcrNhocr_image != NULL)
	{
		cvReleaseImage(&o->srOcrNhocr_image);
		o->srOcrNhocr_image = NULL;
		free(o->srOcrNhocr_option);
		o->srOcrNhocr_option = NULL;
	}
	return;
}

cco_srOcrNhocr *cco_srOcrNhocr_new()
{
	return cco_srOcrNhocr_baseNew(sizeof(cco_srOcrNhocr));
}

void cco_srOcrNhocr_release(void *o)
{
	cco_release(o);
}

void cco_srOcrNhocr_getRecognizeString_currn(void *obj, char*cstring);

CCOSROCR_STATUS cco_srOcrNhocr_initialize(void *obj, char *configfile)
{
	cco_srOcrNhocr *ocr;
	ocr = obj;
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrNhocr_setImage(void *obj, IplImage *image)
{
	cco_srOcrNhocr *ocr;
	ocr = obj;
	if (ocr->srOcrNhocr_image != NULL)
	{
		cvReleaseImage(&ocr->srOcrNhocr_image);
	}
	ocr->srOcrNhocr_image = cvCloneImage(image);
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrNhocr_setOption(void *obj, char *option)
{
	cco_srOcrNhocr *ocr;
	ocr = obj;
	if (ocr->srOcrNhocr_option != NULL)
	{
		free(ocr->srOcrNhocr_option);
	}
	ocr->srOcrNhocr_option = strdup(option);
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrNhocr_getRecognizeString(void *obj, cco_vString **recognizedString)
{
	CCOSROCR_STATUS result = CCOSROCR_STATUS_SUCCESS;
	cco_srOcrNhocr *ocrobj;
	cco_vString *tmppgm_string = NULL;
	cco_vString *tmptxt_string = NULL;
	cco_vString *cmdnhocr_string = NULL;
	cco_vString *tmp1_string = NULL;
	char *getenv_cstring = NULL;
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
		if (ocrobj->srOcrNhocr_image == NULL)
		{
			result = CCOSROCR_STATUS_UNSUPPORTIMAGE;
			break;
		}
		tmpfile_prefix = utility_get_tmp_file("srnocr");
		if (tmpfile_prefix == NULL)
		{
			fprintf(stderr, "ERROR: Cannot create a temporary file in %s.\n", __func__);
			result = CCOSROCR_STATUS_DONOTRECOGNIZE;
			break;
		}
		tmppgm_string = cco_vString_newWithFormat("%s.pgm", tmpfile_prefix);
		tmptxt_string = cco_vString_newWithFormat("%s.txt", tmpfile_prefix);

		tmp1_cstring = tmppgm_string->v_getCstring(tmppgm_string);
		cvSaveImage(tmp1_cstring, ocrobj->srOcrNhocr_image);
		free(tmp1_cstring);
		tmp1_cstring = NULL;

		/* do NHOCR. */
		getenv_cstring = getenv("NHOCR_DICCODES");
		if (getenv_cstring != NULL)
		{
			getenv_cstring = strdup(getenv_cstring);
		}
		setenv("NHOCR_DICCODES", ocrobj->srOcrNhocr_option, 1);
		cmdnhocr_string = cco_vString_newWithFormat("nhocr -line -o %@ %@", tmptxt_string, tmppgm_string);
		tmp1_cstring = cmdnhocr_string->v_getCstring(cmdnhocr_string);
		system(tmp1_cstring);
		free(tmp1_cstring);
		tmp1_cstring = NULL;
		if (getenv_cstring != NULL)
		{
			setenv("NHOCR_DICCODES", getenv_cstring, 1);
		}

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
		cco_srOcrNhocr_getRecognizeString_currn(ocrobj, recognize_buff);
		tmp1_string = cco_vString_new(recognize_buff);
		*recognizedString = cco_vString_getReplacedStringWithCstring(tmp1_string, " ", "");
		/* *recognizedString = cco_vString_getReplacedStringWithCstring(tmp2_string, "[ijflI]", "1"); */
		cco_release(tmp1_string);
		/* cco_release(tmp2_string); */
	} while (0);
	if (getenv_cstring != NULL)
	{
		free(getenv_cstring);
	}
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

void cco_srOcrNhocr_getRecognizeString_currn(void *obj, char*cstring)
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
