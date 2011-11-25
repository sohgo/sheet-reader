/* * Shinsai FaxOCR * * Copyright (C) 2009-2011 National Institute of
Public Health, Japan. * All rights Reserved. * * This program is free
software; you can redistribute it and/or modify * it under the terms
of the GNU General Public License as published by * the Free Software
Foundation * * This program is distributed in the hope that it will be
useful, * but WITHOUT ANY WARRANTY; without even the implied warranty
of * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the *
GNU General Public License for more details. * * You should have
received a copy of the GNU General Public License * along with this
program; if not, write to the Free Software * Foundation, Inc., 59
Temple Place, Suite 330, Boston, MA  02111-1307  USA * */

#ifndef CCO_SRDB_H_
#define CCO_SRDB_H_

#include <cluscore/cco.h>
#include <cluscore/cco_vString.h>
#include <mysql/mysql.h>
//#include <mysql/my_global.h>

#define CCO_SRDB_PROPERTIES \
	MYSQL *cco_srDb_mysql;\
	cco_vString *cco_srDb_mysqlHost;\
	cco_vString *cco_srDb_mysqlDb;\
	cco_vString *cco_srDb_mysqlUser;\
	cco_vString *cco_srDb_mysqlPassword;

typedef struct cco_srDb cco_srDb;

struct cco_srDb {
	CCO_PROPERTIES
	CCO_SRDB_PROPERTIES
};

cco_srDb *cco_srDb_baseNew(int size);
void cco_srDb_baseRelease(void *cco);
void cco_srDb_baseInitialize(cco_srDb *cco);
void cco_srDb_baseFinalize(cco_srDb *cco);
cco_srDb *cco_srDb_new();
void cco_srDb_release(void *cco);


/* Don't touch following comment.
CCOINHERITANCE:CCO_PROPERTIES
CCOINHERITANCE:CCO_SRDB_PROPERTIES
*/

#endif /* CCO_SRDB_H_ */
