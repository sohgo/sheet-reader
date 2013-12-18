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

#ifndef SHEETREADER_H_
#define SHEETREADER_H_

enum SHEETREADER_STATUS {
	SHEETREADER_STATUS_SUCCESS = 0,
	SHEETREADER_STATUS_HAVE_NO_SRMLS,
	SHEETREADER_STATUS_NOTFOUND_3PATTERNS,
	SHEETREADER_STATUS_NOTFOUND_SID,
	SHEETREADER_STATUS_NOTREAD_IMAGE,
	SHEETREADER_STATUS_NOTFOUND_SRMLFILES
};

typedef enum SHEETREADER_STATUS SHEETREADER_STATUS;

#define SHEETREADER_SYSCONF_NAME "sheetreader"

#endif
