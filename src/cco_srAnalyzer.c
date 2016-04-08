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

#include "cco_srOcrKocr.h"
#include "cco_srOcrGocr.h"
/* #include "cco_srOcrNhocr.h" */

#include "cco_vSrPattern.h"
#include "cco_srMl.h"
#include "cco_srMlSheet.h"
#include "cco_srConf.h"
#include "utility.h"

cco_defineClass(cco_srAnalyzer)
;

int cco_srAnalyzer_setOcrObj(cco_srAnalyzer *obj, cco_vString *ocr_type);

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
	cco_srAnalyzer_baseFinalize((cco_srAnalyzer *)o);
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
	o->srAnalyzer_ocr_type = cco_vString_new("kocr"); /* nhocr */
#else
	o->srAnalyzer_ocr_type = cco_vString_new("gocr");
#endif
	o->srAnalyzer_ocr_obj = NULL;
	cco_srAnalyzer_setOcrObj(o, o->srAnalyzer_ocr_type);
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
	cco_release(o->srAnalyzer_ocr_type);
	cco_release(o->srAnalyzer_ocr_obj);
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

extern char *tmpdir_prefix;

struct ocr_engine
{
	char *ocr_type_name;
	cco_srOcr* (*factory)();
};

static struct ocr_engine ocr_engine_list[] = {
	{ "gocr", (cco_srOcr* (*)())cco_srOcrGocr_new },
	/* { "nhocr", (cco_srOcr* (*)())cco_srOcrNhocr_new }, */
#ifdef KOCR
	{ "kocr", (cco_srOcr* (*)())cco_srOcrKocr_new },
#endif
};

int cco_srAnalyzer_setOcrObj(cco_srAnalyzer *obj, cco_vString *ocr_type)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	char *ocr_type_name_in_use = ocr_type->v_getCstring(ocr_type);
	int supported_ocr_engine_flag = 0;
	struct ocr_engine *cur_ptr;
	for (cur_ptr = ocr_engine_list; cur_ptr != NULL; cur_ptr++)
	{
		if (strcmp(cur_ptr->ocr_type_name, ocr_type_name_in_use) == 0)
		{
			cco_safeRelease(obj->srAnalyzer_ocr_obj);
			obj->srAnalyzer_ocr_obj = (cco_srOcr *)cur_ptr->factory();
			supported_ocr_engine_flag = 1;
			break;
		}
	}
	if (supported_ocr_engine_flag == 0)
	{
		result = CCOSRANALYZER_STATUS_UNSUPPORTED_OCR_ENGINE;
	}
	return result;
}

/**
 * save current ROI, do some action and restore the ROI
 *
 * x, y, width, height: ROI
 * needClone: 0=do not clone, 1:clone
 * doAction: action
 */
int cco_srAnalyzer_doSomeActionToROI(cco_srAnalyzer *obj,
		int x, int y, int width, int height,
		int needClone,
		int (*doAction)(IplImage *))
{
	int result = -1;
	CvRect saved_rect;
	IplImage *clonedImage;

	if (obj->srAnalyzer_img == NULL)
	{
		return result;
	}

	saved_rect = cvGetImageROI(obj->srAnalyzer_img);
	cvSetImageROI(obj->srAnalyzer_img, cvRect(x, y, width, height));
	if (needClone == 1) {
		clonedImage = (IplImage *)cvClone(obj->srAnalyzer_img);
		result = doAction(clonedImage);
		cvReleaseImage(&clonedImage);
	}
	else
	{
		result = doAction(obj->srAnalyzer_img);
	}
	cvSetImageROI(obj->srAnalyzer_img, saved_rect);

	return result;
}



CCOSRANALYZER_STATUS cco_srAnalyzer_setOcrEngine(cco_srAnalyzer *obj, char *ocr_type_name)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	int result1 = 0;

	cco_safeRelease(obj->srAnalyzer_ocr_type);
	obj->srAnalyzer_ocr_type = cco_vString_new(ocr_type_name);
	result1 = cco_srAnalyzer_setOcrObj(obj, obj->srAnalyzer_ocr_type);
	if (result1 == CCOSRANALYZER_STATUS_UNSUPPORTED_OCR_ENGINE)
	{
		/* exception */
		fprintf(stderr, "unsupported OCR engine:%s\n", ocr_type_name);
		exit(result1);
	}
	return result;
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
	return CCOSRANALYZER_STATUS_SUCCESS;
}

int cco_srAnalyzer_writeImage(cco_srAnalyzer *obj, char *file)
{
#ifdef COMPRESSION
	static int params[] = {CV_IMWRITE_PNG_COMPRESSION, 9, -1};
#else
	char conv_str[4096];
	sprintf(conv_str, "convert %s -quality 80 %s", file, file);
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
	int action(IplImage *image)
	{
		cvSaveImage(file, image);
		return 0;
	}

	return cco_srAnalyzer_doSomeActionToROI(
			obj,
			x, y, width, height,
			0,
			action);
}

CCOSRANALYZER_STATUS cco_srAnalyzer_writeImageWithPlaceToOcr(cco_srAnalyzer *obj, char *file,
		int x, int y, int width, int height, int smooth, int threshold)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	CvRect save_rect;
	CvRect set_rect;
	IplImage *tmp_img;

	int action(IplImage *image)
	{
		CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;

		result = cco_srAnalyzer_ThresholdImageToOcr(obj, obj->srAnalyzer_img, image, smooth, threshold);
		cvSaveImage(file, image);
		return result;
	}

	return cco_srAnalyzer_doSomeActionToROI(
			obj,
			x, y, width, height,
			1,
			action);
}

CCOSRANALYZER_STATUS cco_srAnalyzer_writeImageWithPlaceToLOcr(cco_srAnalyzer *obj, char *file,
		int x, int y, int width, int height, int smooth, int threshold)
{

	int action(IplImage *image)
	{
		CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
		IplImage *tmp_img;

		if (1) {
			tmp_img = cvCreateImage(cvSize(width * 2, height * 2), obj->srAnalyzer_img->depth, obj->srAnalyzer_img->nChannels);
			cvRectangle(tmp_img, cvPoint(0,0), cvPoint(width * 2, height * 2), cvScalar(0xff, 0xff, 0xff, 0), CV_FILLED, 0, 0);
			cvSetImageROI(tmp_img, cvRect(width / 2, height / 2, width, height));
		}
		else {	// for debug
			tmp_img = cvCreateImage(cvSize(width, height), obj->srAnalyzer_img->depth, obj->srAnalyzer_img->nChannels);
			cvRectangle(tmp_img, cvPoint(0,0), cvPoint(width, height), cvScalar(0xff, 0xff, 0xff, 0), CV_FILLED, 0, 0);
			cvSetImageROI(tmp_img, cvRect(0, 0, width, height));
		}
		result = cco_srAnalyzer_ThresholdImageToOcr(obj, obj->srAnalyzer_img, tmp_img, smooth, threshold);
		cvResetImageROI(tmp_img);
		cvSaveImage(file, tmp_img);
		cvReleaseImage(&tmp_img);

		return result;
	}

	return cco_srAnalyzer_doSomeActionToROI(
			obj,
			x, y, width, height,
			0,
			action);
}

int cco_srAnalyzer_writeResizeImageWithPlace(cco_srAnalyzer *obj, char *file, int x, int y,
		int width, int height, int resize_width, int resize_height)
{
	int action(IplImage *image)
	{
		CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
		CvSize resize;
		CvRect set_rect;
		IplImage *resizeimage;
		IplImage *resizeimage2;

		resize_width = 400;
		resize_height = 200;
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

		cvReleaseImage(&resizeimage);
		cvReleaseImage(&resizeimage2);

		return result;
	}

	return cco_srAnalyzer_doSomeActionToROI(
			obj,
			x, y, width, height,
			0,
			action);

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
static cco_vString *marker_sample_image_filename;
CCOSRANALYZER_STATUS cco_srAnalyzer_setMarkerSampleImageFile(char *filename)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	struct stat st;

	if (stat(filename, &st) != 0)
	{
		return CCOSRANALYZER_STATUS_NOT_LOAD_IMAGE;
	}
	marker_sample_image_filename = cco_vString_new(filename);

	return result;
}

/**
 *
 */
CCOSRANALYZER_STATUS cco_srAnalyzer_getAccuracyByComparingWithSampleMarker(cco_srAnalyzer *obj, double *out_accuracy, IplImage *image, CvRect *marker_rect)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;

	IplImage *target_marker_img = NULL;
	IplImage *target_marker_img_gray = NULL;
	static IplImage *sample_image = NULL;

	if (sample_image == NULL)
	{
		sample_image = cvLoadImage(marker_sample_image_filename->v_getCstring(marker_sample_image_filename), CV_LOAD_IMAGE_GRAYSCALE);
	}

	target_marker_img = (IplImage *)cvClone(image);
	target_marker_img_gray = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	cvCvtColor(target_marker_img, target_marker_img_gray, CV_BGR2GRAY);
	cvSetImageROI(target_marker_img_gray, *marker_rect);
	*out_accuracy = cvMatchShapes(target_marker_img_gray, sample_image, CV_CONTOURS_MATCH_I2, 0);
	if (obj->srAnalyzer_debug >= 3)
	{
		cco_srAnalyzer_showShrinkedImageNow_withImage(obj,
				"cco_srAnalyzer_getAccuracyByComparingWithSampleMarker: target marker",
				target_marker_img_gray, 1);
	}

	cvReleaseImage(&target_marker_img_gray);
	cvReleaseImage(&target_marker_img);

	return result;
}

/**
 *
 */
int cco_srAnalyzer_compareMarkerWithSampleImage(cco_srAnalyzer *obj, IplImage *image, CvRect *marker_rect)
{
	int result = 0;

	double accuracy = 0.0;
	const double accuracy_threshold = 0.2;	/* XXX: must consider the appropriate value */
	cco_srAnalyzer_getAccuracyByComparingWithSampleMarker(obj, &accuracy, image, marker_rect);
	if (obj->srAnalyzer_debug >= 1)
	{
		fprintf(stderr, "Compare the marker with sample marker: target marker's accuracy=%lf: threshold=%lf\n", accuracy, accuracy_threshold);
	}
	if (accuracy < accuracy_threshold)
	{
		result = 1;
	}
	return result;
}

/**
 *
 */
CCOSRANALYZER_STATUS cco_srAnalyzer_getTop3PatternByComparingWithSampleMarker(cco_srAnalyzer *obj,
		cco_arraylist *list_candidate_pattern, cco_arraylist *out_top3_patterns)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	int num_of_patterns = cco_arraylist_length(list_candidate_pattern);
	struct patterns_with_accuracy
	{
		double accuracy;
		cco_vSrPattern *pattern;
	} patterns[num_of_patterns];
	double accuracy_list[num_of_patterns];
	cco_vSrPattern *pattern = NULL;
	int i = 0;
	int j = 0;

	int compare_accuracy(const void* v1, const void* v2)
	{
		const double *p1 = (double *)v1;
		const double *p2 = (double *)v2;
		if (*p1 > *p2)
			return 1;
		else if (*p1 < *p2)
			return -1;
		else
			return 0;

	}

	cco_arraylist_setCursorAtFront(list_candidate_pattern);
	while ((pattern = (cco_vSrPattern *) cco_arraylist_getAtCursor(list_candidate_pattern)) != NULL && i < num_of_patterns)
	{
		CvRect rect;
		rect.x = pattern->vSrPattern_x;
		rect.y = pattern->vSrPattern_y;
		rect.width = pattern->vSrPattern_width;
		rect.height = pattern->vSrPattern_height;
		patterns[i].pattern = pattern;
		cco_srAnalyzer_getAccuracyByComparingWithSampleMarker(obj, &patterns[i].accuracy, obj->srAnalyzer_img, &rect);
		accuracy_list[i] = patterns[i].accuracy;
		cco_arraylist_setCursorAtNext(list_candidate_pattern);
		++i;
	}
	qsort(accuracy_list, num_of_patterns, sizeof(double), compare_accuracy);
	IplImage *tmp_img = (IplImage *)cvClone(obj->srAnalyzer_img);
	for (i = 0; i < num_of_patterns; i++)	/* keep the order as far as we can */
	{
		for (j = 0; j < 3; j++) {
			if (patterns[i].accuracy == accuracy_list[j])
			{
				cco_arraylist_addAtBack(out_top3_patterns, patterns[i].pattern);
				if (obj->srAnalyzer_debug >= 2)
				{
					cvRectangle(tmp_img, cvPoint(patterns[i].pattern->vSrPattern_x, patterns[i].pattern->vSrPattern_y),
							cvPoint(patterns[i].pattern->vSrPattern_x + patterns[i].pattern->vSrPattern_width,
								patterns[i].pattern->vSrPattern_y + patterns[i].pattern->vSrPattern_height),
							CV_RGB(255,0,0), 4, 8, 0);
				}
				break;
			}
		}
	}
	if (obj->srAnalyzer_debug >= 2)
	{
		cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "cco_srAnalyzer_getTop3PatternByComparingWithSampleMarker: top3 patterns", tmp_img, 3);
	}
	cvReleaseImage(&tmp_img);

	return result;
}

/**
 *
 */
CCOSRANALYZER_STATUS cco_srAnalyzer_getTop3Pattern(cco_srAnalyzer *obj,
		cco_arraylist *list_candidate_pattern, cco_arraylist *out_top3_patterns)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	result = cco_srAnalyzer_getTop3PatternByComparingWithSampleMarker(obj, list_candidate_pattern, out_top3_patterns);
	return result;
}

/**
 *
 */
CCOSRANALYZER_STATUS cco_srAnalyzer_findPatternsInSpecifiedArea(
		cco_srAnalyzer *obj, IplImage *image,
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

	/* Examines an image to find contours. */
	seq_conturs = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint),
			strage_conturs);
	cvFindContours(img_binarized, strage_conturs, &seq_conturs, sizeof(CvContour),
			CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
	cvReleaseImage(&img_tmp);

	/* Examines an image to find square patterns. */
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
			if (!cco_vSrPattern_isInside(pattern_parent, pattern_child))
				continue;
			/* if (!cco_vSrPattern_isSquare(pattern_child))
				continue; */
			if (cco_vSrPattern_isSquare(pattern_child) != 0
					&& cco_vSrPattern_isRectangle1to2(pattern_child) != 0)
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
		/* Examines an image to find contours. */
		seq_conturs = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint),
				strage_conturs);
		cvFindContours(img_binarized, strage_conturs, &seq_conturs, sizeof(CvContour),
				CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		cvReleaseImage(&img_tmp);

		/* Examines an image to find square patterns. */
		if (obj->srAnalyzer_debug >= 2)
		{
			img_tmp = cvCloneImage(image);
		}
		pattern_parent = cco_vSrPattern_new();
		pattern_child = cco_vSrPattern_new();
		for (seq_conturs_parent = seq_conturs; seq_conturs_parent != NULL; seq_conturs_parent
				= seq_conturs_parent->h_next)
		{
			rect_parent = cvBoundingRect(seq_conturs_parent, 0);
			cco_vSrPattern_set(pattern_parent, rect_parent.x, rect_parent.y, rect_parent.width,
					rect_parent.height);

			if (obj->srAnalyzer_debug >= 2)
			{
				cvRectangle(img_tmp, cvPoint(rect_parent.x, rect_parent.y), cvPoint(
						rect_parent.x + rect_parent.width, rect_parent.y + rect_parent.height),
						CV_RGB(0, 0, 255), 4, 8, 0);
			}

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
				if (!cco_vSrPattern_isInside(pattern_parent, pattern_child))
					continue;
				/* if (!cco_vSrPattern_isSquare(pattern_child))
					continue; */
				if (cco_vSrPattern_isSquare(pattern_child) != 0
						&& cco_vSrPattern_isRectangle1to2(pattern_child) != 0)
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
					if (obj->srAnalyzer_debug >= 2)
					{
						cvRectangle(img_tmp, cvPoint(rect_parent.x, rect_parent.y), cvPoint(
								rect_parent.x + rect_parent.width, rect_parent.y + rect_parent.height),
								CV_RGB(255, 0, 0), 4, 8, 0);
					}

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

CCOSRANALYZER_STATUS cco_srAnalyzer_findBoxes(cco_srAnalyzer *obj, IplImage *image,
		cco_arraylist *added_patternlist, double matrix_width, double matrix_height)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	IplImage *img_tmp = NULL;
	IplImage *img_prebinarized = NULL;
	IplImage *img_binarized = NULL;
	CvMemStorage *strage_conturs = cvCreateMemStorage(0);
	CvSeq *seq_conturs;
	CvSeq *seq_conturs_parent;
	CvRect rect_parent;
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
		/* Examines an image to find contours. */
		seq_conturs = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint),
				strage_conturs);
		cvFindContours(img_binarized, strage_conturs, &seq_conturs, sizeof(CvContour),
				CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
		cvReleaseImage(&img_tmp);

		/* Examines an image to find square patterns. */
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
				/* continue; */
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
			cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "findbox: found boxes",
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

/*
 * find 3 markers from a fax image
 */
CCOSRANALYZER_STATUS cco_srAnalyzer_examineImageToFindPattern(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_arraylist *list_candidate_pattern = NULL;
	cco_arraylist *list_top3_candidate_pattern = NULL;
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
		if (obj->srAnalyzer_debug >= 1)
			printf("number of patterns found:%d\n", cco_arraylist_length(list_candidate_pattern));
		upperleft = (cco_vSrPattern *) cco_arraylist_getAt(list_candidate_pattern, 0);
		upperright = (cco_vSrPattern *) cco_arraylist_getAt(list_candidate_pattern, 1);
		bottomleft = (cco_vSrPattern *) cco_arraylist_getAt(list_candidate_pattern, 2);

		list_top3_candidate_pattern = cco_arraylist_new();
		result = cco_srAnalyzer_countPatterns(obj, list_candidate_pattern);
		if (result == CCOSRANALYZER_STATUS_NOT_FOUND_ADJUSTPATTERN)
		{
			break;
		}
		else if (result == CCOSRANALYZER_STATUS_MANY_FOUND_ADJUSTPATTERN)
		{
			if (obj->srAnalyzer_debug >= 1)
				printf("use top 3 patterns from %d patterns\n", cco_arraylist_length(list_candidate_pattern));
			result = cco_srAnalyzer_getTop3Pattern(obj, list_candidate_pattern, list_top3_candidate_pattern);
			upperleft = (cco_vSrPattern *) cco_arraylist_getAt(list_top3_candidate_pattern, 0);
			upperright = (cco_vSrPattern *) cco_arraylist_getAt(list_top3_candidate_pattern, 1);
			bottomleft = (cco_vSrPattern *) cco_arraylist_getAt(list_top3_candidate_pattern, 2);
		}
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
	cco_release(list_top3_candidate_pattern);
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

		snprintf(tmpfile, sizeof(tmpfile), "%s/srgetid_XXXXXX", tmpdir_prefix);
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
	} while (0);
	cco_safeRelease(ocr);
	cco_safeRelease(tmp_string);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_getOcrbIdFromImage(cco_srAnalyzer *obj, cco_vSrPattern *upperleft,
		cco_vSrPattern *bottomleft, cco_vSrPattern *upperright, cco_vString **out_uid,
		cco_vString **out_sid, char *ocr_db)
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
		ocr = obj->srAnalyzer_ocr_obj;
		width = (upperleft->vSrPattern_width + bottomleft->vSrPattern_width
				+ upperright->vSrPattern_width) / 3.0;
		height = (upperleft->vSrPattern_height + bottomleft->vSrPattern_height
				+ upperright->vSrPattern_height) / 3.0;

		snprintf(tmpfile, sizeof(tmpfile), "%s/srgetid_XXXXXX", tmpdir_prefix);
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
		ocr->srOcr_setOption(ocr, ocr_db);
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
		ocr->srOcr_setOption(ocr, ocr_db);
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
	} while (0);
	cco_safeRelease(tmp_string);
	return result;
}

double cco_srAnalyzer_vString_toDouble(cco_vString *string)
{
	char *cstring;
	double result = -1;

	if (string != NULL)
	{
		cstring = cco_vString_getCstring(string);
		result = atof(cstring);
		free(cstring);
	}
	return result;
}

/*
 * Get the position of top-left corner
 *
 * Input: index: cell number: 0 origin without marker
 */
double cco_srAnalyzer_get_position_of_the_cell_withoutMarker(cco_redblacktree *cell_size_list, int index)
{
	double position = 0;
	int i;

	for (i = 0; i < index; i++) {
		position += cco_srMlSheet_getCellWidthOrHeight(cell_size_list, i + 1);
	}

	return position;
}
/*
 * Get the size(width or height) of target cell
 *
 * Input: index: cell number: 0 origin without marker
 */
double cco_srAnalyzer_get_size_of_the_cell_withoutMarker(cco_redblacktree *cell_size_list, int index)
{
	return cco_srMlSheet_getCellWidthOrHeight(cell_size_list, index + 1);
}

int cco_srAnalyzer_getCellXnumberByColspan(cco_srMlSheet *sheet, int start_x, int start_y, int index_colspan)
{
	int x;
	int y;
	int i;

	x = start_x;
	y = start_y;
	for (i = 0; i < index_colspan; i++)
	{
		int cell_merging_colspan = cco_srMlSheet_getCellColspan(sheet, y, x);
		if (cell_merging_colspan <= 1)
		{
			cell_merging_colspan = 1;
		}
		x += cell_merging_colspan;
	}

	return x;
}

int cco_srAnalyzer_getCellXnumberByRowspan(cco_srMlSheet *sheet, int start_x, int start_y, int index_rowspan)
{
	int x;
	int y;
	int i;

	x = start_x;
	y = start_y;
	for (i = 0; i < index_rowspan; i++)
	{
		int cell_merging_rowspan = cco_srMlSheet_getCellRowspan(sheet, y, x);
		if (cell_merging_rowspan <= 1)
		{
			cell_merging_rowspan = 1;
		}
		y += cell_merging_rowspan;
	}

	return y;
}

double cco_srAnalyzer_getCurrentMergedCellWidth(cco_srMlSheet *sheet, int cell_x, int cell_y, int index_colspan)
{
	int x;
	int cell_merging_colspan;
	double current_merged_cell_width = 0.0;
	int i;

	x = cco_srAnalyzer_getCellXnumberByColspan(sheet, cell_x, cell_y, index_colspan);
	cell_merging_colspan = cco_srMlSheet_getCellColspan(sheet, cell_y, x);
	for (i = 0; i < cell_merging_colspan; i++)
	{
		current_merged_cell_width += cco_srAnalyzer_get_size_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, x + i);
	}
	assert(current_merged_cell_width != 0.0);

	return current_merged_cell_width;
}

double cco_srAnalyzer_getCurrentMergedCellHeight(cco_srMlSheet *sheet, int cell_x, int cell_y, int index_colspan)
{
	int x;
	int cell_merging_rowspan;
	double current_merged_cell_height = 0.0;
	int i;

	x = cco_srAnalyzer_getCellXnumberByColspan(sheet, cell_x, cell_y, index_colspan);
	cell_merging_rowspan = cco_srMlSheet_getCellRowspan(sheet, cell_y, x);
	for (i = 0; i < cell_merging_rowspan; i++)
	{
		current_merged_cell_height += cco_srAnalyzer_get_size_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, cell_y + i);
	}
	assert(current_merged_cell_height != 0.0);

	return current_merged_cell_height;
}

/*
 * Get the position of top-left corner
 *
 */
double cco_srAnalyzer_get_position_of_the_cell_withoutMarker_by_colspan(cco_redblacktree *cell_size_list, cco_srMlSheet *sheet, int cell_x, int cell_y, int index_colspan)
{
	double position = 0;
	int i;
	int x;

	x = cco_srAnalyzer_getCellXnumberByColspan(sheet, cell_x, cell_y, index_colspan);

	for (i = 0; i < x; i++)
	{
		position += cco_srMlSheet_getCellWidthOrHeight(cell_size_list, i + 1);
	}

	return position;
}

double cco_srAnalyzer_get_position_of_the_cell_withoutMarker_by_rowspan(cco_redblacktree *cell_size_list, cco_srMlSheet *sheet, int cell_x, int cell_y, int index_rowspan)
{
	double position = 0;
	int i;
	int y;

	y = cco_srAnalyzer_getCellXnumberByRowspan(sheet, cell_x, cell_y, index_rowspan);

	for (i = 0; i < y; i++)
	{
		position += cco_srMlSheet_getCellWidthOrHeight(cell_size_list, i + 1);
	}

	return position;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_getCandidateFromContourList(
	cco_srAnalyzer *obj,
	int first_callP,
	cco_arraylist *list_candidate_pattern,
	int (*func)(cco_vSrPattern *pattern),
	cco_vSrPattern **out_pattern)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_vSrPattern *pattern = NULL;
	int found = 0;

	if (first_callP == 1)
	{
		cco_arraylist_setCursorAtBack(list_candidate_pattern);
	}
	while (pattern = (cco_vSrPattern *)cco_arraylist_getAtCursor(list_candidate_pattern))
	{
		found = func(pattern);
		if (found == 1)
		{
			break;
		}
		cco_arraylist_setCursorAtPrevious(list_candidate_pattern);
		cco_safeRelease(pattern);
	}
	*out_pattern = pattern;

	return result;
}

int cco_srAnalyzer_concat_preOcrImageFiles(
		cco_vString *src_file_pattern,
		cco_vString *output_filename)
{
	cco_vString *cmd_string = NULL;
	char *cmd_cstring = NULL;
	int return_value = 0;

	cmd_string = cco_vString_newWithFormat(
			"montage %@ -tile x1 %@",
			src_file_pattern, output_filename);
	cmd_cstring = cmd_string->v_getCstring(cmd_string);
	cco_release(cmd_string);
	return_value = system(cmd_cstring);
	free(cmd_cstring);

	return return_value;
}

int cco_srAnalyzer_get_margin_from_xml_attribute(cco_vString *xml_attr_margin, int default_margin)
{
	int result_margin = 0;
	if (xml_attr_margin != NULL)
	{
		result_margin = cco_vString_toInt(xml_attr_margin);
	} else {
		result_margin = default_margin;
	}
	return result_margin;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ocrProcBlockOcr(cco_srAnalyzer *obj, cco_srMlSheet *sheet,
		cco_redblacktree *keyval)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	double marker_width;
	double marker_height;
	double current_cell_x;
	double current_cell_y;
	double current_cell_width_scaled;
	double current_cell_height_scaled;
	int offset_x;
	int offset_y;
	int offset_width;
	int offset_height;
	double offset_width_excel;
	double offset_height_excel;

	cco_vXml *xml = NULL;
	cco_arraylist *xml_blockOcrs = NULL;
	cco_vString *xml_attr_name = NULL;
	cco_vString *xml_attr_colspan = NULL;
	cco_vString *xml_attr_rowspan = NULL;
	cco_vString *xml_attr_x = NULL;
	cco_vString *xml_attr_y = NULL;
	cco_vString *xml_attr_option = NULL;
	cco_vString *xml_attr_margin = NULL;
	cco_vString *xml_attr_margin_top = NULL;
	cco_vString *xml_attr_margin_bottom = NULL;
	cco_vString *xml_attr_margin_right = NULL;
	cco_vString *xml_attr_margin_left = NULL;
	char *attr_option_cstring = NULL;
	cco_vString *tmp_string;
	cco_vString *valuekey;
	cco_redblacktree *valuestree;
	char *tmp_cstring;
	int attr_margin;
	int attr_margin_top;
	int attr_margin_bottom;
	int attr_margin_right;
	int attr_margin_left;
	int attr_colspan;
	int attr_rowspan;
	int attr_x;
	int attr_y;
	int index_colspan;
	cco_srOcr *ocr = NULL;
	cco_vString *recognized_string = NULL;
	IplImage *img_tmp = NULL;

	char tmpfile[512];
	int tmpfilefd;

	cco_arraylist *list_candidate_pattern = NULL;
	cco_vSrPattern *pattern = NULL;
	float pattern_x;
	float pattern_y;
	double scale_x;
	double scale_y;


	int i;

	img_tmp = cvClone(obj->srAnalyzer_img);

	do {
		offset_x = obj->srAnalyzer_pattern_upperleft->vSrPattern_x
				+ obj->srAnalyzer_pattern_upperleft->vSrPattern_width;
		offset_y = obj->srAnalyzer_pattern_upperleft->vSrPattern_y
				+ obj->srAnalyzer_pattern_upperleft->vSrPattern_height;
		offset_width = obj->srAnalyzer_pattern_upperright->vSrPattern_x - offset_x;
		offset_height = obj->srAnalyzer_pattern_bottomleft->vSrPattern_y - offset_y;

		offset_width_excel = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, sheet->srMlSheet_blockWidth);
		offset_height_excel = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, sheet->srMlSheet_blockHeight);
		scale_x = offset_width / offset_width_excel;
		scale_y = offset_height / offset_height_excel;

		/* size of the upper-left marker */
		marker_width  = cco_srAnalyzer_get_size_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list,  -1) * scale_x;
		marker_height = cco_srAnalyzer_get_size_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, -1) * scale_y;

		list_candidate_pattern = cco_arraylist_new();
		result = cco_srAnalyzer_findBoxes(obj, obj->srAnalyzer_img, list_candidate_pattern, marker_width, marker_height);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}

		snprintf(tmpfile, sizeof(tmpfile), "%s/srocr_XXXXXX", tmpdir_prefix);
		tmpfilefd = mkstemp(tmpfile);
		if (tmpfilefd == -1)
		{
			/* LOG */
			fprintf(stderr, "ERROR: Cannot create a temporary file in cco_srAnalyzer_getIdFromImage.\n");
			break;
		}
		close(tmpfilefd);

		xml_blockOcrs = cco_vXml_getElements(sheet->srMlSheet_xml, "properties/blockOcr");
		cco_arraylist_setCursorAtFront(xml_blockOcrs);
		while ((xml = (cco_vXml *) cco_arraylist_getAtCursor(xml_blockOcrs)) != NULL)
		{
			xml_attr_name = cco_vXml_getAttribute(xml, "name");
			xml_attr_colspan = cco_vXml_getAttribute(xml, "colspan");
			xml_attr_rowspan = cco_vXml_getAttribute(xml, "rowspan");
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
			if (xml_attr_rowspan != NULL)
			{
				attr_rowspan = cco_vString_toInt(xml_attr_rowspan);
			} else {
				attr_rowspan = 1;
			}
			xml_attr_margin        = cco_vXml_getAttribute(xml, "margin");
			xml_attr_margin_top    = cco_vXml_getAttribute(xml, "margin-top");
			xml_attr_margin_bottom = cco_vXml_getAttribute(xml, "margin-bottom");
			xml_attr_margin_right  = cco_vXml_getAttribute(xml, "margin-right");
			xml_attr_margin_left   = cco_vXml_getAttribute(xml, "margin-left");
			attr_margin        = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin, 0);
			attr_margin_top    = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin_top, attr_margin);
			attr_margin_bottom = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin_bottom, attr_margin);
			attr_margin_right  = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin_right, attr_margin);
			attr_margin_left   = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin_left, attr_margin);
			/* create dir */
			tmp_string = cco_vString_newWithFormat("%@R%@/S%@/%s",
					obj->srAnalyzer_save_prefix, obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
					obj->srAnalyzer_date_string);
			tmp_cstring = tmp_string->v_getCstring(tmp_string);
			utility_mkdir(tmp_cstring);
			free(tmp_cstring);
			cco_safeRelease(tmp_string);

			recognized_string = cco_vString_new("");
			for (index_colspan = 0; index_colspan < attr_colspan; index_colspan++)
			{
				/* */
				int cell_merging_colspan = cco_srMlSheet_getCellColspan(
						sheet,
						attr_y,
						cco_srAnalyzer_getCellXnumberByColspan(sheet, attr_x, attr_y, index_colspan)
				);
				current_cell_x = cco_srAnalyzer_get_position_of_the_cell_withoutMarker_by_colspan(sheet->srMlSheet_cellWidth_list, sheet, attr_x, attr_y, index_colspan);
				current_cell_y = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, attr_y);
				current_cell_width_scaled = cco_srAnalyzer_getCurrentMergedCellWidth(sheet, attr_x, attr_y, index_colspan) * scale_x;
				if (index_colspan == 0)
				{
					current_cell_height_scaled = (
						cco_srAnalyzer_get_position_of_the_cell_withoutMarker_by_rowspan(
							sheet->srMlSheet_cellHeight_list,
							sheet, attr_x, attr_y, attr_rowspan
						) - current_cell_y
					) * scale_y;
				}

				/* discovers the box of target. */
				cco_arraylist_setCursorAtBack(list_candidate_pattern);
				int oneCharInOneCell(cco_vSrPattern *pattern)
				{
					pattern_x  = 0;
					pattern_x += current_cell_x;
					pattern_x += cco_srAnalyzer_get_size_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, attr_x + index_colspan) * 0.5;
					pattern_x *= scale_x;
					pattern_x += offset_x;
					pattern_y  = 0;
					pattern_y += current_cell_y;
					pattern_y += cco_srAnalyzer_get_size_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, attr_y) * 0.5;
					pattern_y *= scale_y;
					pattern_y += offset_y;


					/* First, check the position */
					if (pattern->vSrPattern_x < pattern_x
							&& (pattern->vSrPattern_x + pattern->vSrPattern_width) > pattern_x
							&& pattern->vSrPattern_y < pattern_y
							&& (pattern->vSrPattern_y + pattern->vSrPattern_height) > pattern_y)
					{
						if (obj->srAnalyzer_debug >= 1) {
							printf("\t%s:%s\n", __func__, "find a target within an expected area.");
						}
						/* Second, check the size of pattern. If it is small(rate is less than 85%), it will be ignored. */
						float rate_width  = 0.0;
						float rate_height = 0.0;
						if (pattern->vSrPattern_width < current_cell_width_scaled)
						{
							rate_width = pattern->vSrPattern_width / current_cell_width_scaled;
						}
						if (pattern->vSrPattern_height < current_cell_height_scaled)
						{
							rate_height = pattern->vSrPattern_height / current_cell_height_scaled;
						}
						/* if (rate_height < 0.85 || rate_width < 0.85) */
						if (rate_height < 0.5)
						{
							if (obj->srAnalyzer_debug >= 1) {
								printf("\t%s:%s\n", __func__, "find a target. But it's small. I'll ignore this pattern.");
							}
						} else {
							/* it is a target. */
							if (obj->srAnalyzer_debug >= 1) {
								printf("\t%s:%s\n", __func__, "FOUND a target.");
							}
							return 1;
						}
					}
					return 0;
				}
				int multipleCharsInMergedCell(cco_vSrPattern *pattern)
				{
					int found_flag = 0;
					float height_ratio = 0.0;
					float width_ratio = 0.0;
					float area_ratio = 0.0;

					cco_vSrPattern *current_merged_cell = cco_vSrPattern_new();
					current_merged_cell->vSrPattern_x = current_cell_x * scale_x + offset_x;
					current_merged_cell->vSrPattern_y = current_cell_y * scale_y + offset_y;
					current_merged_cell->vSrPattern_width = current_cell_width_scaled;
					current_merged_cell->vSrPattern_height = current_cell_height_scaled;

					area_ratio = (pattern->vSrPattern_width * pattern->vSrPattern_height) / (current_cell_width_scaled * current_cell_height_scaled);

					height_ratio = pattern->vSrPattern_height / current_cell_height_scaled;
					width_ratio  = pattern->vSrPattern_width / current_cell_width_scaled;

					/* First, check the position */
					if (cco_vSrPattern_isInside(current_merged_cell, pattern) && height_ratio > 0.4 && height_ratio < 1.0 && area_ratio < 0.8)
					{
						found_flag = 1;
						if (obj->srAnalyzer_debug >= 1) {
							printf("\t%s:%s %f\n", __func__, "FOUND a target.", area_ratio);
						}
					} else {
						if (obj->srAnalyzer_debug >= 1) {
							printf("\t%s:%s height ratio=%f, area_ratio=%f\n", __func__, "find a target. But it's small or area is small. I'll ignore this pattern.", height_ratio, area_ratio);
						}
					}
					cco_release(current_merged_cell);
					return found_flag;
				}

				int char_no_in_cell, loop_exit_flag = 0, first_call_flag = 1;
				for (char_no_in_cell = 0; loop_exit_flag == 0; char_no_in_cell++, first_call_flag = 0)
				{

					/* makes the path of image. */
					tmp_string = cco_vString_newWithFormat("%@R%@/S%@/%s/preOcrImg-%@-%d-%d.png",
							obj->srAnalyzer_save_prefix,
							obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
							obj->srAnalyzer_date_string, xml_attr_name,
							index_colspan, char_no_in_cell);
					tmp_cstring = tmp_string->v_getCstring(tmp_string);
					cco_safeRelease(tmp_string);
					if (obj->srAnalyzer_debug >= 1) {
						printf("*** trying to analyze x:%d, y:%d, colspan=%d, index_colspan=%d, cell_merging_colspan=%d, char_no=%d\n", attr_x, attr_y, attr_colspan, index_colspan, cell_merging_colspan, char_no_in_cell);
					}
					/*
					 * determine the way how to find the target to be passed to OCR engine
					 */
					if (cell_merging_colspan == 1)
					{
						/* try to get a target as assuming one character is written in the in the rectangle cell by using a list of contours */
						cco_srAnalyzer_getCandidateFromContourList(obj, first_call_flag, list_candidate_pattern, &oneCharInOneCell, &pattern);
						loop_exit_flag = 1;
					} else if (cell_merging_colspan > 1) {
						/* just pass the rectangle cell to the OCR engine by not using a list of contours */
						pattern == NULL;
						loop_exit_flag = 1;
					} else if (0) {	// currently disabled
						/* try to get a target from a list of contours in the rectangle cell */
						cco_srAnalyzer_getCandidateFromContourList(obj, first_call_flag, list_candidate_pattern, &multipleCharsInMergedCell, &pattern);
						if (pattern == NULL)
						{
							loop_exit_flag = 1;
						}
					}
					if (pattern != NULL)
					{
						/* found a target. */
						cco_srAnalyzer_writeImageWithPlace(obj, tmp_cstring,
								(int) (pattern->vSrPattern_x),
								(int) (pattern->vSrPattern_y),
								(int) (pattern->vSrPattern_width),
								(int) (pattern->vSrPattern_height));
						if (obj->srAnalyzer_debug >= 2) {
							cvRectangle(img_tmp,
									cvPoint((int) (pattern->vSrPattern_x),
										(int) (pattern->vSrPattern_y)),
									cvPoint((int) (pattern->vSrPattern_x + pattern->vSrPattern_width),
										(int) (pattern->vSrPattern_y + pattern->vSrPattern_height)),
									CV_RGB(0, 0, 255), 4, 8, 0);
						}
					} else if (pattern == NULL && first_call_flag == 1) {
						/* did not find a target. */
						cco_srAnalyzer_writeImageWithPlace(obj, tmp_cstring,
								(int) (current_cell_x * scale_x + offset_x + current_cell_width_scaled * attr_margin_left / 100.0),
								(int) (current_cell_y * scale_y + offset_y + current_cell_height_scaled * attr_margin_top / 100.0),
								current_cell_width_scaled - current_cell_width_scaled * (attr_margin_left + attr_margin_right) / 100.0,
								current_cell_height_scaled - current_cell_height_scaled * (attr_margin_top + attr_margin_bottom) / 100.0);
						if (obj->srAnalyzer_debug >= 2) {
							cvRectangle(img_tmp,
									cvPoint((int) (current_cell_x * scale_x + offset_x + current_cell_width_scaled * attr_margin_left / 100.0),
										(int) (current_cell_y * scale_y + offset_y + current_cell_height_scaled * attr_margin_top / 100.0)),
									cvPoint((int) (current_cell_x * scale_x + offset_x + current_cell_width_scaled - current_cell_width_scaled * attr_margin_right / 100.0),
										(int) (current_cell_y * scale_y + offset_y + current_cell_height_scaled - current_cell_width_scaled * attr_margin_bottom / 100.0)),
									CV_RGB(255, 0, 0), 4, 8, 0);
						}
						loop_exit_flag = 1;
					} else {
						break;
					}
					if (obj->srAnalyzer_debug >= 2) {
						cvCircle(img_tmp,
							cvPoint((int) (pattern_x),
								(int) (pattern_y)),
								2,
								CV_RGB(255, 0, 0), 4, 8, 0);
						cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "ocr: try to find a survey box from a list of found boxes.",
								img_tmp, 4);
					}
					cco_safeRelease(pattern);
					cco_arraylist_setCursorAtPrevious(list_candidate_pattern);
					/* creates an ocr engine. */
					ocr = obj->srAnalyzer_ocr_obj;
					ocr->srOcr_setImage(ocr, tmp_cstring);
					if (xml_attr_option != NULL)
					{
						attr_option_cstring = xml_attr_option->v_getCstring(xml_attr_option);
						ocr->srOcr_setOption(ocr, attr_option_cstring);
						free(attr_option_cstring);
					}
					ocr->srOcr_getRecognizeString(ocr, &tmp_string);
					cco_vString_catenate(recognized_string, tmp_string);
					free(tmp_cstring);
					cco_release(tmp_string);

					/*
					 *cvRectangle(img_tmp,
					 *                cvPoint((int) (cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, attr_x) * scale_x + offset_x),
					 *                        (int) (cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, attr_y) * scale_y + offset_y)),
					 *                cvPoint((int) (cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, attr_x) * scale_x + offset_x + current_cell_width_scaled),
					 *                        (int) (cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, attr_y) * scale_y + offset_y + current_cell_height_scaled)),
					 *                CV_RGB(255, 0, 0), 4, 8, 0);
					 */
				}
			}
			/* insert result of OCRed value */
			valuekey = cco_vString_new("blockOcr");
			valuestree = (cco_redblacktree *)cco_redblacktree_get(keyval, (cco_v*) xml_attr_name);
			if (valuestree == NULL) {
				valuestree = cco_redblacktree_new();
				cco_redblacktree_insert(keyval, (cco_v*) xml_attr_name, (cco*) valuestree);
			}
			cco_redblacktree_insert(valuestree, (cco_v*) valuekey, (cco*) recognized_string);
			cco_safeRelease(valuestree);
			cco_safeRelease(valuekey);
			if (obj->srAnalyzer_debug >= 1)
			{
				tmp_string = cco_vString_newWithFormat("name:%@ x:%d y:%d colspan:%d recognizedText:%@\n",
						xml_attr_name, attr_x, attr_y, attr_colspan, recognized_string);
				tmp_cstring = tmp_string->v_getCstring(tmp_string);
				printf("%s", tmp_cstring);
				free(tmp_cstring);
				cco_safeRelease(tmp_string);
			}
			/* pre-ocr image file hangling */
			do {
				cco_vString *preOcrImgFileNamePrefix = NULL;

				if (index_colspan >= 2)
				{	// concatenate the multiple image files to one image file for the cell having colspan
					cco_vString *mergedPreOcrImgFileName = NULL;
					cco_vString *allPreOcrImgFiles = NULL;
					mergedPreOcrImgFileName = cco_vString_newWithFormat("%@R%@/S%@/%s/preOcrImg-%@-concat.png",
							obj->srAnalyzer_save_prefix,
							obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
							obj->srAnalyzer_date_string, xml_attr_name);
					allPreOcrImgFiles = cco_vString_newWithFormat("%@R%@/S%@/%s/preOcrImg-%@-*.png",
							obj->srAnalyzer_save_prefix,
							obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
							obj->srAnalyzer_date_string, xml_attr_name);
					cco_srAnalyzer_concat_preOcrImageFiles(allPreOcrImgFiles, mergedPreOcrImgFileName);
					cco_release(allPreOcrImgFiles);
					cco_release(mergedPreOcrImgFileName);

					preOcrImgFileNamePrefix = cco_vString_newWithFormat("R%@/S%@/%s/preOcrImg-%@-concat.png",
							obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
							obj->srAnalyzer_date_string, xml_attr_name);
				} else {
					preOcrImgFileNamePrefix = cco_vString_newWithFormat("R%@/S%@/%s/preOcrImg-%@-%d.png",
							obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
							obj->srAnalyzer_date_string, xml_attr_name,
							index_colspan - 1);
				}

				/* save the path information of pre ocr image file */
				valuekey = cco_vString_new("blockPreOcr");
				valuestree = (cco_redblacktree *)cco_redblacktree_get(keyval, (cco_v*) xml_attr_name);
				if (valuestree == NULL) {
					valuestree = cco_redblacktree_new();
					cco_redblacktree_insert(keyval, (cco_v*) xml_attr_name, (cco*) valuestree);
				}
				cco_redblacktree_insert(valuestree, (cco_v*) valuekey, (cco*) preOcrImgFileNamePrefix);
				cco_safeRelease(valuestree);
				cco_safeRelease(valuekey);
				cco_release(preOcrImgFileNamePrefix);
			} while(0);

			cco_safeRelease(recognized_string);
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
	double current_cell_width_scaled;
	double current_cell_height_scaled;
	double current_cell_position_x;
	double current_cell_position_y;
	double current_cell_position_plus_colspan_x;
	double current_cell_position_plus_colspan_y;
	int offset_x;
	int offset_y;
	int offset_width;
	int offset_height;
	double offset_width_excel;
	double offset_height_excel;
	double scale_x;
	double scale_y;

	cco_vXml *xml = NULL;
	cco_arraylist *xml_blockOcrs = NULL;
	cco_vString *xml_attr_name = NULL;
	cco_vString *xml_attr_colspan = NULL;
	cco_vString *xml_attr_rowspan = NULL;
	cco_vString *xml_attr_x = NULL;
	cco_vString *xml_attr_y = NULL;
	cco_vString *xml_attr_margin = NULL;
	cco_vString *xml_attr_margin_top = NULL;
	cco_vString *xml_attr_margin_bottom = NULL;
	cco_vString *xml_attr_margin_right = NULL;
	cco_vString *xml_attr_margin_left = NULL;
	cco_vString *tmp_string;
	cco_vString *valuekey;
	cco_redblacktree *valuestree;
	char *tmp_cstring;
	int attr_colspan;
	int attr_rowspan;
	int attr_x;
	int attr_y;
	int attr_margin;
	int attr_margin_top;
	int attr_margin_bottom;
	int attr_margin_right;
	int attr_margin_left;

	offset_x = obj->srAnalyzer_pattern_upperleft->vSrPattern_x
			+ obj->srAnalyzer_pattern_upperleft->vSrPattern_width;
	offset_y = obj->srAnalyzer_pattern_upperleft->vSrPattern_y
			+ obj->srAnalyzer_pattern_upperleft->vSrPattern_height;
	offset_width = obj->srAnalyzer_pattern_upperright->vSrPattern_x - offset_x;
	offset_height = obj->srAnalyzer_pattern_bottomleft->vSrPattern_y - offset_y;
	offset_width_excel = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, sheet->srMlSheet_blockWidth);
	offset_height_excel = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, sheet->srMlSheet_blockHeight);
	scale_x = offset_width / offset_width_excel;
	scale_y = offset_height / offset_height_excel;

	xml_blockOcrs = cco_vXml_getElements(sheet->srMlSheet_xml, "properties/blockImg");
	cco_arraylist_setCursorAtFront(xml_blockOcrs);
	while ((xml = (cco_vXml *) cco_arraylist_getAtCursor(xml_blockOcrs)) != NULL)
	{
		xml_attr_name = cco_vXml_getAttribute(xml, "name");
		xml_attr_colspan = cco_vXml_getAttribute(xml, "colspan");
		xml_attr_rowspan = cco_vXml_getAttribute(xml, "rowspan");
		xml_attr_x = cco_vXml_getAttribute(xml, "x");
		xml_attr_y = cco_vXml_getAttribute(xml, "y");
		xml_attr_margin = cco_vXml_getAttribute(xml, "margin");
		xml_attr_margin_top = cco_vXml_getAttribute(xml, "margin-top");
		xml_attr_margin_bottom = cco_vXml_getAttribute(xml, "margin-bottom");
		xml_attr_margin_right = cco_vXml_getAttribute(xml, "margin-right");
		xml_attr_margin_left = cco_vXml_getAttribute(xml, "margin-left");

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
		if (xml_attr_rowspan != NULL)
		{
			attr_rowspan = cco_vString_toInt(xml_attr_rowspan);
		} else {
			attr_rowspan = 1;
		}
		attr_margin        = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin, 0);
		attr_margin_top    = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin_top, attr_margin);
		attr_margin_bottom = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin_bottom, attr_margin);
		attr_margin_right  = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin_right, attr_margin);
		attr_margin_left   = cco_srAnalyzer_get_margin_from_xml_attribute(xml_attr_margin_left, attr_margin);
		tmp_string = cco_vString_newWithFormat("%@R%@/S%@/%s",
				obj->srAnalyzer_save_prefix, obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
				obj->srAnalyzer_date_string);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		utility_mkdir(tmp_cstring);
		free(tmp_cstring);
		cco_safeRelease(tmp_string);

		current_cell_position_x = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, attr_x) * scale_x + offset_x;
		current_cell_position_y = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, attr_y) * scale_y + offset_y;
		current_cell_position_plus_colspan_x =
			cco_srAnalyzer_get_position_of_the_cell_withoutMarker_by_colspan(
				sheet->srMlSheet_cellWidth_list,
				sheet, attr_x, attr_y, attr_colspan
			) * scale_x + offset_x;
		current_cell_position_plus_colspan_y =
			cco_srAnalyzer_get_position_of_the_cell_withoutMarker_by_rowspan(
				sheet->srMlSheet_cellHeight_list,
				sheet, attr_x, attr_y, attr_rowspan
			) * scale_y + offset_y;
		current_cell_width_scaled  = current_cell_position_plus_colspan_x - current_cell_position_x;
		current_cell_height_scaled = current_cell_position_plus_colspan_y - current_cell_position_y;

		tmp_string = cco_vString_newWithFormat("%@R%@/S%@/%s/blockImg-%@.png",
				obj->srAnalyzer_save_prefix, obj->srAnalyzer_sender, obj->srAnalyzer_receiver,
				obj->srAnalyzer_date_string, xml_attr_name);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		cco_srAnalyzer_writeImageWithPlace(obj, tmp_cstring,
				(int) (current_cell_position_x + current_cell_width_scaled * attr_margin_left / 100.0),
				(int) (current_cell_position_y + current_cell_height_scaled * attr_margin_top / 100.0),
				current_cell_width_scaled - current_cell_width_scaled * (attr_margin_left + attr_margin_right) / 100.0,
				current_cell_height_scaled - current_cell_height_scaled * (attr_margin_top + attr_margin_bottom) / 100.0);
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

/*
 * View a grid by using cell width and height info in SrML
 */
CCOSRANALYZER_STATUS cco_srAnalyzer_viewGrid(cco_srAnalyzer *obj, cco_srMlSheet *sheet)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	double current_cell_position_x;
	double current_cell_position_y;
	double start_cell_position_x;
	double start_cell_position_y;
	int offset_x;
	int offset_y;
	int offset_width;
	int offset_height;
	int col, row;
	double offset_width_excel;
	double offset_height_excel;
	double scale_x;
	double scale_y;
	IplImage *grid_img = NULL;

	offset_x = obj->srAnalyzer_pattern_upperleft->vSrPattern_x
			+ obj->srAnalyzer_pattern_upperleft->vSrPattern_width;
	offset_y = obj->srAnalyzer_pattern_upperleft->vSrPattern_y
			+ obj->srAnalyzer_pattern_upperleft->vSrPattern_height;
	offset_width = obj->srAnalyzer_pattern_upperright->vSrPattern_x - offset_x;
	offset_height = obj->srAnalyzer_pattern_bottomleft->vSrPattern_y - offset_y;
	offset_width_excel = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, sheet->srMlSheet_blockWidth);
	offset_height_excel = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, sheet->srMlSheet_blockHeight);
	scale_x = offset_width / offset_width_excel;
	scale_y = offset_height / offset_height_excel;

	grid_img = cvClone(obj->srAnalyzer_img);

	start_cell_position_x = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, 0) * scale_x + offset_x;
	start_cell_position_y = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, 0) * scale_y + offset_y;
	// draw horizontal lines
	current_cell_position_x = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, sheet->srMlSheet_blockWidth) * scale_x + offset_x;
	for (row = 0; row < sheet->srMlSheet_blockHeight + 1; row++)
	{
		current_cell_position_y = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, row) * scale_y + offset_y;
		cvLine(grid_img, cvPoint(start_cell_position_x, current_cell_position_y), cvPoint(current_cell_position_x, current_cell_position_y), CV_RGB(0, 255, 0), 2, 8, 0);
	}
	// draw vertical lines
	current_cell_position_y = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellHeight_list, sheet->srMlSheet_blockHeight) * scale_y + offset_y;
	for (col = 0; col < sheet->srMlSheet_blockWidth + 1; col++)
	{
		current_cell_position_x = cco_srAnalyzer_get_position_of_the_cell_withoutMarker(sheet->srMlSheet_cellWidth_list, col) * scale_x + offset_x;
		cvLine(grid_img, cvPoint(current_cell_position_x, start_cell_position_y), cvPoint(current_cell_position_x, current_cell_position_y), CV_RGB(0, 255, 0), 2, 8, 0);
	}
	cco_srAnalyzer_showShrinkedImageNow_withImage(obj, "Grid by using CellWidth and CellHeight in SrML", grid_img, 2);

	cvReleaseImage(&grid_img);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ocrToGetIdsFromImage(cco_srAnalyzer *obj, char *ocr_db, int force_set_uid, int force_set_sid)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_vString *userid = NULL;
	cco_vString *sheetid = NULL;

	do
	{
		cco_srAnalyzer_getOcrbIdFromImage(obj, obj->srAnalyzer_pattern_upperleft,
				obj->srAnalyzer_pattern_bottomleft, obj->srAnalyzer_pattern_upperright,
				&userid, &sheetid, ocr_db);
		if (force_set_uid)
		{
			cco_safeRelease(obj->srAnalyzer_uid);
			obj->srAnalyzer_uid = cco_get(userid);
		}
		if (force_set_sid)
		{
			cco_safeRelease(obj->srAnalyzer_sid);
			obj->srAnalyzer_sid = cco_get(sheetid);
		}
	} while (0);
	cco_safeRelease(userid);
	cco_safeRelease(sheetid);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ocrToGetSheetAndSurveyIds(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	int is_uid_specified_in_cmd_line = 0;
	int is_sid_specified_in_cmd_line = 0;

	do
	{
		if (cco_vString_length(obj->srAnalyzer_uid) != 0)
		{
			is_uid_specified_in_cmd_line = 1;
		}
		if (cco_vString_length(obj->srAnalyzer_sid) != 0)
		{
			is_sid_specified_in_cmd_line = 1;
		}
#ifdef KOCR
		/* try ocr by using kocr */
		cco_srAnalyzer_setOcrEngine(obj, "kocr");
		cco_srAnalyzer_ocrToGetIdsFromImage(obj, "idocrbs", !is_uid_specified_in_cmd_line, !is_sid_specified_in_cmd_line);
		if (cco_vString_length(obj->srAnalyzer_sid) == 5)
		{
			break;
		}
#endif
		/* try ocr by using gocr */
		cco_srAnalyzer_setOcrEngine(obj, "gocr");
		cco_srAnalyzer_ocrToGetIdsFromImage(obj, "number", !is_uid_specified_in_cmd_line, !is_sid_specified_in_cmd_line);
		if (cco_vString_length(obj->srAnalyzer_sid) == 5)
		{
			break;
		}
#ifdef KOCR
		cco_srAnalyzer_setOcrEngine(obj, "kocr");
#endif
	} while (0);
	return result;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_ocr(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_srMlSheet *sheet = NULL;

	do
	{
		/* Find pattern to set informations to this object. */
		result = cco_srAnalyzer_examineImageToFindPattern(obj);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}
		/* get a sheet ID and a survey ID */
		result = cco_srAnalyzer_ocrToGetSheetAndSurveyIds(obj);
		if (result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}
		/* OCR */
		sheet = cco_srMl_getSheet(obj->srAnalyzer_srml, obj->srAnalyzer_sid);
		if (sheet == NULL)
		{
			fprintf(stderr, "ERROR: CCOSRANALYZER_STATUS_NOT_FOUND_SRML\n");
			result = CCOSRANALYZER_STATUS_NOT_FOUND_SRML;
			break;
		}
		if (obj->srAnalyzer_debug >= 2)
		{
			cco_srAnalyzer_viewGrid(obj, sheet);
		}
		cco_srAnalyzer_ocrProcBlockOcr(obj, sheet, obj->srAnalyzer_analyzedData);
		cco_srAnalyzer_ocrProcBlockImg(obj, sheet, obj->srAnalyzer_analyzedData);
	} while (0);
	cco_safeRelease(sheet);
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
	cco_vString *preOcrImage_string;
	cco_vString *blockImage_string;

	values = (cco_redblacktree *)cco_get(object);
	ocrName_string = (cco_vString *)cco_get(key);
	tmp_string = cco_vString_new("blockOcr");
	ocrValue_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (ocrValue_string == NULL)
	{
		ocrValue_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);
	tmp_string = cco_vString_new("blockPreOcr");
	preOcrImage_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (preOcrImage_string == NULL)
	{
		preOcrImage_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);
	tmp_string = cco_vString_new("blockImg");
	blockImage_string = (cco_vString *)cco_redblacktree_get(values, (cco_v*)tmp_string);
	if (blockImage_string == NULL)
	{
		blockImage_string = cco_vString_new("");
	}
	cco_safeRelease(tmp_string);
	tmp_string = cco_vString_newWithFormat(
			"  <value>\n"
			"   <ocrName>%@</ocrName>\n"
			"   <ocrValue><![CDATA[%@]]></ocrValue>\n"
			"   <ocrImg><![CDATA[%@]]></ocrImg>\n"
			"   <preOcrImg><![CDATA[%@]]></preOcrImg>\n"
			"   <humanCheck>no</humanCheck>\n"
			"  </value>\n", ocrName_string, ocrValue_string, blockImage_string, preOcrImage_string);
	cco_vString_catenate((cco_vString *)((cco_srAnalyzer *)callbackobject)->srAnalyzer_outXml, tmp_string);
	cco_safeRelease(tmp_string);
	cco_safeRelease(values);
	cco_safeRelease(ocrName_string);
	cco_safeRelease(ocrValue_string);
	cco_safeRelease(preOcrImage_string);
	cco_safeRelease(blockImage_string);
	return;
}

CCOSRANALYZER_STATUS cco_srAnalyzer_outXml(cco_srAnalyzer *obj)
{
	CCOSRANALYZER_STATUS result = CCOSRANALYZER_STATUS_SUCCESS;
	char *tmp_cstring;

	cco_safeRelease(obj->srAnalyzer_outXml);
	obj->srAnalyzer_outXml = cco_vString_new("<ocrResult>\n");
	cco_vString_catenateWithFormat(obj->srAnalyzer_outXml, " <sheetId>");
	cco_vString_catenate(obj->srAnalyzer_outXml, obj->srAnalyzer_sid);
	cco_vString_catenateWithFormat(obj->srAnalyzer_outXml, "</sheetId>\n");
	cco_vString_catenateWithFormat(obj->srAnalyzer_outXml, " <surveyTargetId>");
	cco_vString_catenate(obj->srAnalyzer_outXml, obj->srAnalyzer_uid);
	cco_vString_catenateWithFormat(obj->srAnalyzer_outXml, "</surveyTargetId>\n");
	cco_vString_catenateWithFormat(obj->srAnalyzer_outXml, " <valueList>\n");
	cco_redblacktree_traversePreorder(obj->srAnalyzer_analyzedData, (cco *)obj, cco_srAnalyzer_outXml_sub);
	cco_vString_catenateWithFormat(obj->srAnalyzer_outXml, " </valueList>\n");
	cco_vString_catenateWithFormat(obj->srAnalyzer_outXml, "</ocrResult>\n");
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
	} while (0);
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
