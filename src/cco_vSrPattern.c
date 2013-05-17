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

#include "cco_vSrPattern.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cluscore/cco_vString.h>

cco_defineClass(cco_vSrPattern);

cco_vSrPattern *cco_vSrPattern_baseNew(int size)
{
	cco_vSrPattern *o = NULL;
	do {
		if (size < sizeof(cco_vSrPattern))
		{
			break;
		}
		o = (cco_vSrPattern *)cco_v_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_vSrPattern);
		cco_vSrPattern_baseInitialize(o);
	} while (0);
	return o;
}

void cco_vSrPattern_baseRelease(void *o)
{
	cco_vSrPattern_baseFinalize(o);
	cco_v_baseRelease(o);
}

void cco_vSrPattern_baseInitialize(cco_vSrPattern *obj)
{
	obj->baseRelease = &cco_vSrPattern_baseRelease;
	obj->vSrPattern_x = 0.0;
	obj->vSrPattern_y = 0.0;
	obj->vSrPattern_width = 0.0;
	obj->vSrPattern_height = 0.0;
	obj->v_getCstring = cco_vSrPattern_getCstring;
	obj->v_compere = cco_vSrPattern_compere;
	obj->v_hash = cco_vSrPattern_hash;

	return;
}

void cco_vSrPattern_baseFinalize(cco_vSrPattern *o)
{
	return;
}

cco_vSrPattern *cco_vSrPattern_new()
{
	return cco_vSrPattern_baseNew(sizeof(cco_vSrPattern));
}

void cco_vSrPattern_release(void *o)
{
	cco_release(o);
}

int cco_vSrPattern_compere(void *cco_v1, void *cco_v2)
{
	int result = 0;
	cco_vSrPattern *pattern1;
	cco_vSrPattern *pattern2;
	float len1;
	float len2;

	pattern1 = (cco_vSrPattern *)cco_v1;
	pattern2 = (cco_vSrPattern *)cco_v2;
	len1 = pattern1->vSrPattern_width + pattern1->vSrPattern_height;
	len2 = pattern2->vSrPattern_width + pattern2->vSrPattern_height;
	if (len1 == len2)
	{
		result = 0;
	} else if (len1 > len2) {
		result = 1;
	} else {
		result = -1;
	}
	return result;
}

int cco_vSrPattern_hash(void *cco_v, int salt)
{
	return 0;
}

char *cco_vSrPattern_getCstring(void *cco_v)
{
	cco_vString *outstring;
	cco_vSrPattern *pattern;
	char *result = NULL;
	if (cco_v != NULL)
	{
		pattern = (cco_vSrPattern *)cco_v;
		outstring = cco_vString_newWithFormat("point:(%d,%d) size(%d,%d)",
				(int)pattern->vSrPattern_x, (int)pattern->vSrPattern_y,
				(int)pattern->vSrPattern_width, (int)pattern->vSrPattern_height);
		result = outstring->v_getCstring(outstring);
	}
	return result;
}

int cco_vSrPattern_set(cco_vSrPattern *obj, float x, float y, float width, float height)
{
	int result = -1;
	if (obj != NULL)
	{
		obj->vSrPattern_x = x;
		obj->vSrPattern_y = y;
		obj->vSrPattern_width = width;
		obj->vSrPattern_height = height;
		result = 0;
	}
	return result;
}

int cco_vSrPattern_setInInt(cco_vSrPattern *obj, int x, int y, int width, int height)
{
	int result = -1;
	if (obj != NULL)
	{
		obj->vSrPattern_x = (float)x;
		obj->vSrPattern_y = (float)y;
		obj->vSrPattern_width = (float)width;
		obj->vSrPattern_height = (float)height;
		result = 0;
	}
	return result;
}

int cco_vSrPattern_isInside(cco_vSrPattern *obj, cco_vSrPattern *inside_obj)
{
	int result = 0;

	if (obj->vSrPattern_x < inside_obj->vSrPattern_x
			&& (obj->vSrPattern_x + obj->vSrPattern_width) > (inside_obj->vSrPattern_x + inside_obj->vSrPattern_width)
			&& obj->vSrPattern_y < inside_obj->vSrPattern_y
			&& (obj->vSrPattern_y + obj->vSrPattern_height) > (inside_obj->vSrPattern_y + inside_obj->vSrPattern_height)) {
		result = 1;
	}

	return result;
}

int cco_vSrPattern_isSquare(cco_vSrPattern *obj)
{
	float rate;
	int result = 0;

	if (obj->vSrPattern_width > obj->vSrPattern_height)
	{
		rate = (float)obj->vSrPattern_height / (float)obj->vSrPattern_width;
	} else {
		rate = (float)obj->vSrPattern_width / (float)obj->vSrPattern_height;
	}
	if (rate > 0.8) {
		result = 1;
	}

	return result;
}

int cco_vSrPattern_isRectangle1to2(cco_vSrPattern *obj)
{
	float rate;
	int result = 0;

	if (obj->vSrPattern_width > obj->vSrPattern_height)
	{
		rate = (float)obj->vSrPattern_height / (float)obj->vSrPattern_width;
	} else {
		rate = (float)obj->vSrPattern_width / (float)obj->vSrPattern_height;
	}
	if (rate < 0.65 && rate > 0.35) {
		result = 1;
	}

	return result;
}

int cco_vSrPattern_isPattern(cco_vSrPattern *obj, cco_arraylist *insideboxes)
{
	int result;
    int len_parent;
    int len_parent_c1;
    int len_parent_c2;
    int len_child1;
    int len_child2;
    int len_tmp;
    float rate_child1;
    float rate_child2;
    cco_vSrPattern *candidate_pattern = NULL;

	result = 0;
    len_parent = obj->vSrPattern_width + obj->vSrPattern_height;
    len_parent_c1 = len_parent * (float)((float)2.0 / (float)3.0);
    len_parent_c2 = len_parent * (float)((float)1.0 / (float)3.0);

    candidate_pattern = (cco_vSrPattern *)cco_arraylist_getAt(insideboxes, 0);
    len_child1 = candidate_pattern->vSrPattern_width + candidate_pattern->vSrPattern_height;
    cco_release(candidate_pattern);
    candidate_pattern = (cco_vSrPattern *)cco_arraylist_getAt(insideboxes, 1);
    len_child2 = candidate_pattern->vSrPattern_width + candidate_pattern->vSrPattern_height;
    cco_release(candidate_pattern);

	if (len_child1 < len_child2) {
		len_tmp = len_child1;
		len_child1 = len_child2;
		len_child2 = len_tmp;
	}
	if (len_child1 > len_parent_c1) {
		rate_child1 = (float)len_parent_c1 / (float)len_child1;
	} else {
		rate_child1 = (float)len_child1 / (float)len_parent_c1;
	}
	if (len_child2 > len_parent_c2) {
		rate_child2 = (float)len_parent_c2 / (float)len_child2;
	} else {
		rate_child2 = (float)len_child2 / (float)len_parent_c2;
	}
	/* TODO: Examines proportion. */
	if (rate_child1 > 0.8 && rate_child2 > 0.8) {
		result = 1;
	}
	return result;
}
