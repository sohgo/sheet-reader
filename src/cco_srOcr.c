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

#include "cco_srOcr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cco_defineClass(cco_srOcr);

cco_srOcr *cco_srOcr_baseNew(int size)
{
	cco_srOcr *o = NULL;
	do {
		if (size < sizeof(cco_srOcr))
		{
			break;
		}
		o = (cco_srOcr *)cco_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srOcr);
		cco_srOcr_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srOcr_baseRelease(void *o)
{
	cco_srOcr_baseFinalize(o);
	cco_baseRelease(o);
}

void cco_srOcr_baseInitialize(cco_srOcr *o)
{
	o->baseRelease = &cco_srOcr_baseRelease;
	o->srOcr_initialize = &cco_srOcr_initialize;
	o->srOcr_setImage = &cco_srOcr_setImage;
	o->srOcr_getRecognizeString = &cco_srOcr_getRecognizeString;
	o->srOcr_setOption = &cco_srOcr_setOption;
	return;
}

void cco_srOcr_baseFinalize(cco_srOcr *o)
{
	return;
}

cco_srOcr *cco_srOcr_new()
{
	return cco_srOcr_baseNew(sizeof(cco_srOcr));
}

void cco_srOcr_release(void *o)
{
	cco_release(o);
}

CCOSROCR_STATUS cco_srOcr_initialize(void *obj, char *configfile)
{
	return CCOSROCR_STATUS_EMPTYCLASS;
}

CCOSROCR_STATUS cco_srOcr_setImage(void *obj, char *imagefile)
{
	return CCOSROCR_STATUS_EMPTYCLASS;
}

CCOSROCR_STATUS cco_srOcr_getRecognizeString(void *obj, cco_vString **recognizedString)
{
	return CCOSROCR_STATUS_EMPTYCLASS;
}

CCOSROCR_STATUS cco_srOcr_setOption(void *obj, char *option)
{
	return CCOSROCR_STATUS_SUCCESS;
}

