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

#include "cco_srDb.h"
#include "cco_srAnalyzer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cco_defineClass(cco_srDb);

cco_srDb *cco_srDb_baseNew(int size)
{
	cco_srDb *o = NULL;
	do {
		if (size < sizeof(cco_srDb))
		{
			break;
		}
		o = (cco_srDb *)cco_baseNew(size);
		if (o == NULL)
		{
			break;
		}
		cco_setClass(o, cco_srDb);
		cco_srDb_baseInitialize(o);
	} while (0);
	return o;
}

void cco_srDb_baseRelease(void *o)
{
	cco_srDb_baseFinalize(o);
	cco_baseRelease(o);
}

void cco_srDb_baseInitialize(cco_srDb *o)
{
	o->baseRelease = &cco_srDb_baseRelease;
	o->cco_srDb_mysql = NULL;

	return;
}

void cco_srDb_baseFinalize(cco_srDb *o)
{
	if (o->cco_srDb_mysql != NULL)
	{
		mysql_close(o->cco_srDb_mysql);
	}
	return;
}

cco_srDb *cco_srDb_new()
{
	return cco_srDb_baseNew(sizeof(cco_srDb));
}

void cco_srDb_release(void *o)
{
	cco_release(o);
}

int cco_srDb_setHost(cco_srDb *obj, cco_vString *host)
{
	if (host != NULL)
	{
		obj->cco_srDb_mysqlHost = cco_get(host);
	}
	return 0;
}

int cco_srDb_setUser(cco_srDb *obj, cco_vString *user, cco_vString *passwd)
{
	cco_safeRelease(obj->cco_srDb_mysqlUser);
	if (user != NULL)
	{
		obj->cco_srDb_mysqlUser = cco_get(user);
	}
	cco_safeRelease(obj->cco_srDb_mysqlPassword);
	if (passwd != NULL)
	{
		obj->cco_srDb_mysqlPassword = cco_get(passwd);
	}
	return 0;
}

int cco_srDb_setDb(cco_srDb *obj, cco_vString *db)
{
	if (db != NULL)
	{
		obj->cco_srDb_mysqlDb = cco_get(db);
	}
	return 0;
}

CCOSRANALYZER_STATUS cco_srDb_connect(cco_srDb *obj)
{
	CCOSRANALYZER_STATUS result =CCOSRANALYZER_STATUS_SUCCESS;
	char *user = NULL;
	char *passwd = NULL;
	char *host = NULL;
	char *db = NULL;
	do {
		if (obj->cco_srDb_mysql == NULL)
		{
			obj->cco_srDb_mysql = mysql_init(NULL);
		}
		if (obj->cco_srDb_mysql != NULL)
		{
			result = CCOSRANALYZER_STATUS_ERROR;
			fprintf(stderr, "Error %u: %s\n", mysql_errno(obj->cco_srDb_mysql), mysql_error(obj->cco_srDb_mysql));
			break;
		}
		user = obj->cco_srDb_mysqlUser->v_getCstring(obj->cco_srDb_mysqlUser);
		host = obj->cco_srDb_mysqlHost->v_getCstring(obj->cco_srDb_mysqlHost);
		passwd = obj->cco_srDb_mysqlPassword->v_getCstring(obj->cco_srDb_mysqlPassword);
		db = obj->cco_srDb_mysqlDb->v_getCstring(obj->cco_srDb_mysqlDb);
		if (mysql_real_connect(obj->cco_srDb_mysql, user, host, passwd, db, 0, NULL, 0)  == NULL) {
			result = CCOSRANALYZER_STATUS_ERROR;
			fprintf(stderr, "Error %u: %s\n", mysql_errno(obj->cco_srDb_mysql), mysql_error(obj->cco_srDb_mysql));
			break;
		}
	} while(0);
	return result;
}

