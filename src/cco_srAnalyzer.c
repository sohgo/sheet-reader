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
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <opencv/highgui.h>
#include <mysql/mysql.h>

#include <cluscore/cco_arraylist.h>
#include <cluscore/cco_vString.h>
#include <cluscore/cco_vXml.h>
#include "cco_srAnalyzer.h"

#include "cco_srOcr.h"

// #include "cco_srOcrNhocr.h"

#include "cco_vSrPattern.h"
#include "cco_srMl.h"
#include "cco_srMlSheet.h"
#include "utility.h"

cco_defineClass(cco_srAnalyzer)
;

cco_srAnalyzer *cco_srAnalyzer_baseNew(int size)
{
	cco_srAnalyzer *o = NULL;
	do
	{
		if (size < sizeof(cco_srAnalyzer))
		{
			break;
		}
		o = (cco_srAnalyzer *) cco_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srAnalyzer);
		cco_srAnalyzer_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srAnalyzer_baseRelease(void *o)
{
	cco_srAnalyzer_baseFinalize(o);
	cco_baseRelease(o);
}

void cco_srAnalyzer_baseInitialize(cco_srAnalyzer *o)
{
	char time_str[32];
	time_t time_val;
	struct tm tm_val;

	o->baseRelease = &cco_srAnalyzer_baseRelease;
	o->srAnalyzer_img = NULL;
	o->srAnalyzer_debug = 0;
	o->srAnalyzer_srml = cco_srMl_new();
	o->srAnalyzer_pattern_upperleft = NULL;
	o->srAnalyzer_pattern_bottomleft = NULL;
	o->srAnalyzer_pattern_upperright = NULL;
	o->srAnalyzer_uid = cco_vString_new("UNNUMBER");
	o->srAnalyzer_sid = cco_vString_new("UNNUMBER");
	o->srAnalyzer_sender = cco_vString_new("");
	o->srAnalyzer_receiver = cco_vString_new("");
	o->srAnalyzer_save_prefix = cco_vString_new("");
	o->srAnalyzer_backup_image = cco_vString_new("");
#ifdef KOCR
	o->srAnalyzer_ocr = cco_vString_new("kocr"); // nhocr
#else
	o->srAnalyzer_ocr = cco_vString_new("gocr");
#endif
	time(&time_val);
	localtime_r(&time_val, &tm_val);
    strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S", &tm_val);
	o->srAnalyzer_date_string = strdup(time_str);
	o->srAnalyzer_outSql = NULL;
	o->srAnalyzer_outXml = NULL;
	o->srAnalyzer_out = NULL;
	o->srAnalyzer_outProp = NULL;
	o->srAnalyzer_analyzedData = cco_redblacktree_new();
	return;
}

void cco_srAnalyzer_baseFinalize(cco_srAnalyzer *o)
{
	if (o->srAnalyzer_img != NULL)
	{
		cvReleaseImage(&o->srAnalyzer_img);
	}
	cco_release(o->srAnalyzer_srml);
	cco_release(o->srAnalyzer_pattern_upperleft);
	cco_release(o->srAnalyzer_pattern_bottomleft);
	cco_release(o->srAnalyzer_pattern_upperright);
	cco_release(o->srAnalyzer_uid);
	cco_release(o->srAnalyzer_sid);
	cco_release(o->srAnalyzer_sender);
	cco_release(o->srAnalyzer_receiver);
	cco_release(o->srAnalyzer_save_prefix);
	cco_release(o->srAnalyzer_backup_image);
	cco_release(o->srAnalyzer_ocr);
	cco_release(o->srAnalyzer_outXml);
	cco_release(o->srAnalyzer_outSql);
	cco_release(o->srAnalyzer_out);
	cco_release(o->srAnalyzer_outProp);
	cco_release(o->srAnalyzer_analyzedData);
	free(o->srAnalyzer_date_string);
	return;
}

cco_srAnalyzer *cco_srAnalyzer_new()
{
	return cco_srAnalyzer_baseNew(sizeof(cco_srAnalyzer));
}

void cco_srAnalyzer_release(void *o)
{
	cco_release(o);
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ThresholdImageToOcr(cco_srAnalyzer *obj, IplImage *img,
		IplImage *dstimg, int smooth, int threshold);

CCOSRANALYZER_STATUS cco_srAnalyzer_readImage(cco_srAnalyzer *obj, char *file)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	IplImage *read_img = NULL;
	CvSize img_size;
	CvRect img_rect;

	do {
		if (obj->srAnalyzer_img != NULL)
		{
			cvReleaseImage(&obj->srAnalyzer_img);
		}

		read_img = cvLoadImage(file, 1);
		if (read_img == NULL)
		{
			result = CCOSRANALYZER_STATUS_NOT_LOAD_IMAGE;
			break;
		}
		img_size = cvGetSize(read_img);
		if (img_size.height > img_size.width)
		{
			img_rect.x = (img_size.height / 2) - (img_size.width / 2);
			img_rect.y = 0;
			img_rect.width = img_size.width;
			img_rect.height = img_size.height;
			img_size.width = img_size.height;
		}
		else
		{
			img_rect.x = 0;
			img_rect.y = (img_size.width / 2) - (img_size.height / 2);
			img_rect.width = img_size.width;
			img_rect.height = img_size.height;
			img_size.height = img_size.width;
		}
		obj->srAnalyzer_img = cvCreateImage(img_size, read_img->depth, read_img->nChannels);
		cvSetImageROI(obj->srAnalyzer_img, img_rect);
		cvCopyImage(read_img, obj->srAnalyzer_img);
		cvResetImageROI(obj->srAnalyzer_img);
	} while (0);
	cvReleaseImage(&read_img);
	return 0;
}

int cco_srAnalyzer_writeImage(cco_srAnalyzer *obj, char *file)
{
#ifdef COMPRESSION
	static int params[] = {CV_IMWRITE_PNG_COMPRESSION, 9, -1};
#else
	char conv_str[4096];
	sprintf(conv_str, "convert %s -quality 9 %s", file, file);
#endif


	int result = -1;
	if (obj->srAnalyzer_img != NULL)
	{
#ifdef COMPRESSION
		cvSaveImage(file, obj->srAnalyzer_img, params);
#else
		cvSaveImage(file, obj->srAnalyzer_img);

		/* XXX: a dirty workaround to reduce image size */
		system(conv_str);
#endif
		result = 0;
	}
	return result;
}

int cco_srAnalyzer_writeImageWithPlace(cco_srAnalyzer *obj, char *file, int x, int y, int width,
		int height)
{
	int result = -1;
	CvRect save_rect;
	CvRect set_rect;

	if (obj->srAnalyzer_img != NULL)
	{
		save_rect = cvGetImageROI(obj->srAnalyzer_img);
		set_rect.x = x;
		set_rect.y = y;
		set_rect.width = width;
		set_rect.height = height;
		cvSetImageROI(obj->srAnalyzer_img, set_rect);
		cvSaveImage(file, obj->srAnalyzer_img);
		cvSetImageROI(obj->srAnalyzer_img, save_rect);
		result = 0;
	}
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_writeImageWithPlaceToOcr(cco_srAnalyzer *obj, char *file,
		int x, int y, int width, int height, int smooth, int threshold)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	CvRect save_rect;
	CvRect set_rect;
	IplImage *tmp_img;

	if (obj->srAnalyzer_img != NULL)
	{
		save_rect = cvGetImageROI(obj->srAnalyzer_img);
		set_rect.x = x;
		set_rect.y = y;
		set_rect.width = width;
		set_rect.height = height;
		cvSetImageROI(obj->srAnalyzer_img, set_rect);
		tmp_img = cvClone(obj->srAnalyzer_img);
		cvSetImageROI(tmp_img, set_rect);
		result = cco_srAnalyzer_ThresholdImageToOcr(obj, obj->srAnalyzer_img, tmp_img, smooth, threshold);

		cvSaveImage(file, tmp_img);
		cvSetImageROI(obj->srAnalyzer_img, save_rect);
		cvReleaseImage(&tmp_img);
	}
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_writeImageWithPlaceToLOcr(cco_srAnalyzer *obj, char *file,
		int x, int y, int width, int height, int smooth, int threshold)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	CvRect save_rect;
	CvRect set_rect;
	IplImage *tmp_img;

	if (obj->srAnalyzer_img != NULL)
	{
		save_rect = cvGetImageROI(obj->srAnalyzer_img);
		set_rect.x = x;
		set_rect.y = y;
		set_rect.width = width;
		set_rect.height = height;
		cvSetImageROI(obj->srAnalyzer_img, set_rect);
		tmp_img = cvCreateImage(cvSize(width * 2, height * 2), obj->srAnalyzer_img->depth, obj->srAnalyzer_img->nChannels);
		cvRectangle(tmp_img, cvPoint(0,0), cvPoint(width * 2, height * 2), cvScalar(0xff, 0xff, 0xff, 0), CV_FILLED, 0, 0);
		cvSetImageROI(tmp_img, cvRect(width / 2, height / 2, width, height));
		result = cco_srAnalyzer_ThresholdImageToOcr(obj, obj->srAnalyzer_img, tmp_img, smooth, threshold);
		cvResetImageROI(tmp_img);
		cvSaveImage(file, tmp_img);
		cvSetImageROI(obj->srAnalyzer_img, save_rect);
		cvReleaseImage(&tmp_img);
	}
	return result;
}

int cco_srAnalyzer_writeResizeImageWithPlace(cco_srAnalyzer *obj, char *file, int x, int y,
		int width, int height, int resize_width, int resize_height)
{
	int result = -1;
	CvRect save_rect;
	CvRect set_rect;
	CvSize resize;
	IplImage *resizeimage;
	IplImage *resizeimage2;

	resize_width = 400;
	resize_height = 200;
	if (obj->srAnalyzer_img != NULL)
	{
		save_rect = cvGetImageROI(obj->srAnalyzer_img);
		set_rect.x = x;
		set_rect.y = y;
		set_rect.width = width;
		set_rect.height = height;
		cvSetImageROI(obj->srAnalyzer_img, set_rect);

		resize.width = resize_width;
		resize.height = resize_height;
		resizeimage = cvCreateImage(resize, obj->srAnalyzer_img->depth,
				obj->srAnalyzer_img->nChannels);
		cvRectangle(resizeimage, cvPoint(0, 0), cvPoint(resize.width, resize.height),
				CV_RGB(255,255,255), CV_FILLED, 8, 0);

		set_rect.x = 10;
		set_rect.y = resize_height * 0.1;
		set_rect.width = resize_height * 0.8;
		set_rect.height = resize_height * 0.8;
		cvSetImageROI(resizeimage, set_rect);
		cvResize(obj->srAnalyzer_img, resizeimage, CV_INTER_LINEAR);
		cvResetImageROI(resizeimage);
		resizeimage2 = cvCloneImage(resizeimage);
		cvSmooth(resizeimage, resizeimage2, CV_MEDIAN, 3, 0, 0, 0);
		cvSaveImage(file, resizeimage2);

		cvSetImageROI(obj->srAnalyzer_img, save_rect);

		cvReleaseImage(&resizeimage);
		cvReleaseImage(&resizeimage2);
		result = 0;
	}

	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_calculateAngleToFit(cco_srAnalyzer *obj,
		cco_vSrPattern *upperleft, cco_vSrPattern *bottomleft, cco_vSrPattern *upperright,
		double *out_angle)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	*out_angle = atan2((upperright->vSrPattern_x - upperleft->vSrPattern_x) * 0.0
			- (upperright->vSrPattern_y - upperleft->vSrPattern_y) * 1.0, (upperright->vSrPattern_x
			- upperleft->vSrPattern_x) * 1.0 + (upperright->vSrPattern_y - upperleft->vSrPattern_y)
			* 0.0);
	return result;
}

int cco_srAnalyzer_calculateCrossProduct(cco_srAnalyzer *obj, cco_vSrPattern *c,
		cco_vSrPattern *v1, cco_vSrPattern *v2)
{
	int S;
	S = (v1->vSrPattern_x - c->vSrPattern_x) * (v2->vSrPattern_y - c->vSrPattern_y)
			- (v1->vSrPattern_y - c->vSrPattern_y) * (v2->vSrPattern_x - c->vSrPattern_x);
	return S;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_countPatterns(cco_srAnalyzer *obj, cco_arraylist *list)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;

	if (cco_arraylist_length(list) != 3)
	{
		if (cco_arraylist_length(list) < 3)
		{
			result = CCOSRANALYZER_STATUS_NOT_FOUND_ADJUSTPATTERN;
		}
		if (cco_arraylist_length(list) > 3)
		{
			result = CCOSRANALYZER_STATUS_MANY_FOUND_ADJUSTPATTERN;
		}
	}

	return result;
}

/**
 * In three pattern points, fits place.
 *
 * @obj This object of cco_srAnalyzer is used by reference.
 * @inout_upperleft This pointer is used to reference and to out a new place.
 * @inout_bottomleft This pointer is used to reference and to out a new place.
 * @inout_upperright This pointer is used to reference and to out a new place.
 */
CCOSRANALYZER_STATUS cco_srAnalyzer_fitPlace(cco_srAnalyzer *obj, cco_vSrPattern **inout_upperleft,
		cco_vSrPattern **inout_bottomleft, cco_vSrPattern **inout_upperright)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_vSrPattern *pattern_a = *inout_upperleft;
	cco_vSrPattern *pattern_b = *inout_bottomleft;
	cco_vSrPattern *pattern_c = *inout_upperright;
	cco_vSrPattern *upperleft = NULL;
	cco_vSrPattern *upperright = NULL;
	cco_vSrPattern *bottomleft = NULL;
	double hero_s;
	double hero_S;
	double hero_a;
	double hero_b;
	double hero_c;
	double angle_upperleft;
	double length_h;
	double rate;

	do
	{
		/* Uses the Hero's formula to get area. */
		hero_a = sqrt(pow((double) (pattern_b->vSrPattern_x - pattern_c->vSrPattern_x), 2) + pow(
				(double) (pattern_b->vSrPattern_y - pattern_c->vSrPattern_y), 2));
		hero_b = sqrt(pow((double) (pattern_a->vSrPattern_x - pattern_c->vSrPattern_x), 2) + pow(
				(double) (pattern_a->vSrPattern_y - pattern_c->vSrPattern_y), 2));
		hero_c = sqrt(pow((double) (pattern_a->vSrPattern_x - pattern_b->vSrPattern_x), 2) + pow(
				(double) (pattern_a->vSrPattern_y - pattern_b->vSrPattern_y), 2));
		hero_s = (hero_a + hero_b + hero_c) / 2.0;
		hero_S = sqrt(hero_s * (hero_s - hero_a) * (hero_s - hero_b) * (hero_s - hero_c));

		/* Decides Upper-left angle. */
		if ((hero_a > hero_b) && (hero_a > hero_c))
		{
			/* The hero_a is most length. and decide candidate of right angle. */
			length_h = (2 * hero_S) / hero_a;
			angle_upperleft = M_PI - (asin(length_h / hero_b) + asin(length_h / hero_c));
			upperleft = pattern_a;
			upperright = pattern_b;
			bottomleft = pattern_c;
		}
		else if ((hero_b > hero_a) && (hero_b > hero_c))
		{
			/* The hero_b is most length. and decide candidate of right angle. */
			length_h = (2 * hero_S) / hero_b;
			angle_upperleft = M_PI - (asin(length_h / hero_a) + asin(length_h / hero_c));
			upperleft = pattern_b;
			upperright = pattern_c;
			bottomleft = pattern_a;
		}
		else
		{
			/* The hero_c is most length. and decide candidate of right angle. */
			length_h = (2 * hero_S) / hero_c;
			angle_upperleft = M_PI - ((asin(length_h / hero_a) + asin(length_h / hero_b)));
			upperleft = pattern_c;
			upperright = pattern_a;
			bottomleft = pattern_b;
		}
		/* If angle is 90, it's OK. */
		if (angle_upperleft > (M_PI / 2.0))
		{
			rate = (M_PI / 2.0) / angle_upperleft;
		}
		else
		{
			rate = angle_upperleft / (M_PI / 2.0);
		}
		if (rate <= CCOSRANALYZER_IMAGE_DISTORTED_RATE)
		{
			result = CCOSRANALYZER_STATUS_DISTORTED_IMAGE;
			break;
		}
		if (cco_srAnalyzer_calculateCrossProduct(obj, upperleft, bottomleft, upperright) > 0)
		{
			/* Modifies way of triangle to fit. */
			cco_vSrPattern *tmp_pattern;
			tmp_pattern = bottomleft;
			bottomleft = upperright;
			upperright = tmp_pattern;
		}
		*inout_upperleft = upperleft;
		*inout_upperright = upperright;
		*inout_bottomleft = bottomleft;
	} while (0);

	return result;
}

/**
 *
 */
CCOSRANALYZER_STATUS cco_srAnalyzer_findPatterns(cco_srAnalyzer *obj, IplImage *image,
		cco_arraylist *added_patternlist)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	IplImage *img_tmp = NULL;
	IplImage *img_prebinarized = NULL;
	IplImage *img_binarized = NULL;
	CvMemStorage *strage_conturs = cvCreateMemStorage(0);
	CvSeq *seq_conturs;
	CvSeq *seq_conturs_parent;
	CvSeq *seq_conturs_child;
	CvRect rect_parent;
	CvRect rect_child;
	cco_vSrPattern *pattern_parent = NULL;
	cco_vSrPattern *pattern_child = NULL;
	cco_arraylist *list_candidate_pattern = added_patternlist;
	cco_arraylist *list_pattern_in_build = NULL;
	cco_vSrPattern *parent_pattern = NULL;
	cco_vSrPattern *candidate_pattern = NULL;

	do
	{
		/* Check loaded image. */
		if (image == NULL)
		{
			result = CCOSRANALYZER_STATUS_NOT_LOAD_IMAGE;
			break;
		}
		/* Makes binarized image and smooth it. */
		img_tmp = cvCloneImage(image);
		img_prebinarized = cvCloneImage(image);
		img_binarized = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
		cvSmooth(image, img_tmp, CV_MEDIAN, 7, 0, 0, 0);
		cvThreshold(img_tmp, img_prebinarized, 130, 255, CV_THRESH_BINARY);
		cvCvtColor(img_prebinarized, img_binarized, CV_BGR2GRAY);
		/* Shows images. */
		if (obj->srAnalyzer_debug >= 2)
		{
			cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "findPatterns: input", image, 3);
			cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "findPatterns: smooth",
					img_binarized, 3);
		}
		/* Examines a image to find contours. */
		seq_conturs = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint),
				strage_conturs);
		cvFindContours(img_binarized, strage_conturs, &seq_conturs, sizeof(CvContour),
				CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		cvReleaseImage(&img_tmp);

		/* Examines a image to find square patterns. */
		img_tmp = cvCloneImage(image);
		pattern_parent = cco_vSrPattern_new();
		pattern_child = cco_vSrPattern_new();
		for (seq_conturs_parent = seq_conturs; seq_conturs_parent != NULL; seq_conturs_parent
				= seq_conturs_parent->h_next)
		{
			rect_parent = cvBoundingRect(seq_conturs_parent, 0);
			cco_vSrPattern_set(pattern_parent, rect_parent.x, rect_parent.y, rect_parent.width,
					rect_parent.height);

			cvRectangle(img_tmp, cvPoint(rect_parent.x, rect_parent.y), cvPoint(
					rect_parent.x + rect_parent.width, rect_parent.y + rect_parent.height),
					CV_RGB(0, 0, 255), 4, 8, 0);

			if (cco_vSrPattern_isSquare(pattern_parent) != 0
					&& cco_vSrPattern_isRectangle1to2(pattern_parent) != 0)
				continue;

			/* Checks child squares in a parent square. */
			list_pattern_in_build = cco_arraylist_new();
			for (seq_conturs_child = seq_conturs; seq_conturs_child != NULL
					&& cco_arraylist_length(list_pattern_in_build) < 3; seq_conturs_child
					= seq_conturs_child->h_next)
			{
				/* Checks candidate square. */
				if (seq_conturs_parent == seq_conturs_child)
					continue;
				rect_child = cvBoundingRect(seq_conturs_child, 0);
				cco_vSrPattern_set(pattern_child, rect_child.x, rect_child.y, rect_child.width,
						rect_child.height);
//				if (!cco_vSrPattern_isSquare(pattern_child))
//					continue;
				if (cco_vSrPattern_isSquare(pattern_child) != 0
						&& cco_vSrPattern_isRectangle1to2(pattern_child) != 0)
					continue;
				if (!cco_vSrPattern_isInside(pattern_parent, pattern_child))
					continue;

				/* Adds a child square to build a pattern. */
				candidate_pattern = cco_vSrPattern_new();
				cco_vSrPattern_set(candidate_pattern, rect_child.x, rect_child.y, rect_child.width,
						rect_child.height);
				cco_arraylist_addAtBack(list_pattern_in_build, candidate_pattern);
				cco_release(candidate_pattern);
			}
			/* Checks double square pattern. */
			if (cco_arraylist_length(list_pattern_in_build) == CCOSRANALYZER_PATTERN_STYLE)
			{
				/* TODO: Examines scale of square and proportion. */
				parent_pattern = cco_vSrPattern_new();
				cco_vSrPattern_set(parent_pattern, rect_parent.x, rect_parent.y, rect_parent.width,
						rect_parent.height);
				if (cco_vSrPattern_isPattern(parent_pattern, list_pattern_in_build))
				{
					/* Draws pattern to image. */
					cvRectangle(img_tmp, cvPoint(rect_parent.x, rect_parent.y), cvPoint(
							rect_parent.x + rect_parent.width, rect_parent.y + rect_parent.height),
							CV_RGB(255, 0, 0), 4, 8, 0);

					/* Adds pattern to list of candidate. */
					candidate_pattern = cco_vSrPattern_new();
					cco_vSrPattern_setInInt(candidate_pattern, rect_parent.x, rect_parent.y,
							rect_parent.width, rect_parent.height);
					cco_arraylist_addAtBack(list_candidate_pattern, candidate_pattern);
					cco_release(candidate_pattern);
				}
				cco_release(parent_pattern);
			}
			cco_release(list_pattern_in_build);
		}
		/* Shows images. */
		if (obj->srAnalyzer_debug >= 2)
		{
			cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "findPatterns: found patterns",
					img_tmp, 3);
		}
	} while (0);
	cco_safeRelease(pattern_parent);
	cco_safeRelease(pattern_child);
	cvReleaseImage(&img_tmp);
	cvReleaseImage(&img_prebinarized);
	cvReleaseImage(&img_binarized);
	cvReleaseMemStorage(&strage_conturs);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_findBoxs(cco_srAnalyzer *obj, IplImage *image,
		cco_arraylist *added_patternlist, double matrix_width, double matrix_height)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	IplImage *img_tmp = NULL;
	IplImage *img_prebinarized = NULL;
	IplImage *img_binarized = NULL;
	CvMemStorage *strage_conturs = cvCreateMemStorage(0);
	CvSeq *seq_conturs;
	CvSeq *seq_conturs_parent;
	CvSeq *seq_conturs_child;
	CvRect rect_parent;
	CvRect rect_child;
	cco_vSrPattern *pattern_parent = NULL;
	cco_arraylist *list_candidate_pattern = added_patternlist;

	float rate_width;
	float rate_height;

	do
	{
		/* Check loaded image. */
		if (image == NULL)
		{
			result = CCOSRANALYZER_STATUS_NOT_LOAD_IMAGE;
			break;
		}
		/* Makes binarized image and smooth it. */
		img_tmp = cvCloneImage(image);
		img_prebinarized = cvCloneImage(image);
		img_binarized = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
		cvSmooth(image, img_tmp, CV_GAUSSIAN, 7, 0, 0, 0);
		cvThreshold(img_tmp, img_prebinarized, 250, 255, CV_THRESH_BINARY);
		cvCvtColor(img_prebinarized, img_binarized, CV_BGR2GRAY);
		/* Shows images. */
		if (obj->srAnalyzer_debug >= 2)
		{
			cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "findPatterns: TEST",
					img_binarized, 3);
		}
		/* Examines a image to find contours. */
		seq_conturs = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint),
				strage_conturs);
		cvFindContours(img_binarized, strage_conturs, &seq_conturs, sizeof(CvContour),
				CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		cvReleaseImage(&img_tmp);

		/* Examines a image to find square patterns. */
		img_tmp = cvCloneImage(image);
		for (seq_conturs_parent = seq_conturs; seq_conturs_parent != NULL; seq_conturs_parent
				= seq_conturs_parent->h_next)
		{
			rect_parent = cvBoundingRect(seq_conturs_parent, 0);
			rate_width = 0.0;
			if (rect_parent.width < matrix_width)
			{
				rate_width = rect_parent.width / matrix_width;
			}
			rate_height = 0.0;
			if (rect_parent.height < matrix_height)
			{
				rate_height = rect_parent.height / matrix_height;
			}
			if (rate_height < 0.85 || rate_width < 0.85)
			{
				continue;
			}
			pattern_parent = cco_vSrPattern_new();
			cco_vSrPattern_set(pattern_parent, rect_parent.x, rect_parent.y, rect_parent.width,
					rect_parent.height);
			cco_arraylist_addAtBack(list_candidate_pattern, pattern_parent);
			cco_safeRelease(pattern_parent);
			cvRectangle(img_tmp, cvPoint(rect_parent.x, rect_parent.y), cvPoint(
					rect_parent.x + rect_parent.width, rect_parent.y + rect_parent.height),
					CV_RGB(0, 0, 255), 4, 8, 0);
		}
		/* Shows images. */
		if (obj->srAnalyzer_debug >= 2)
		{
			cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "findbox: found boxs",
					img_tmp, 2);
		}
	} while (0);
	cco_safeRelease(pattern_parent);
	cvReleaseImage(&img_tmp);
	cvReleaseImage(&img_prebinarized);
	cvReleaseImage(&img_binarized);
	cvReleaseMemStorage(&strage_conturs);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_rotateImage(cco_srAnalyzer *obj, double angle, float cx,
		float cy, IplImage *img, IplImage *dstimg)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	CvPoint2D32f center;
	CvMat *translate = cvCreateMat(2, 3, CV_32FC1);
	center.x = cx;
	center.y = cy;
	cv2DRotationMatrix(center, angle, 1.0, translate);
	cvWarpAffine(img, dstimg, translate, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
	cvReleaseMat(&translate);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ThresholdImageToOcr(cco_srAnalyzer *obj, IplImage *img,
		IplImage *dstimg, int smooth, int threshold)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	IplImage *img_tmp = NULL;

	/* Makes binarized image and smooth it. */
	img_tmp = cvCloneImage(img);
	cvSmooth(img, img_tmp, CV_GAUSSIAN, smooth, 0, 0, 0);
	cvThreshold(img_tmp, dstimg, threshold, 255, CV_THRESH_BINARY);
	cvReleaseImage(&img_tmp);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_examineImageToFindPattern(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_arraylist *list_candidate_pattern = NULL;
	cco_vSrPattern *upperleft = NULL;
	cco_vSrPattern *upperright = NULL;
	cco_vSrPattern *bottomleft = NULL;

	do
	{
		list_candidate_pattern = cco_arraylist_new();
		result = cco_srAnalyzer_findPatterns(obj, obj->srAnalyzer_img, list_candidate_pattern);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}
		/* Count the pattern of adjust to check an error. */
		result = cco_srAnalyzer_countPatterns(obj, list_candidate_pattern);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}
		upperleft = (cco_vSrPattern *) cco_arraylist_getAt(list_candidate_pattern, 0);
		upperright = (cco_vSrPattern *) cco_arraylist_getAt(list_candidate_pattern, 1);
		bottomleft = (cco_vSrPattern *) cco_arraylist_getAt(list_candidate_pattern, 2);
		result = cco_srAnalyzer_fitPlace(obj, &upperleft, &bottomleft, &upperright);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}
		/* Sets information of pattern to this object. */
		cco_release(obj->srAnalyzer_pattern_upperleft);
		cco_release(obj->srAnalyzer_pattern_bottomleft);
		cco_release(obj->srAnalyzer_pattern_upperright);
		obj->srAnalyzer_pattern_upperleft = cco_get(upperleft);
		obj->srAnalyzer_pattern_bottomleft = cco_get(bottomleft);
		obj->srAnalyzer_pattern_upperright = cco_get(upperright);
	} while (0);
	cco_release(list_candidate_pattern);
	cco_release(upperleft);
	cco_release(upperright);
	cco_release(bottomleft);

	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_adjustImage(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	IplImage *img_tmp = NULL;
	double rotate_angle;
	float rotate_center_x;
	float rotate_center_y;
	float pattern_width;
	float pattern_height;
	CvSize rotatedImageSize;

	do
	{
		/* Finds pattern to set informations to this object. */
		result = cco_srAnalyzer_examineImageToFindPattern(obj);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}

		/* Calculates angle of image to rotate in appropriate. */
		result = cco_srAnalyzer_calculateAngleToFit(obj, obj->srAnalyzer_pattern_upperleft,
				obj->srAnalyzer_pattern_bottomleft, obj->srAnalyzer_pattern_upperright,
				&rotate_angle);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}
		pattern_width = (obj->srAnalyzer_pattern_upperright->vSrPattern_width
				+ obj->srAnalyzer_pattern_upperleft->vSrPattern_width
				+ obj->srAnalyzer_pattern_bottomleft->vSrPattern_width) / 3.0;
		pattern_height = (obj->srAnalyzer_pattern_upperright->vSrPattern_height
				+ obj->srAnalyzer_pattern_upperleft->vSrPattern_height
				+ obj->srAnalyzer_pattern_bottomleft->vSrPattern_height) / 3.0;
		rotate_center_x = ((obj->srAnalyzer_pattern_upperright->vSrPattern_x
				+ obj->srAnalyzer_pattern_bottomleft->vSrPattern_x) / 2.0)
				+ (pattern_width / 2.0);
		rotate_center_y = ((obj->srAnalyzer_pattern_upperright->vSrPattern_y
				+ obj->srAnalyzer_pattern_bottomleft->vSrPattern_y) / 2.0)
				+ (pattern_height / 2.0);

		rotatedImageSize = cvGetSize(obj->srAnalyzer_img);
		if (rotatedImageSize.height > rotatedImageSize.width)
		{
			rotatedImageSize.width = rotatedImageSize.height;
		}
		else
		{
			rotatedImageSize.height = rotatedImageSize.width;
		}
		img_tmp = cvCreateImage(rotatedImageSize, obj->srAnalyzer_img->depth,
				obj->srAnalyzer_img->nChannels);
		result = cco_srAnalyzer_rotateImage(obj, rotate_angle * (-180 / M_PI), rotate_center_x,
				rotate_center_y, obj->srAnalyzer_img, img_tmp);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}
		cvReleaseImage(&obj->srAnalyzer_img);
		obj->srAnalyzer_img = img_tmp;
		img_tmp = NULL;
#if 1
		/* Examines this image and Shrinks it to square. If it is square do not shrink. */
		/* TODO: madamada */
		result = cco_srAnalyzer_examineImageToFindPattern(obj);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}
		if (cco_vSrPattern_isRectangle1to2(obj->srAnalyzer_pattern_upperleft) &&
			cco_vSrPattern_isRectangle1to2(obj->srAnalyzer_pattern_upperright) &&
			cco_vSrPattern_isRectangle1to2(obj->srAnalyzer_pattern_bottomleft))
		{
			rotatedImageSize = cvGetSize(obj->srAnalyzer_img);
			if (obj->srAnalyzer_pattern_upperleft->vSrPattern_width
					> obj->srAnalyzer_pattern_upperleft->vSrPattern_height)
			{
				img_tmp = cvCreateImage(
						cvSize(rotatedImageSize.width, rotatedImageSize.height * 2),
						obj->srAnalyzer_img->depth,
						obj->srAnalyzer_img->nChannels);
				cvSetImageROI(img_tmp, cvRect(0,0,rotatedImageSize.width,rotatedImageSize.height*2));
			} else {
				img_tmp = cvCreateImage(
						cvSize(rotatedImageSize.width * 2, rotatedImageSize.height),
						obj->srAnalyzer_img->depth,
						obj->srAnalyzer_img->nChannels);
				cvSetImageROI(img_tmp, cvRect(0,0,rotatedImageSize.width*2,rotatedImageSize.height));
			}
			cvResize(obj->srAnalyzer_img, img_tmp, CV_INTER_NN);
			cvReleaseImage(&obj->srAnalyzer_img);
			obj->srAnalyzer_img = img_tmp;
			img_tmp = NULL;
		}
#endif

	} while (0);

	/* Finalizes this. */
	if (img_tmp != NULL)
	{
		cvReleaseImage(&img_tmp);
	}
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_getIdFromImage(cco_srAnalyzer *obj, cco_vSrPattern *upperleft,
		cco_vSrPattern *bottomleft, cco_vSrPattern *upperright, cco_vString **out_uid,
		cco_vString **out_sid)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_vString *tmp_string = NULL;
	char *tmp_cstring;
	cco_srOcr *ocr = NULL;
	float width;
	float height;
	char tmpfile[512];
	char tmpfiletif[512];
	int tmpfilefd;
	int smooth;

	do {
		ocr = (cco_srOcr *) cco_srOcrGocr_new();
		width = (upperleft->vSrPattern_width + bottomleft->vSrPattern_width
				+ upperright->vSrPattern_width) / 3.0;
		height = (upperleft->vSrPattern_height + bottomleft->vSrPattern_height
				+ upperright->vSrPattern_height) / 3.0;

		snprintf(tmpfile, sizeof(tmpfile), "/tmp/srgetid_XXXXXX");
		tmpfilefd = mkstemp(tmpfile);
		if (tmpfilefd == -1)
		{
			result = CCOSRANALYZER_STATUS_NOT_CREATE_TMPFILE;
			break;
		}
		close(tmpfilefd);
		snprintf(tmpfiletif, sizeof(tmpfiletif), "%s-1.tif", tmpfile);

		smooth = 7;
		/* Gets ID.*/
		cco_safeRelease(*out_uid);
		cco_srAnalyzer_writeImageWithPlaceToOcr(obj, tmpfiletif, upperleft->vSrPattern_x + (width * 2),
				upperleft->vSrPattern_y + (height * 0.05), (width * 5), height * 0.4, smooth, 150);
		ocr->srOcr_setImage(ocr, tmpfiletif);
		ocr->srOcr_setOption(ocr, "ids");
		ocr->srOcr_getRecognizeString(ocr, out_uid);
		remove(tmpfiletif);

		snprintf(tmpfiletif, sizeof(tmpfiletif), "%s-2.tif", tmpfile);
		/* Gets SheetID.*/
		cco_safeRelease(*out_sid);
		cco_srAnalyzer_writeImageWithPlaceToOcr(obj, tmpfiletif, upperleft->vSrPattern_x + (width * 2),
				upperleft->vSrPattern_y + (height * 0.55), (width * 5), height * 0.4, smooth, 150);
		ocr->srOcr_setImage(ocr, tmpfiletif);
		ocr->srOcr_setOption(ocr, "ids");
		ocr->srOcr_getRecognizeString(ocr, out_sid);
		if (obj->srAnalyzer_debug >= 1)
		{
			tmp_string = cco_vString_newWithFormat("Can get IDs from image. ID is %@. Sheet is %@.", *out_uid, *out_sid);
			tmp_cstring = (tmp_string)->v_getCstring(tmp_string);
			printf("%s\n", tmp_cstring);
			free(tmp_cstring);
			cco_safeRelease(tmp_string);
		}
		remove(tmpfiletif);
		remove(tmpfile);
	} while(0);
	cco_safeRelease(ocr);
	cco_safeRelease(tmp_string);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_getOcrbIdFromImage(cco_srAnalyzer *obj, cco_vSrPattern *upperleft,
		cco_vSrPattern *bottomleft, cco_vSrPattern *upperright, cco_vString **out_uid,
		cco_vString **out_sid)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_vString *tmp_string = NULL;
	char *tmp_cstring;
	cco_srOcr *ocr = NULL;
	float width;
	float height;
	char tmpfile[512];
	char tmpfiletif[512];
	int tmpfilefd;
	int smooth;

	do {
#ifdef KOCR
		ocr = (cco_srOcr *) cco_srOcrKocr_new();
#else
		ocr = (cco_srOcr *) cco_srOcrGocr_new();
#endif
		width = (upperleft->vSrPattern_width + bottomleft->vSrPattern_width
				+ upperright->vSrPattern_width) / 3.0;
		height = (upperleft->vSrPattern_height + bottomleft->vSrPattern_height
				+ upperright->vSrPattern_height) / 3.0;

		snprintf(tmpfile, sizeof(tmpfile), "/tmp/srgetid_XXXXXX");
		tmpfilefd = mkstemp(tmpfile);
		if (tmpfilefd == -1)
		{
			result = CCOSRANALYZER_STATUS_NOT_CREATE_TMPFILE;
			break;
		}
		close(tmpfilefd);
		snprintf(tmpfiletif, sizeof(tmpfiletif), "%s-1.tif", tmpfile);

		smooth = 7;
		/* Gets ID.*/
		cco_safeRelease(*out_uid);
		cco_srAnalyzer_writeImageWithPlaceToOcr(obj, tmpfiletif,
				upperleft->vSrPattern_x + (width * 1.5),
				upperleft->vSrPattern_y,
				(width * 5), height, smooth, 150);
		ocr->srOcr_setImage(ocr, tmpfiletif);
		ocr->srOcr_setOption(ocr, "idocrbs");
		ocr->srOcr_getRecognizeString(ocr, out_uid);
		remove(tmpfiletif);

		snprintf(tmpfiletif, sizeof(tmpfiletif), "%s-2.tif", tmpfile);
		/* Gets SheetID.*/
		cco_safeRelease(*out_sid);
		cco_srAnalyzer_writeImageWithPlaceToOcr(obj, tmpfiletif,
				bottomleft->vSrPattern_x + (width * 1.5),
				bottomleft->vSrPattern_y,
				(width * 5), height, smooth, 150);
		ocr->srOcr_setImage(ocr, tmpfiletif);
		ocr->srOcr_setOption(ocr, "idocrbs");
		ocr->srOcr_getRecognizeString(ocr, out_sid);
		if (obj->srAnalyzer_debug >= 1)
		{
			tmp_string = cco_vString_newWithFormat("Can get IDs from image. ID is %@. Sheet is %@.", *out_uid, *out_sid);
			tmp_cstring = (tmp_string)->v_getCstring(tmp_string);
			printf("%s\n", tmp_cstring);
			free(tmp_cstring);
			cco_safeRelease(tmp_string);
		}
		remove(tmpfiletif);
		remove(tmpfile);
	} while(0);
	cco_safeRelease(ocr);
	cco_safeRelease(tmp_string);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ocrProcBlockOcr(cco_srAnalyzer *obj, cco_srMlSheet *sheet,
		cco_redblacktree *keyval)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	double matrix_width;
	double matrix_height;
	int offset_x;
	int offset_y;
	int offset_width;
	int offset_height;

	cco_vXml *xml = NULL;
	cco_arraylist *xml_blockOcrs = NULL;
	cco_vString *xml_attr_name = NULL;
	cco_vString *xml_attr_colspan = NULL;
	cco_vString *xml_attr_x = NULL;
	cco_vString *xml_attr_y = NULL;
	cco_vString *xml_attr_option = NULL;
	char *attr_option_cstring = NULL;
	cco_vString *tmp_string;
	cco_vString *valuekey;
	cco_redblacktree *valuestree;
	char *tmp_cstring;
	int attr_colspan;
	int attr_x;
	int attr_y;
	int index_colspan;
	cco_srOcr *ocr = NULL;
	cco_vString *recgnized_string = NULL;
	char *ocrtype = NULL;
	IplImage *img_tmp = NULL;

	char tmpfile[512];
	int tmpfilefd;

	cco_arraylist *list_candidate_pattern = NULL;
	cco_vSrPattern *pattern = NULL;
	float pattern_x;
	float pattern_y;

	img_tmp = cvClone(obj->srAnalyzer_img);

	do {
		offset_x = obj->srAnalyzer_pattern_upperleft->vSrPattern_x
				+ obj->srAnalyzer_pattern_upperleft->vSrPattern_width;
		offset_y = obj->srAnalyzer_pattern_upperleft->vSrPattern_y
				+ obj->srAnalyzer_pattern_upperleft->vSrPattern_height;
		offset_width = obj->srAnalyzer_pattern_upperright->vSrPattern_x - offset_x;
		offset_height = obj->srAnalyzer_pattern_bottomleft->vSrPattern_y - offset_y;
		matrix_width = (double) offset_width / (double) sheet->srMlSheet_blockWidth;
		matrix_height = (double) offset_height / (double) sheet->srMlSheet_blockHeight;

		list_candidate_pattern = cco_arraylist_new();
		result = cco_srAnalyzer_findBoxs(obj, obj->srAnalyzer_img, list_candidate_pattern, matrix_width, matrix_height);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}

		snprintf(tmpfile, sizeof(tmpfile), "/tmp/srocr_XXXXXX");
		tmpfilefd = mkstemp(tmpfile);
		if (tmpfilefd == -1)
		{
			/* LOG */
			fprintf(stderr, "ERROR: Cannot create a temporary file in cco_srAnalyzer_getIdFromImage.\n");
			break;
		}
		close(tmpfilefd);

		ocrtype = obj->srAnalyzer_ocr->v_getCstring(obj->srAnalyzer_ocr);

		xml_blockOcrs = cco_vXml_getElements(sheet->srMlSheet_xml, "properties/blockOcr");
		cco_arraylist_setCursorAtFront(xml_blockOcrs);
		while ((xml = (cco_vXml *) cco_arraylist_getAtCursor(xml_blockOcrs)) != NULL)
		{
			xml_attr_name = cco_vXml_getAttribute(xml, "name");
			xml_attr_colspan = cco_vXml_getAttribute(xml, "colspan");
			xml_attr_x = cco_vXml_getAttribute(xml, "x");
			xml_attr_y = cco_vXml_getAttribute(xml, "y");
			xml_attr_option = cco_vXml_getAttribute(xml, "option");

			if (xml_attr_name == NULL)
			{
				xml_attr_name = cco_vString_new("BLOCKOCRUNKNOWN");
			}
			if (xml_attr_x != NULL)
			{
				attr_x = cco_vString_toInt(xml_attr_x);
			} else {
				attr_x = 0;
			}
			if (xml_attr_y != NULL)
			{
				attr_y = cco_vString_toInt(xml_attr_y);
			} else {
				attr_y = 0;
			}
			if (xml_attr_colspan != NULL)
			{
				attr_colspan = cco_vString_toInt(xml_attr_colspan);
			} else {
				attr_colspan = 1;
			}
			recgnized_string = cco_vString_new("");
			for (index_colspan = 0; index_colspan < attr_colspan; index_colspan++)
			{

				/* makes the path of image. */
				tmp_string = cco_vString_newWithFormat("%s-%@-%d.png", tmpfile, xml_attr_name,
						index_colspan);
				tmp_cstring = tmp_string->v_getCstring(tmp_string);
				cco_safeRelease(tmp_string);

				/* discovers the box of target. */
				cco_arraylist_setCursorAtFront(list_candidate_pattern);
				pattern_x = (offset_x + matrix_width * attr_x + matrix_width * index_colspan + matrix_width * 0.5);
				pattern_y = (offset_y + matrix_height * attr_y + matrix_width * 0.5);
				while(pattern = (cco_vSrPattern *)cco_arraylist_getAtCursor(list_candidate_pattern))
				{
					if (pattern->vSrPattern_x < pattern_x
							&& (pattern->vSrPattern_x + pattern->vSrPattern_width) > pattern_x
							&& pattern->vSrPattern_y < pattern_y
							&& (pattern->vSrPattern_y + pattern->vSrPattern_height) > pattern_y)
					{
						/* it is a target. */
						break;
					}
					cco_arraylist_setCursorAtNext(list_candidate_pattern);
					cco_safeRelease(pattern);
				}
				if (pattern != NULL)
				{
					/* found a target. */
					cco_srAnalyzer_writeImageWithPlaceToLOcr(obj, tmp_cstring,
							(int) (pattern->vSrPattern_x),
							(int) (pattern->vSrPattern_y),
							(int) (pattern->vSrPattern_width),
							(int) (pattern->vSrPattern_height),
							3, 250);
				} else {
					/* did not find a target. */
					cco_srAnalyzer_writeImageWithPlaceToLOcr(obj, tmp_cstring,
							(int) (offset_x + matrix_width * attr_x + matrix_width * index_colspan + matrix_width * 0.13),
							(int) (offset_y + matrix_height * attr_y + matrix_width * 0.13),
							matrix_width * 0.74,
							matrix_height * 0.74,
							7, 250);
				}
				cco_safeRelease(pattern);
				/* creates an ocr engine. */
				if (strcmp(ocrtype, "gocr") == 0)
				{
					ocr = (cco_srOcr *) cco_srOcrGocr_new();
				} else {
#ifdef KOCR
					ocr = (cco_srOcr *) cco_srOcrKocr_new();
#else
					ocr = (cco_srOcr *) cco_srOcrGocr_new();
#endif
				}
				ocr->srOcr_setImage(ocr, tmp_cstring);
				if (xml_attr_option != NULL)
				{
					attr_option_cstring = xml_attr_option->v_getCstring(xml_attr_option);
					ocr->srOcr_setOption(ocr, attr_option_cstring);
					free(attr_option_cstring);
				}
				ocr->srOcr_getRecognizeString(ocr, &tmp_string);
				cco_vString_catenate(recgnized_string, tmp_string);
				remove(tmp_cstring);
				free(tmp_cstring);
				cco_release(tmp_string);

				cvRectangle(img_tmp,
						cvPoint(offset_x + matrix_width * attr_x, offset_y + matrix_height * attr_y),
						cvPoint(offset_x + matrix_width * attr_x + matrix_width, offset_y + matrix_height * attr_y + matrix_height),
						CV_RGB(255, 0, 0), 4, 8, 0);

			}
			valuekey = cco_vString_new("blockOcr");
			valuestree = (cco_redblacktree *)cco_redblacktree_get(keyval, (cco_v*) xml_attr_name);
			if (valuestree == NULL) {
				valuestree = cco_redblacktree_new();
				cco_redblacktree_insert(keyval, (cco_v*) xml_attr_name, (cco*) valuestree);
			}
			cco_redblacktree_insert(valuestree, (cco_v*) valuekey, (cco*) recgnized_string);
			cco_safeRelease(valuestree);
			cco_safeRelease(valuekey);
			if (obj->srAnalyzer_debug >= 1)
			{
				tmp_string = cco_vString_newWithFormat("name:%@ x:%d y:%d colspan:%d recgnizedText:%@\n",
						xml_attr_name, attr_x, attr_y, attr_colspan, recgnized_string);
				tmp_cstring = tmp_string->v_getCstring(tmp_string);
				printf("%s", tmp_cstring);
				free(tmp_cstring);
				cco_safeRelease(tmp_string);
			}
			cco_safeRelease(recgnized_string);
			cco_safeRelease(xml_attr_name);
			cco_safeRelease(xml_attr_colspan);
			cco_safeRelease(xml_attr_x);
			cco_safeRelease(xml_attr_y);
			cco_arraylist_setCursorAtNext(xml_blockOcrs);
		}
		remove(tmpfile);
		cco_safeRelease(xml_blockOcrs);
	} while (0);
	cco_safeRelease(list_candidate_pattern);
	if (ocrtype != NULL)
	{
		free(ocrtype);
	}
	if (obj->srAnalyzer_debug >= 2)
	{
		cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "ocr", img_tmp, 3);
	}
	cvReleaseImage(&img_tmp);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ocrProcBlockImg(cco_srAnalyzer *obj, cco_srMlSheet *sheet,
		cco_redblacktree *keyval)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	double matrix_width;
	double matrix_height;
	int offset_x;
	int offset_y;
	int offset_width;
	int offset_height;
	double margin_width;
	double margin_height;

	cco_vXml *xml = NULL;
	cco_arraylist *xml_blockOcrs = NULL;
	cco_vString *xml_attr_name = NULL;
	cco_vString *xml_attr_colspan = NULL;
	cco_vString *xml_attr_x = NULL;
	cco_vString *xml_attr_y = NULL;
	cco_vString *xml_attr_margin = NULL;
	cco_vString *tmp_string;
	cco_vString *valuekey;
	cco_redblacktree *valuestree;
	char *tmp_cstring;
	int attr_colspan;
	int attr_x;
	int attr_y;
	int attr_margen;

	offset_x = obj->srAnalyzer_pattern_upperleft->vSrPattern_x
			+ obj->srAnalyzer_pattern_upperleft->vSrPattern_width;
	offset_y = obj->srAnalyzer_pattern_upperleft->vSrPattern_y
			+ obj->srAnalyzer_pattern_upperleft->vSrPattern_height;
	offset_width = obj->srAnalyzer_pattern_upperright->vSrPattern_x - offset_x;
	offset_height = obj->srAnalyzer_pattern_bottomleft->vSrPattern_y - offset_y;
	matrix_width = (double) offset_width / (double) sheet->srMlSheet_blockWidth;
	matrix_height = (double) offset_height / (double) sheet->srMlSheet_blockHeight;

	xml_blockOcrs = cco_vXml_getElements(sheet->srMlSheet_xml, "properties/blockImg");
	cco_arraylist_setCursorAtFront(xml_blockOcrs);
	while ((xml = (cco_vXml *) cco_arraylist_getAtCursor(xml_blockOcrs)) != NULL)
	{
		xml_attr_name = cco_vXml_getAttribute(xml, "name");
		xml_attr_colspan = cco_vXml_getAttribute(xml, "colspan");
		xml_attr_x = cco_vXml_getAttribute(xml, "x");
		xml_attr_y = cco_vXml_getAttribute(xml, "y");
		xml_attr_margin = cco_vXml_getAttribute(xml, "margin");

		if (xml_attr_x != NULL)
		{
			attr_x = cco_vString_toInt(xml_attr_x);
		} else {
			attr_x = 0;
		}
		if (xml_attr_y != NULL)
		{
			attr_y = cco_vString_toInt(xml_attr_y);
		} else {
			attr_y = 0;
		}
		if (xml_attr_colspan != NULL)
		{
			attr_colspan = cco_vString_toInt(xml_attr_colspan);
		} else {
			attr_colspan = 0;
		}
		if (xml_attr_margin != NULL)
		{
			attr_margen = cco_vString_toInt(xml_attr_margin);
			if (attr_margen >= 100) {
				attr_margen = 99;
			}
			if (attr_margen < 0) {
				attr_margen = 0;
			}
		} else {
			attr_margen = 0;
		}
		tmp_string = cco_vString_newWithFormat("%@R%@/S%@/%s",
				obj->srAnalyzer_save_prefix, obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
				obj->srAnalyzer_date_string);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		utility_mkdir(tmp_cstring);
		free(tmp_cstring);
		cco_safeRelease(tmp_string);

		tmp_string = cco_vString_newWithFormat("%@R%@/S%@/%s/blockImg-%@.png",
				obj->srAnalyzer_save_prefix, obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
				obj->srAnalyzer_date_string, xml_attr_name);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		margin_width = (matrix_width * (attr_margen / 100.0)) / 2.0;
		margin_height = (matrix_width * (attr_margen / 100.0)) / 2.0;
		cco_srAnalyzer_writeImageWithPlace(obj, tmp_cstring,
				(int) (offset_x + (matrix_width * attr_x) + margin_width),
				(int) (offset_y + (matrix_height * attr_y) + margin_height),
				(matrix_width * attr_colspan) - (margin_width * 2.0),
				matrix_height - (margin_height * 2.0));
		cco_safeRelease(tmp_string);
		free(tmp_cstring);

		tmp_string = cco_vString_newWithFormat("R%@/S%@/%s/blockImg-%@.png",
				obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
				obj->srAnalyzer_date_string, xml_attr_name);
		valuekey = cco_vString_new("blockImg");
		valuestree = (cco_redblacktree *)cco_redblacktree_get(keyval, (cco_v*) xml_attr_name);
		if (valuestree == NULL) {
			valuestree = cco_redblacktree_new();
			cco_redblacktree_insert(keyval, (cco_v*) xml_attr_name, (cco*) valuestree);
		}
		cco_redblacktree_insert(valuestree, (cco_v*) valuekey, (cco*) tmp_string);
		cco_safeRelease(valuestree);
		cco_safeRelease(valuekey);
		cco_safeRelease(tmp_string);

		cco_safeRelease(xml_attr_name);
		cco_safeRelease(xml_attr_colspan);
		cco_safeRelease(xml_attr_x);
		cco_safeRelease(xml_attr_y);
		cco_arraylist_setCursorAtNext(xml_blockOcrs);
	}
	cco_safeRelease(xml_blockOcrs);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ocr(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_srMlSheet *sheet = NULL;
	cco_vString *userid = NULL;
	cco_vString *sheetid = NULL;

	do
	{
		/* Find pattern to set informations to this object. */
		result = cco_srAnalyzer_examineImageToFindPattern(obj);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}

		/* gets the code of new version. */
		cco_srAnalyzer_getOcrbIdFromImage(obj, obj->srAnalyzer_pattern_upperleft,
				obj->srAnalyzer_pattern_bottomleft, obj->srAnalyzer_pattern_upperright,
				&userid, &sheetid);
		if (cco_vString_length(obj->srAnalyzer_uid) == 0)
		{
			cco_safeRelease(obj->srAnalyzer_uid);
			obj->srAnalyzer_uid = cco_get(userid);
		}
		if (cco_vString_length(obj->srAnalyzer_sid) == 0)
		{
			cco_safeRelease(obj->srAnalyzer_sid);
			obj->srAnalyzer_sid = cco_get(sheetid);
		}
		cco_safeRelease(userid);
		cco_safeRelease(sheetid);
		if (cco_vString_length(obj->srAnalyzer_sid) != 5)
		{
			/* if could not get the code of new version */
			/* gets the code of old version. */
			cco_srAnalyzer_getIdFromImage(obj, obj->srAnalyzer_pattern_upperleft,
					obj->srAnalyzer_pattern_bottomleft, obj->srAnalyzer_pattern_upperright,
					&userid, &sheetid);
			cco_safeRelease(obj->srAnalyzer_uid);
			obj->srAnalyzer_uid = cco_get(userid);
			cco_safeRelease(obj->srAnalyzer_sid);
			obj->srAnalyzer_sid = cco_get(sheetid);
		}

		/* OCR */
		sheet = cco_srMl_getSheet(obj->srAnalyzer_srml, obj->srAnalyzer_sid);
		if (sheet == NULL)
		{
			fprintf(stderr, "ERROR: CCOSRANALYZER_STATUS_NOT_FOUND_SRML\n");
			result = CCOSRANALYZER_STATUS_NOT_FOUND_SRML;
			break;
		}
		cco_srAnalyzer_ocrProcBlockOcr(obj, sheet, obj->srAnalyzer_analyzedData);
		cco_srAnalyzer_ocrProcBlockImg(obj, sheet, obj->srAnalyzer_analyzedData);
	} while (0);
	cco_safeRelease(sheet);
	cco_safeRelease(userid);
	cco_safeRelease(sheetid);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_backupImage(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_srMl *srml = NULL;
	cco_vString *tmp_string = NULL;
	char *tmp_cstring = NULL;
	do
	{
		tmp_string = cco_vString_newWithFormat("%@R%@/S%@/%s",
				obj->srAnalyzer_save_prefix,
				obj->srAnalyzer_sender,
				obj->srAnalyzer_receiver,
				obj->srAnalyzer_date_string);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		utility_mkdir(tmp_cstring);
		free(tmp_cstring);
		cco_safeRelease(tmp_string);

		tmp_string = cco_vString_newWithFormat("%@R%@/S%@/%s/image.png",
				obj->srAnalyzer_save_prefix, obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
				obj->srAnalyzer_date_string);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		cco_srAnalyzer_writeImage(obj, tmp_cstring);
		free(tmp_cstring);
		cco_safeRelease(tmp_string);

		tmp_string = cco_vString_newWithFormat("R%@/S%@/%s/image.png",
				obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
				obj->srAnalyzer_date_string);
		cco_safeRelease(obj->srAnalyzer_backup_image);
		obj->srAnalyzer_backup_image = cco_get(tmp_string);
		cco_safeRelease(tmp_string);
	} while (0);
	cco_safeRelease(srml);
	return result;
}

void cco_srAnalyzer_outXml_sub(cco *callbackobject, cco_v *key, cco *object)
{
	cco_redblacktree *values;
	cco_vString *tmp_string;
	cco_vString *ocrName_string;
	cco_vString *ocrValue_string;
	cco_vString *ocrImage_string;

	values = (cco_redblacktree *)cco_get(object);
	ocrName_string = (cco_vString *)cco_get(key);
	tmp_string = cco_vString_new("blockOcr");
	ocrValue_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (ocrValue_string == NULL)
	{
		ocrValue_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);
	tmp_string = cco_vString_new("blockImg");
	ocrImage_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (ocrImage_string == NULL)
	{
		ocrImage_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);
	tmp_string = cco_vString_newWithFormat(
			" <value>\n"
			"  <ocrName>%@</ocrName>\n"
			"  <ocrValue><![CDATA[%@]]></ocrValue>\n"
			"  <ocrImg><![CDATA[%@]]></ocrImg>\n"
			"  <humanCheck>no</humanCheck>\n"
			" </value>\n", ocrName_string, ocrValue_string, ocrImage_string);
	cco_vString_catenate((cco_vString *)((cco_srAnalyzer *)callbackobject)->srAnalyzer_outXml, tmp_string);
	cco_safeRelease(tmp_string);
	cco_safeRelease(values);
	cco_safeRelease(ocrName_string);
	cco_safeRelease(ocrValue_string);
	cco_safeRelease(ocrImage_string);
	return;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_outXml(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	char *tmp_cstring;

	cco_safeRelease(obj->srAnalyzer_outXml);
	obj->srAnalyzer_outXml = cco_vString_new("<valueList>\n");
	cco_redblacktree_traversePreorder(obj->srAnalyzer_analyzedData, (cco *)obj, cco_srAnalyzer_outXml_sub);
	cco_vString_catenateWithFormat(obj->srAnalyzer_outXml, "</valueList>\n");
	tmp_cstring = obj->srAnalyzer_outXml->v_getCstring(obj->srAnalyzer_outXml);
	printf("%s\n", tmp_cstring);
	free(tmp_cstring);

	return result;
}

void cco_srAnalyzer_outSql_sub(cco *callbackobject, cco_v *key, cco *object)
{
	cco_redblacktree *values;
	cco_vString *tmp_string;
	cco_vString *ocrName_string;
	cco_vString *ocrValue_string;
	cco_vString *ocrImage_string;
	char *ocrName_cstring;
	char *ocrValue_cstring;
	char *ocrImage_cstring;
	char *ocrName_escape;
	char *ocrValue_escape;
	char *ocrImage_escape;
	MYSQL *my;

	values = (cco_redblacktree *)cco_get(object);
	ocrName_string = (cco_vString *)cco_get(key);
	tmp_string = cco_vString_new("blockOcr");
	ocrValue_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (ocrValue_string == NULL)
	{
		ocrValue_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);
	tmp_string = cco_vString_new("blockImg");
	ocrImage_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (ocrImage_string == NULL)
	{
		ocrImage_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);
	ocrName_cstring = ocrName_string->v_getCstring(ocrName_string);
	ocrValue_cstring = ocrValue_string->v_getCstring(ocrValue_string);
	ocrImage_cstring = ocrImage_string->v_getCstring(ocrImage_string);
	ocrName_escape = malloc(strlen(ocrName_cstring) * 2 + 1);
	ocrValue_escape = malloc(strlen(ocrValue_cstring) * 2 + 1);
	ocrImage_escape = malloc(strlen(ocrImage_cstring) * 2 + 1);
	my = mysql_init(NULL);
	mysql_real_escape_string(my, ocrName_escape, ocrName_cstring, strlen(ocrName_cstring));
	mysql_real_escape_string(my, ocrValue_escape, ocrValue_cstring, strlen(ocrValue_cstring));
	mysql_real_escape_string(my, ocrImage_escape, ocrImage_cstring, strlen(ocrImage_cstring));
	mysql_close(my);
	tmp_string = cco_vString_newWithFormat(
			"INSERT INTO response_properties (response_code, ocr_name, ocr_value, ocr_image, need_check)"
			" VALUES('%@%@%s','%s','%s','%s',%s);\n"
			, ((cco_srAnalyzer *)callbackobject)->srAnalyzer_sid
			, ((cco_srAnalyzer *)callbackobject)->srAnalyzer_uid
			, ((cco_srAnalyzer *)callbackobject)->srAnalyzer_date_string
			, ocrName_escape, ocrValue_escape, ocrImage_escape, "TRUE");
	cco_vString_catenate((cco_vString *)((cco_srAnalyzer *)callbackobject)->srAnalyzer_outSql, tmp_string);
	cco_safeRelease(tmp_string);
	free(ocrName_escape);
	free(ocrValue_escape);
	free(ocrImage_escape);
	free(ocrName_cstring);
	free(ocrValue_cstring);
	free(ocrImage_cstring);

	cco_safeRelease(values);
	cco_safeRelease(ocrName_string);
	cco_safeRelease(ocrValue_string);
	cco_safeRelease(ocrImage_string);
	return;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_outSql(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	char *tmp_cstring;
	char *uid_cstring;
	char *sid_cstring;
	char *sender_cstring;
	char *receiver_cstring;
	char *uid_escape;
	char *sid_escape;
	char *sender_escape;
	char *receiver_escape;
	MYSQL *my;

	cco_safeRelease(obj->srAnalyzer_outSql);

	uid_cstring = obj->srAnalyzer_uid->v_getCstring(obj->srAnalyzer_uid);
	sid_cstring = obj->srAnalyzer_sid->v_getCstring(obj->srAnalyzer_sid);
	sender_cstring = obj->srAnalyzer_sender->v_getCstring(obj->srAnalyzer_sender);
	receiver_cstring = obj->srAnalyzer_receiver->v_getCstring(obj->srAnalyzer_receiver);
	uid_escape = malloc(strlen(uid_cstring) * 2 + 1);
	sid_escape = malloc(strlen(sid_cstring) * 2 + 1);
	sender_escape = malloc(strlen(sender_cstring) * 2 + 1);
	receiver_escape = malloc(strlen(receiver_cstring) * 2 + 1);
	my = mysql_init(NULL);
	mysql_real_escape_string(my, uid_escape, uid_cstring, strlen(uid_cstring));
	mysql_real_escape_string(my, sid_escape, sid_cstring, strlen(sid_cstring));
	mysql_real_escape_string(my, sender_escape, sender_cstring, strlen(sender_cstring));
	mysql_real_escape_string(my, receiver_escape, receiver_cstring, strlen(receiver_cstring));
	mysql_close(my);

	obj->srAnalyzer_outSql = cco_vString_newWithFormat(
			"INSERT INTO responses (response_code,date,sheet_code,candidate_code,fax_number,receiver_number,sheet_image)"
			" VALUES('%s%s%s','%s','%s','%s','%s','%s','%@');\n"
			, sid_escape
			, uid_escape
			, obj->srAnalyzer_date_string
			, obj->srAnalyzer_date_string
			, sid_escape
			, uid_escape
			, sender_escape
			, receiver_escape
			, obj->srAnalyzer_backup_image
			);
	cco_redblacktree_traversePreorder(obj->srAnalyzer_analyzedData,
			(cco *)obj, cco_srAnalyzer_outSql_sub);
	tmp_cstring = obj->srAnalyzer_outSql->v_getCstring(obj->srAnalyzer_outSql);
	printf("%s\n", tmp_cstring);
	free(uid_cstring);
	free(sid_cstring);
	free(sender_cstring);
	free(receiver_cstring);
	free(uid_escape);
	free(sid_escape);
	free(sender_escape);
	free(receiver_escape);
	free(tmp_cstring);
	return result;
}

void cco_srAnalyzer_out_sub(cco *callbackobject, cco_v *key, cco *object)
{
	cco_redblacktree *values;
	cco_vString *tmp_string;
	cco_vString *ocrName_string;
	cco_vString *ocrValue_string;
	cco_vString *ocrImage_string;
	cco_vString *outPropertyCode;
	cco_vString *tmpCode1;
	cco_vString *tmpCode2;
	cco_srAnalyzer *obj = (cco_srAnalyzer *)callbackobject;
	char *outCode_cstr;

	values = (cco_redblacktree *)cco_get(object);
	ocrName_string = (cco_vString *)cco_get(key);
	tmp_string = cco_vString_new("blockOcr");
	ocrValue_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (ocrValue_string == NULL)
	{
		ocrValue_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);
	tmp_string = cco_vString_new("blockImg");
	ocrImage_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (ocrImage_string == NULL)
	{
		ocrImage_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);

	outCode_cstr = obj->srAnalyzer_srconf->srConf_outPropertyCode->v_getCstring(obj->srAnalyzer_srconf->srConf_outPropertyCode);
	outPropertyCode = cco_vString_new(outCode_cstr);
	free(outCode_cstr);

	cco_vString_replaceWithCstring(outPropertyCode, "#DATE#", obj->srAnalyzer_date_string);
	cco_vString_replace(outPropertyCode, "#SHEET#", obj->srAnalyzer_sid);
	cco_vString_replace(outPropertyCode, "#CANDIDATE#", obj->srAnalyzer_uid);
	cco_vString_replace(outPropertyCode, "#SENDER#", obj->srAnalyzer_sender);
	cco_vString_replace(outPropertyCode, "#RECEIVER#", obj->srAnalyzer_receiver);
	cco_vString_replace(outPropertyCode, "#SHEETIMAGE#", obj->srAnalyzer_backup_image);
	cco_vString_replace(outPropertyCode, "#OCRNAME#", ocrName_string);
	cco_vString_replace(outPropertyCode, "#OCRVALUE#", ocrValue_string);
	cco_vString_replace(outPropertyCode, "#OCRIMAGE#", ocrImage_string);
	cco_vString_replaceWithCstring(outPropertyCode, "#NEEDCHECK#", "true");
	cco_vString_catenate(obj->srAnalyzer_outProp, outPropertyCode);
	cco_safeRelease(outPropertyCode);

	cco_safeRelease(values);
	cco_safeRelease(ocrName_string);
	cco_safeRelease(ocrValue_string);
	cco_safeRelease(ocrImage_string);
	return;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_out(cco_srAnalyzer *obj, char *sr_result)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_vString *outCode;
	char *outCode_cstr;

	cco_safeRelease(obj->srAnalyzer_out);
	cco_safeRelease(obj->srAnalyzer_outProp);
	outCode_cstr = obj->srAnalyzer_srconf->srConf_outCode->v_getCstring(obj->srAnalyzer_srconf->srConf_outCode);
	obj->srAnalyzer_out = cco_vString_new(outCode_cstr);
	obj->srAnalyzer_outProp = cco_vString_new("");
	free(outCode_cstr);

	cco_vString_replaceWithCstring(obj->srAnalyzer_out, "#DATE#", obj->srAnalyzer_date_string);
	cco_vString_replace(obj->srAnalyzer_out, "#SHEET#", obj->srAnalyzer_sid);
	cco_vString_replace(obj->srAnalyzer_out, "#CANDIDATE#", obj->srAnalyzer_uid);
	cco_vString_replace(obj->srAnalyzer_out, "#SENDER#", obj->srAnalyzer_sender);
	cco_vString_replace(obj->srAnalyzer_out, "#RECEIVER#", obj->srAnalyzer_receiver);
	cco_vString_replace(obj->srAnalyzer_out, "#SHEETIMAGE#", obj->srAnalyzer_backup_image);
	cco_vString_replaceWithCstring(obj->srAnalyzer_out, "#NEEDCHECK#", "true");
	cco_vString_replaceWithCstring(obj->srAnalyzer_out, "#RESULT#", sr_result);
	cco_redblacktree_traversePreorder(obj->srAnalyzer_analyzedData,
			(cco *)obj, cco_srAnalyzer_out_sub);
	cco_vString_replace(obj->srAnalyzer_out, "#PROPERTIES#", obj->srAnalyzer_outProp);
	outCode_cstr = obj->srAnalyzer_out->v_getCstring(obj->srAnalyzer_out);
	printf("%s\n", outCode_cstr);
	free(outCode_cstr);

	return result;
}

void cco_srAnalyzer_resultPrint(cco_srAnalyzer *obj, char *mode, char *sr_result)
{
	CCOSRCONF_STATUS confstatus = CCOSRCONF_STATUS_SUCCESS;
	do {
		if (mode == NULL) {
			cco_srAnalyzer_outXml(obj);
			break;
		}
		if (strcasecmp(mode, "sql") == 0) {
			cco_srAnalyzer_outSql(obj);
			break;
		}
		confstatus = cco_srConf_setCurrentMode(obj->srAnalyzer_srconf, mode);
		if (confstatus != CCOSRCONF_STATUS_SUCCESS)
		{
			cco_srAnalyzer_outXml(obj);
			break;
		}
		cco_srAnalyzer_out(obj, sr_result);
	} while(0);
	return;
}

int cco_srAnalyzer_drawCircle_withImg(cco_srAnalyzer *obj, IplImage *img, int px, int py, int cr,
		int cg, int cb)
{
	CvPoint pt;
	pt = cvPoint(px, py);
	cvCircle(img, pt, 3, CV_RGB(cr, cg, cb), CV_FILLED, 8, 1);
	return 0;
}

int cco_srAnalyzer_countSheets(cco_srAnalyzer *obj)
{
	return cco_srMl_countSheets(obj->srAnalyzer_srml);
}

int cco_srAnalyzer_readSrconf(cco_srAnalyzer *obj, char *file)
{
	CCOSRCONF_STATUS result_srconf = CCOSRCONF_STATUS_SUCCESS;
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_srConf *ml;

	do
	{
		ml = cco_srConf_new();
		result_srconf = cco_srConf_read(ml, file);
		if (result_srconf != CCOSRCONF_STATUS_SUCCESS)
		{
			result = CCOSRANALYZER_STATUS_NOT_READ_SRCONF;
			break;
		}
		obj->srAnalyzer_srconf = cco_get(ml);
	} while (0);
	cco_release(ml);
	return result;
}


int cco_srAnalyzer_showImage_createWindow(cco_srAnalyzer *obj, char* windowname, IplImage *img)
{
	cvNamedWindow(windowname, 1);
	cvShowImage(windowname, img);
	cvResizeWindow(windowname, 300, 300);
	return 0;
}

int cco_srAnalyzer_showImage_destroyWindow(cco_srAnalyzer *obj, char* windowname)
{
	cvDestroyWindow(windowname);
	return 0;
}

int cco_srAnalyzer_showImageNow_withImage(cco_srAnalyzer *obj, char* windowname, IplImage *img)
{
	cvNamedWindow(windowname, 1);
	cvShowImage(windowname, img);
	cvResizeWindow(windowname, 300, 300);
	cvWaitKey(0);
	cvDestroyWindow(windowname);
	return 0;
}

int cco_srAnalyzer_showShrinkedImageNow_withImage(cco_srAnalyzer *obj, char* windowname,
		IplImage *img, int shrink)
{
	CvSize shrinksize;
	IplImage *shrinkimage;

	shrinksize = cvGetSize(img);
	shrinksize.width = shrinksize.width / shrink;
	shrinksize.height = shrinksize.height / shrink;

	shrinkimage = cvCreateImage(shrinksize, img->depth, img->nChannels);
	cvResize(img, shrinkimage, CV_INTER_NN);
	cvNamedWindow(windowname, 1);
	cvShowImage(windowname, shrinkimage);
	cvWaitKey(0);
	cvDestroyWindow(windowname);
	cvReleaseImage(&shrinkimage);
	return 0;
}

int cco_srAnalyzer_showImageNow(cco_srAnalyzer *obj, char* windowname)
{
	cco_srAnalyzer_showImageNow_withImage(obj, windowname, obj->srAnalyzer_img);
	return 0;
}

int cco_srAnalyzer_showShrinkedImageNow(cco_srAnalyzer *obj, char* windowname, int shrink)
{
	cco_srAnalyzer_showShrinkedImageNow_withImage(obj, windowname, obj->srAnalyzer_img, shrink);
	return 0;
}

