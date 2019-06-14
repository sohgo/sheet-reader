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

#define HAVE_OPENCV_IMGCODECS
#include <opencv/highgui.h>

#include <cluscore/cco_arraylist.h>
#include <cluscore/cco_vString.h>
#include "cco_srOcrOpencv.h"
#include "cco_vSrPattern.h"

cco_defineClass(cco_srOcrOpencv);

cco_srOcrOpencv *cco_srOcrOpencv_baseNew(int size)
{
	cco_srOcrOpencv *o = NULL;
	do {
		if (size < sizeof(cco_srOcrOpencv))
		{
			break;
		}
		o = (cco_srOcrOpencv *)cco_srOcr_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srOcrOpencv);
		cco_srOcrOpencv_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srOcrOpencv_baseRelease(void *o)
{
	cco_srOcrOpencv_baseFinalize(o);
	cco_srOcr_baseRelease(o);
}

void cco_srOcrOpencv_baseInitialize(cco_srOcrOpencv *o)
{
	o->baseRelease = &cco_srOcrOpencv_baseRelease;
	o->srOcrOpencv_img = NULL;
	o->srOcrOpencv_debug = 0;
	o->srOcrOpencv_conturs = NULL;
	o->srOcrOpencv_conturs_approx = NULL;
	o->srOcrOpencv_strage = cvCreateMemStorage(0);
	o->srOcrOpencv_strage_approx = cvCreateMemStorage(0);

	return;
}

void cco_srOcrOpencv_baseFinalize(cco_srOcrOpencv *o)
{
	if (o->srOcrOpencv_img != NULL)
	{
		cvReleaseImage(&o->srOcrOpencv_img);
	}
	cvReleaseMemStorage(&o->srOcrOpencv_strage);
	cvReleaseMemStorage(&o->srOcrOpencv_strage_approx);
	return;
}

cco_srOcrOpencv *cco_srOcrOpencv_new()
{
	return cco_srOcrOpencv_baseNew(sizeof(cco_srOcrOpencv));
}

void cco_srOcrOpencv_release(void *o)
{
	cco_release(o);
}

CCOSROCR_STATUS cco_srOcrOpencv_initialize(void *obj, char *configfile)
{
	cco_srOcrOpencv *ocr;
	ocr = obj;
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrOpencv_setImage(void *obj, char *imagefile)
{
	cco_srOcrOpencv *ocr;
	ocr = obj;
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrOpencv_getRecognizeString(void *obj, cco_vString **recognizedString)
{
	cco_srOcrOpencv *ocr;
	ocr = obj;
	return CCOSROCR_STATUS_SUCCESS;
}

CCOSROCR_STATUS cco_srOcrOpencv_findLetter(cco_srOcrOpencv *obj, IplImage *image, cco_arraylist *added_patternlist)
{
	CCOSROCR_STATUS result = CCOSROCR_STATUS_SUCCESS;
	IplImage *img_tmp = NULL;
	IplImage *img_prebinarized = NULL;
	IplImage *img_binarized = NULL;
    CvSeq *seq_cursor;
    CvRect rect_parent;
    cco_arraylist *list_candidate_pattern = added_patternlist;
    cco_vSrPattern *candidate_pattern = NULL;

    do {
    	/* Check loaded image. */
    	if (image == NULL)
    	{
    		result = CCOSROCR_STATUS_UNSUPPORTIMAGE;
    		break;
    	}
		/* Makes binarized image and smooth it. */
		img_tmp = cvCloneImage(image);
		img_prebinarized = cvCloneImage(image);
		img_binarized = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
		cvSmooth(image, img_tmp, CV_GAUSSIAN, 7, 0, 0, 0);
		cvThreshold (img_tmp, img_prebinarized, 250, 255, CV_THRESH_BINARY);
		cvCvtColor (img_prebinarized, img_binarized, CV_BGR2GRAY);

		/* Shows images. */
		if (obj->srOcrOpencv_debug)
		{
			cco_srOcrOpencv_showImageNow_withImage(obj, "findPatterns: smooth", img_binarized);
		}

		/* Examines an image to find contours. */
		obj->srOcrOpencv_conturs = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint), obj->srOcrOpencv_strage);
		cvFindContours(img_binarized, obj->srOcrOpencv_strage, &obj->srOcrOpencv_conturs, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE,
				cvPoint(0, 0));
		cvReleaseImage(&img_tmp);

		/* Examines an image to find square patterns. */
		img_tmp = cvCloneImage(image);
		for (seq_cursor = obj->srOcrOpencv_conturs; seq_cursor != NULL; seq_cursor = seq_cursor->h_next)
		{
			/* Makes point to decide letter. */
			rect_parent = cvBoundingRect(seq_cursor, 0);
			obj->srOcrOpencv_conturs_approx = cvApproxPoly(seq_cursor, sizeof(CvContour), obj->srOcrOpencv_strage_approx, CV_POLY_APPROX_DP, 5, 0);

			/* Draws pattern to image. */
			cvRectangle(img_tmp, cvPoint(rect_parent.x, rect_parent.y), cvPoint(rect_parent.x + rect_parent.width,
					rect_parent.y + rect_parent.height), CV_RGB(255, 0, 0), 1, 8, 0);
			cvDrawContours(img_tmp, obj->srOcrOpencv_conturs_approx, CV_RGB (0, 0, 255), CV_RGB (0, 0, 255), 100, 1, 8, cvPoint(0,0));

			/* Adds pattern to list of candidate. */
			candidate_pattern = cco_vSrPattern_new();
			cco_vSrPattern_setInInt(candidate_pattern, rect_parent.x, rect_parent.y, rect_parent.width, rect_parent.height);
			cco_arraylist_addAtBack(list_candidate_pattern, candidate_pattern);
			cco_release(candidate_pattern);
		}
		/* Shows images. */
		if (obj->srOcrOpencv_debug)
		{
			cco_srOcrOpencv_showImageNow_withImage(obj, "findPatterns: found patterns", img_tmp);
		}
    } while (0);
	cvReleaseImage(&img_tmp);
	cvReleaseImage(&img_prebinarized);
	cvReleaseImage(&img_binarized);
	return result;
}

int cco_srOcrOpencv_showImageNow_withImage(cco_srOcrOpencv *obj, char* windowname, IplImage *img)
{
	cvNamedWindow(windowname, 1);
    cvShowImage(windowname, img);
    cvResizeWindow(windowname, 300, 300);
    cvWaitKey(0);
    cvDestroyWindow(windowname);
    return 0;
}

CCOSROCR_STATUS cco_srOcrOpencv_readImage(cco_srOcrOpencv *obj, char *file)
{
	CCOSROCR_STATUS result = CCOSROCR_STATUS_SUCCESS;
	if (obj->srOcrOpencv_img != NULL) {
		cvReleaseImage(&obj->srOcrOpencv_img);
	}
	obj->srOcrOpencv_img = cvLoadImage(file, 1);
	cvResetImageROI(obj->srOcrOpencv_img);
	return result;
}

CCOSROCR_STATUS cco_srOcrOpencv_recognizeImage(cco_srOcrOpencv *obj)
{
	CCOSROCR_STATUS result = CCOSROCR_STATUS_SUCCESS;
	cco_arraylist *candidate_list;

	candidate_list = cco_arraylist_new();
	cco_srOcrOpencv_findLetter(obj, obj->srOcrOpencv_img, candidate_list);
	cco_release(candidate_list);

	return result;
}
