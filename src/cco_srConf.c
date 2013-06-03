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

#include "cco_srConf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cco_defineClass(cco_srConf);

cco_srConf *cco_srConf_baseNew(int size)
{
	cco_srConf *o = NULL;
	do {
		if (size < sizeof(cco_srConf))
		{
			break;
		}
		o = (cco_srConf *)cco_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srConf);
		cco_srConf_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srConf_baseRelease(void *o)
{
	cco_srConf_baseFinalize(o);
	cco_baseRelease(o);
}

void cco_srConf_baseInitialize(cco_srConf *o)
{
	o->baseRelease = &cco_srConf_baseRelease;

	o->srConf_xml = cco_vXml_new();
	o->srConf_outCode = cco_vString_new("");
	o->srConf_outPropertyCode = cco_vString_new("");

	return;
}

void cco_srConf_baseFinalize(cco_srConf *o)
{
	cco_release(o->srConf_xml);
	cco_release(o->srConf_outCode);
	cco_release(o->srConf_outPropertyCode);
	return;
}

cco_srConf *cco_srConf_new()
{
	return cco_srConf_baseNew(sizeof(cco_srConf));
}

void cco_srConf_release(void *o)
{
	cco_release(o);
}

CCOSRCONF_STATUS cco_srConf_examineXml(cco_srConf *xml)
{
	CCOSRCONF_STATUS result = CCOSRCONF_STATUS_SUCCESS;
	cco_vXml *headxml;

	do {
		cco_release(xml->srConf_outCode);
		cco_release(xml->srConf_outPropertyCode);
		headxml = cco_vXml_getElementAtFront(xml->srConf_xml, "sheetreader");
		if (headxml == NULL)
		{
			result = CCOSRCONF_STATUS_NOTREADEDXML;
			break;
		}
		cco_safeRelease(headxml);
		headxml = cco_vXml_getElementAtFront(xml->srConf_xml, "sheetreader/outFormat/rails/code");
		if (headxml != NULL)
		{
			xml->srConf_outCode = cco_vXml_getContent(headxml);
		}
		cco_safeRelease(headxml);
		headxml = cco_vXml_getElementAtFront(xml->srConf_xml, "sheetreader/outFormat/rails/propertyCode");
		if (headxml != NULL)
		{
			xml->srConf_outPropertyCode = cco_vXml_getContent(headxml);
		}
		cco_safeRelease(headxml);
	} while (0);
	cco_release(headxml);
	return result;
}

CCOSRCONF_STATUS cco_srConf_read(cco_srConf *obj, char *filename)
{
	CCOSRCONF_STATUS result = CCOSRCONF_STATUS_SUCCESS;

	do
	{
		if (obj == NULL)
		{
			result = CCOSRCONF_STATUS_ERROR;
			break;
		}
		cco_release(obj->srConf_xml);
		obj->srConf_xml = cco_vXml_new();
		cco_vXml_read(obj->srConf_xml, filename);
		cco_srConf_examineXml(obj);
	} while (0);
	return result;
}

CCOSRCONF_STATUS cco_srConf_setCurrentMode(cco_srConf *xml, char *mode)
{
	CCOSRCONF_STATUS result = CCOSRCONF_STATUS_SUCCESS;
	cco_vXml *codexml = NULL;
	cco_vXml *propxml = NULL;
	char pathbuff[1024];

	do {
		snprintf(pathbuff, sizeof(pathbuff), "sheetreader/outFormat/%s/code", mode);
		codexml = cco_vXml_getElementAtFront(xml->srConf_xml, pathbuff);
		if (codexml == NULL)
		{
			result = CCOSRCONF_STATUS_CANNOTFINDMODE;
			break;
		}
		snprintf(pathbuff, sizeof(pathbuff), "sheetreader/outFormat/%s/propertyCode", mode);
		propxml = cco_vXml_getElementAtFront(xml->srConf_xml, pathbuff);
		if (propxml == NULL)
		{
			result = CCOSRCONF_STATUS_CANNOTFINDMODE;
			break;
		}
		cco_release(xml->srConf_outCode);
		cco_release(xml->srConf_outPropertyCode);
		xml->srConf_outCode = cco_vXml_getContent(codexml);
		xml->srConf_outPropertyCode = cco_vXml_getContent(propxml);

	} while (0);
	cco_safeRelease(codexml);
	cco_safeRelease(propxml);
	return result;
}
