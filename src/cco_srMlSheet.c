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
	o->srMlSheet_cellWidth_list = cco_arraylist_new();
	o->srMlSheet_cellHeight_list = cco_arraylist_new();
	o->srMlSheet_cellRowspan = NULL;
	o->srMlSheet_cellColspan = NULL;
	return;
}

void cco_srMlSheet_baseFinalize(cco_srMlSheet *o)
{
	cco_safeRelease(o->srMlSheet_xml);
	cco_safeRelease(o->srMlSheet_xml);
	cco_arraylist_release(o->srMlSheet_cellWidth_list);
	cco_arraylist_release(o->srMlSheet_cellHeight_list);
	free(o->srMlSheet_cellRowspan);
	free(o->srMlSheet_cellColspan);
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

int cco_srMlSheet_setCellWidth(cco_srMlSheet *obj, cco_vString *str, int index)
{
	cco_arraylist_addDynamicAt(obj->srMlSheet_cellWidth_list, str, index);
	return 0;
}

int cco_srMlSheet_setCellHeight(cco_srMlSheet *obj, cco_vString *str, int index)
{
	cco_arraylist_addDynamicAt(obj->srMlSheet_cellHeight_list, str, index);
	return 0;
}

int cco_srMlSheet_setCellRowspan(cco_srMlSheet *obj, int *rowspans)
{
	obj->srMlSheet_cellRowspan = rowspans;
	return 0;
}

int cco_srMlSheet_setCellColspan(cco_srMlSheet *obj, int *colspans)
{
	obj->srMlSheet_cellColspan = colspans;
	return 0;
}
