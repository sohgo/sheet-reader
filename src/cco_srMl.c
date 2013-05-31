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

#include "cco_srMl.h"
#include "cco_srMlSheet.h"
#include <stdio.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

cco_defineClass(cco_srMl);

cco_srMl *cco_srMl_baseNew(int size)
{
	cco_srMl *o = NULL;
	do {
		if (size < sizeof(cco_srMl))
		{
			break;
		}
		o = (cco_srMl *)cco_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srMl);
		cco_srMl_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srMl_baseRelease(void *o)
{
	cco_srMl_baseFinalize(o);
	cco_baseRelease(o);
}

void cco_srMl_baseInitialize(cco_srMl *o)
{
	o->baseRelease = &cco_srMl_baseRelease;
	o->srMl_xml = cco_vXml_new();
	o->srMl_sheets = cco_redblacktree_new();

	return;
}

void cco_srMl_baseFinalize(cco_srMl *o)
{
	cco_release(o->srMl_xml);
	cco_release(o->srMl_sheets);
	return;
}

cco_srMl *cco_srMl_new()
{
	return cco_srMl_baseNew(sizeof(cco_srMl));
}

void cco_srMl_release(void *o)
{
	cco_release(o);
}

cco_srMlSheet *cco_srMl_getSheet(cco_srMl *xml, cco_vString *id)
{
	return (cco_srMlSheet *)cco_redblacktree_get(xml->srMl_sheets, (cco_v *)id);
}

CCOSRML_STATUS cco_srMl_examineHeder(cco_srMl *xml)
{
	CCOSRML_STATUS result = CCOSRML_STATUS_SUCCESS;
	cco_vXml *curxml = NULL;
	cco_vXml *cursor_xmlCellWidth = NULL;
	cco_vXml *cursor_xmlCellHeight = NULL;
	cco_vXml *elexml = NULL;
	cco_vString *id_string = NULL;
	cco_vString *width_string = NULL;
	cco_vString *height_string = NULL;
	cco_vString *xml_attr_cell_number = NULL;
	cco_vString *xml_attr_cell_length = NULL;
	cco_arraylist *list = NULL;
	cco_srMlSheet *sheet = NULL;
	cco_arraylist *xml_cellWidths = NULL;
	cco_arraylist *xml_cellHeights = NULL;
	int cell_number = 0;
	int cell_length = 0;

	do {
		curxml = cco_vXml_getElementAtFront(xml->srMl_xml, "srMl");
		if (curxml == NULL)
		{
			result = CCOSRML_STATUS_NOTREADEDXML;
			break;
		}
		cco_safeRelease(curxml);
		list = cco_vXml_getElements(xml->srMl_xml, "srMl/sheet");
		cco_arraylist_setCursorAtFront(list);
		while (curxml = (cco_vXml *)cco_arraylist_getAtCursor(list))
		{
			sheet = cco_srMlSheet_new();
			elexml = cco_vXml_getElementAtFront(curxml, "id");
			id_string = (cco_vString *)cco_vXml_getContent(elexml);
			cco_safeRelease(elexml);
			elexml = cco_vXml_getElementAtFront(curxml, "blockWidth");
			width_string = (cco_vString *)cco_vXml_getContent(elexml);
			cco_safeRelease(elexml);
			elexml = cco_vXml_getElementAtFront(curxml, "blockHeight");
			height_string = (cco_vString *)cco_vXml_getContent(elexml);
			cco_safeRelease(elexml);

			xml_cellWidths = cco_vXml_getElements(curxml, "cellWidth/cellAttribute");
			cco_arraylist_setCursorAtFront(xml_cellWidths);
			while ((cursor_xmlCellWidth = (cco_vXml *) cco_arraylist_getAtCursor(xml_cellWidths)) != NULL)
			{
				int satisfied_flag = 1;

				xml_attr_cell_number = cco_vXml_getAttribute(cursor_xmlCellWidth, "number");
				xml_attr_cell_length = cco_vXml_getAttribute(cursor_xmlCellWidth, "length");

				if (xml_attr_cell_number == NULL)
				{
					satisfied_flag = 0;
				} else {
					cell_number = cco_vString_toInt(xml_attr_cell_number);
				}
				if (xml_attr_cell_length == NULL)
				{
					satisfied_flag = 0;
				}

				if (satisfied_flag == 0) {
					printf("Format ERROR: Not satisfied with specification of srML: this cellAttribute line was igrenored.\n");
				} else {
					cco_srMlSheet_setCellWidth(sheet, xml_attr_cell_length, cell_number);
				}

				cco_safeRelease(xml_attr_cell_number);
				cco_safeRelease(xml_attr_cell_length);
				cco_arraylist_setCursorAtNext(xml_cellWidths);

			}
			cco_safeRelease(xml_cellWidths);

			xml_cellHeights = cco_vXml_getElements(curxml, "cellHeight/cellAttribute");
			cco_arraylist_setCursorAtFront(xml_cellHeights);
			while ((cursor_xmlCellHeight = (cco_vXml *) cco_arraylist_getAtCursor(xml_cellHeights)) != NULL)
			{
				int satisfied_flag = 1;

				xml_attr_cell_number = cco_vXml_getAttribute(cursor_xmlCellHeight, "number");
				xml_attr_cell_length = cco_vXml_getAttribute(cursor_xmlCellHeight, "length");

				if (xml_attr_cell_number == NULL)
				{
					satisfied_flag = 0;
				} else {
					cell_number = cco_vString_toInt(xml_attr_cell_number);
				}
				if (xml_attr_cell_length == NULL)
				{
					satisfied_flag = 0;
				}

				if (satisfied_flag == 0) {
					printf("Format ERROR: Not satisfied with specification of srML: this cellAttribute line was igrenored.\n");
				} else {
					cco_srMlSheet_setCellHeight(sheet, xml_attr_cell_length, cell_number);
				}

				cco_safeRelease(xml_attr_cell_number);
				cco_safeRelease(xml_attr_cell_length);
				cco_arraylist_setCursorAtNext(xml_cellHeights);

			}
			cco_safeRelease(xml_cellHeights);

			cco_srMlSheet_setXml(sheet, curxml);
			cco_srMlSheet_setId(sheet, id_string);
			cco_srMlSheet_setWidth(sheet, width_string);
			cco_srMlSheet_setHeight(sheet, height_string);
			cco_redblacktree_insert(xml->srMl_sheets, (cco_v *)id_string, (cco *)sheet);
			cco_safeRelease(id_string);
			cco_safeRelease(width_string);
			cco_safeRelease(height_string);
			cco_safeRelease(sheet);
			cco_safeRelease(curxml);
			cco_arraylist_setCursorAtNext(list);
		}
		cco_safeRelease(list);
	} while (0);
	cco_safeRelease(curxml);
	return result;
}

CCOSRML_STATUS cco_srMl_readFile(cco_srMl *xml, char *filename)
{
	CCOSRML_STATUS result = CCOSRML_STATUS_SUCCESS;
	do
	{
		if (xml == NULL)
		{
			result = CCOSRML_STATUS_ERROR;
			break;
		}
		cco_vXml_read(xml->srMl_xml, filename);
		cco_srMl_examineHeder(xml);
		cco_safeRelease(xml->srMl_xml);
		xml->srMl_xml = cco_vXml_new();
	} while (0);
	return result;
}

int cco_srMl_readDirectory(cco_srMl *obj, char *directorynaame, int depth)
{
	int result = 0;
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;

	regex_t preg;
	regmatch_t pmatch[1];

	cco_vString *currentdirectory_string;
	char *currentdirectory_cstring;

	if ((dp = opendir(directorynaame)) == NULL) {
		fprintf(stderr, "cannot open directory %s in cco_srMl_readDirectory.", directorynaame);
		return -1;
	}
	while ((entry = readdir(dp)) != NULL) {
		if (*entry->d_name != '/') {
			currentdirectory_string = cco_vString_newWithFormat("%s/%s", directorynaame, entry->d_name);
			if (currentdirectory_string == NULL) {
				closedir(dp);
				return -1;
			}
		} else {
			currentdirectory_string = cco_vString_newWithFormat("%s", directorynaame);
			if (currentdirectory_string == NULL) {
				closedir(dp);
				return -1;
			}
		}
		currentdirectory_cstring = currentdirectory_string->v_getCstring(currentdirectory_string);
		stat(currentdirectory_cstring, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			if (strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0) {
				free(currentdirectory_cstring);
				currentdirectory_cstring = NULL;
				cco_safeRelease(currentdirectory_string);
				continue;
			}
			if (depth > 0) {
				if (cco_srMl_readDirectory(obj, currentdirectory_cstring, depth - 1) < 0) {
					free(currentdirectory_cstring);
					currentdirectory_cstring = NULL;
					cco_safeRelease(currentdirectory_string);
					return -1;
				}
			}
		} else {
			regcomp(&preg, ".xml$", REG_EXTENDED);
			if (regexec(&preg, entry->d_name, 0, pmatch, 0) == 0){
				cco_srMl_readFile(obj, currentdirectory_cstring);
			}
			regfree(&preg);
		}
		free(currentdirectory_cstring);
		currentdirectory_cstring = NULL;
		cco_safeRelease(currentdirectory_string);
	}
	return result;
}

int cco_srMl_countSheets(cco_srMl *obj)
{
	return cco_redblacktree_count(obj->srMl_sheets);
}
