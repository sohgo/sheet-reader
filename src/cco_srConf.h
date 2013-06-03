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

#ifndef CCO_SRCONF_H_
#define CCO_SRCONF_H_

#include <cluscore/cco.h>
#include <cluscore/cco_vString.h>
#include <cluscore/cco_vXml.h>

#define CCO_SRCONF_PROPERTIES \
	cco_vXml *srConf_xml;\
	cco_vString *srConf_outCode;\
	cco_vString *srConf_outPropertyCode;

typedef struct cco_srConf cco_srConf;

struct cco_srConf {
	CCO_PROPERTIES
	CCO_SRCONF_PROPERTIES
};

typedef enum CCOSRCONF_STATUS CCOSRCONF_STATUS;

enum CCOSRCONF_STATUS {
	CCOSRCONF_STATUS_SUCCESS = 0,
	CCOSRCONF_STATUS_ERROR,
	CCOSRCONF_STATUS_NOTREADEDXML,
	CCOSRCONF_STATUS_CANNOTFINDMODE
};

cco_srConf *cco_srConf_baseNew(int size);
void cco_srConf_baseRelease(void *cco);
void cco_srConf_baseInitialize(cco_srConf *cco);
void cco_srConf_baseFinalize(cco_srConf *cco);
cco_srConf *cco_srConf_new();
void cco_srConf_release(void *cco);
CCOSRCONF_STATUS cco_srConf_setCurrentMode(cco_srConf *xml, char *mode);
CCOSRCONF_STATUS cco_srConf_read(cco_srConf *obj, char *filename);

/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SRCONF_PROPERTIES
*/

#endif /* CCO_SRCONF_H_ */
