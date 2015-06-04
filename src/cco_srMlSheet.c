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

#include "cco_srMlSheet.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cco_defineClass(cco_srMlSheet);

cco_srMlSheet *cco_srMlSheet_baseNew(int size)
{
	cco_srMlSheet *o = NULL;
	do {
		if (size < sizeof(cco_srMlSheet))
		{
			break;
		}
		o = (cco_srMlSheet *)cco_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srMlSheet);
		cco_srMlSheet_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srMlSheet_baseRelease(void *o)
{
	cco_srMlSheet_baseFinalize(o);
	cco_baseRelease(o);
}

void cco_srMlSheet_baseInitialize(cco_srMlSheet *o)
{
	o->baseRelease = &cco_srMlSheet_baseRelease;
	o->srMlSheet_xml = NULL;
	o->srMlSheet_id = NULL;
	o->srMlSheet_blockWidth = 0;
	o->srMlSheet_blockHeight = 0;
	o->srMlSheet_cellWidth_list = cco_redblacktree_new();
	o->srMlSheet_cellHeight_list = cco_redblacktree_new();
	o->srMlSheet_cellRowspan = cco_redblacktree_new();
	o->srMlSheet_cellColspan = cco_redblacktree_new();
	return;
}

void cco_srMlSheet_baseFinalize(cco_srMlSheet *o)
{
	cco_safeRelease(o->srMlSheet_xml);
	cco_safeRelease(o->srMlSheet_xml);
	cco_redblacktree_release(o->srMlSheet_cellWidth_list);
	cco_redblacktree_release(o->srMlSheet_cellHeight_list);
	cco_redblacktree_release(o->srMlSheet_cellRowspan);
	cco_redblacktree_release(o->srMlSheet_cellColspan);
	return;
}

cco_srMlSheet *cco_srMlSheet_new()
{
	return cco_srMlSheet_baseNew(sizeof(cco_srMlSheet));
}

void cco_srMlSheet_release(void *o)
{
	cco_release(o);
}

int cco_srMlSheet_setXml(cco_srMlSheet *obj, cco_vXml *xml)
{
	cco_safeRelease(obj->srMlSheet_xml);
	obj->srMlSheet_xml = cco_get(xml);
	return 0;
}

int cco_srMlSheet_setId(cco_srMlSheet *obj, cco_vString *str)
{
	cco_safeRelease(obj->srMlSheet_id);
	obj->srMlSheet_id = cco_get(str);
	return 0;
}

int cco_srMlSheet_setWidth(cco_srMlSheet *obj, cco_vString *str)
{
	if (str != NULL)
	{
		obj->srMlSheet_blockWidth = cco_vString_toInt(str);
	} else {
		obj->srMlSheet_blockWidth = 0;
	}
	return 0;
}

int cco_srMlSheet_setHeight(cco_srMlSheet *obj, cco_vString *str)
{
	if (str != NULL)
	{
		obj->srMlSheet_blockHeight = cco_vString_toInt(str);
	} else {
		obj->srMlSheet_blockHeight = 0;
	}
	return 0;
}

int cco_srMlSheet_setCellWidth(cco_srMlSheet *obj, cco_vString *index, cco_vString *str)
{
	cco_redblacktree_status status;

	status = cco_redblacktree_insert(obj->srMlSheet_cellWidth_list, (cco_v *)index, (cco *)str);
	if (status != CCO_REDBLACKTREE_STATUS_INSERTED)
	{
		fprintf(stderr, "%s:the key(%s) already exists\n", __func__, cco_vString_getCstring(index));
	}
	return 0;
}

int cco_srMlSheet_setCellHeight(cco_srMlSheet *obj, cco_vString *index, cco_vString *str)
{
	cco_redblacktree_status status;

	status = cco_redblacktree_insert(obj->srMlSheet_cellHeight_list, (cco_v *)index, (cco *)str);
	if (status != CCO_REDBLACKTREE_STATUS_INSERTED)
	{
		fprintf(stderr, "%s:the key(%s) already exists\n", __func__, cco_vString_getCstring(index));
	}
	return 0;
}

int cco_srMlSheet_setCellRowspan(cco_srMlSheet *obj, cco_vString *row_num, cco_vString *col_num, cco_vString *rowspan)
{
	cco_redblacktree_status status;
	cco_vString *row_col_str = cco_vString_newWithFormat("%@/%@", row_num, col_num);

	status = cco_redblacktree_insert(obj->srMlSheet_cellRowspan, (cco_v *)row_col_str, (cco *)rowspan);
	if (status != CCO_REDBLACKTREE_STATUS_INSERTED)
	{
		fprintf(stderr, "%s:the key(%s) already exists\n", __func__, cco_vString_getCstring(row_col_str));
	}
	cco_safeRelease(row_col_str);
	return 0;
}

int cco_srMlSheet_setCellColspan(cco_srMlSheet *obj, cco_vString *row_num, cco_vString *col_num, cco_vString *colspan)
{
	cco_redblacktree_status status;
	cco_vString *row_col_str = cco_vString_newWithFormat("%@/%@", row_num, col_num);

	status = cco_redblacktree_insert(obj->srMlSheet_cellColspan, (cco_v *)row_col_str, (cco *)colspan);
	if (status != CCO_REDBLACKTREE_STATUS_INSERTED)
	{
		fprintf(stderr, "%s:the key(%s) already exists\n", __func__, cco_vString_getCstring(row_col_str));
	}
	cco_safeRelease(row_col_str);
	return 0;
}

double cco_srMlSheet_getCellWidthOrHeight(cco_redblacktree *obj, int index)
{
	cco_vString *index_str = cco_vString_newWithFormat("%d", index);
	cco_vString *result_str;
	char *cstring;
	double size;

	result_str = (cco_vString *)cco_redblacktree_get(obj, (cco_v *)index_str);
	cco_safeRelease(index_str);
	if (result_str == NULL)
	{
		return 1;
	} else {
		cstring = cco_vString_getCstring(result_str);
		size = atof(cstring);
		free(cstring);
		return size;
	}
}

int cco_srMlSheet_getCellRowspan(cco_srMlSheet *obj, int row_num, int col_num)
{
	cco_vString *row_col_str = cco_vString_newWithFormat("%d/%d", row_num, col_num);
	cco_vString *result_str;

	result_str = (cco_vString *)cco_redblacktree_get(obj->srMlSheet_cellRowspan, (cco_v *)row_col_str);
	cco_safeRelease(row_col_str);
	if (result_str == NULL)
	{
		return 1;
	} else {
		return cco_vString_toInt(result_str);
	}
}

int cco_srMlSheet_getCellColspan(cco_srMlSheet *obj, int row_num, int col_num)
{
	cco_vString *row_col_str = cco_vString_newWithFormat("%d/%d", row_num, col_num);
	cco_vString *result_str;

	result_str = (cco_vString *)cco_redblacktree_get(obj->srMlSheet_cellColspan, (cco_v *)row_col_str);
	cco_safeRelease(row_col_str);
	if (result_str == NULL)
	{
		return 1;
	} else {
		return cco_vString_toInt(result_str);
	}
}
