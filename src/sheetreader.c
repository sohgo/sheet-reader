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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <opencv/highgui.h>
#include <cluscore/cco_vString.h>
#include "sheetreader.h"
#include "cco_srAnalyzer.h"
#include "cco_srMl.h"
#include "config.h"

/* KOCR */
char *ocrdb_dir = NULL;

/**
 * This function should be print usage of this program.
 */
void sheetreader_usage()
{
	printf("usage: sheetreader [-m output-mode] [-c configuration-directory] image-file\n");
	printf(" -m sql rails xml.\n");
	printf(" -c default is %s/etc/sheetreader\n", PREFIX);
	printf(" -s set sender-number.\n");
	printf(" -r set receiver-number.\n");
	printf(" -p set prefix of directory to save images.\n");
	printf(" -u force set user-id.\n");
	printf(" -i force set sheet-id.\n");
	printf(" -l KOCR character database {list-num.db, list-mbs.db, list-ocrb.db} dir.\n");
}


char *sheetreader_errmsg(int id)
{
	char *result;
	switch (id)
	{
	case SHEETREADER_STATUS_HAVE_NO_SRMLS:
		result = "SHEETREADER_STATUS_HAVE_NO_SRMLS";
		break;
	case SHEETREADER_STATUS_NOTREAD_IMAGE:
		result = "SHEETREADER_STATUS_NOTREAD_IMAGE";
		break;
	case CCOSRANALYZER_STATUS_CANNOT_SAVE_IMAGE:
		result = "CCOSRANALYZER_STATUS_CANNOT_SAVE_IMAGE";
		break;
	case SHEETREADER_STATUS_NOTFOUND_3PATTERNS:
		result = "SHEETREADER_STATUS_NOTFOUND_3PATTERNS";
		break;
	case SHEETREADER_STATUS_NOTFOUND_SID:
		result = "SHEETREADER_STATUS_NOTFOUND_SID";
		break;
	default:
		result = "SHEETREADER_STATUS_UNKNOWN";
		break;
	}
	return result;
}

/**
 * This is the main of Sheetreader.
 */
int main(int argc, char *argv[])
{
	int result = 0;
	int optval;
	int debug = 0;
	char *mode = NULL;
	char *configdir = NULL;
	int help = 0;
	cco_srAnalyzer *srAnalyzer = NULL;
	CCOSRANALYZER_STATUS analyzer_result = CCOSRANALYZER_STATUS_SUCCESS;
	cco_vString *tmp_string = NULL;
	char *tmp_cstring = NULL;
	cco_vString *sender_string = cco_vString_new("UNNUMBER");
	cco_vString *receiver_string = cco_vString_new("UNNUMBER");
	cco_vString *saveprefix_string = cco_vString_new("");
	cco_vString *sheetid_string = cco_vString_new("");
	cco_vString *userid_string = cco_vString_new("");
#ifdef KOCR
	cco_vString *ocr_string = cco_vString_new("kocr");
#else
	cco_vString *ocr_string = cco_vString_new("gocr");
#endif
	char sr_result[64];

	do {
		/* Checks options. */
		while ((optval = getopt(argc, argv, "l:o:u:i:p:s:r:d:m:c:v?")) != -1) {
			switch (optval) {
			case 'd':
				debug = atoi(optarg);
				break;
			case 'm':
				mode = optarg;
				break;
			case 'c':
				configdir = strdup(optarg);
				break;
			case 's':
				cco_safeRelease(sender_string);
				sender_string = cco_vString_new(optarg);
				break;
			case 'r':
				cco_safeRelease(receiver_string);
				receiver_string = cco_vString_new(optarg);
				break;
			case 'p':
				cco_safeRelease(saveprefix_string);
				saveprefix_string = cco_vString_new(optarg);
				break;
			case 'o':
				cco_safeRelease(ocr_string);
				ocr_string = cco_vString_new(optarg);
				break;
			case 'u':
				cco_safeRelease(userid_string);
				userid_string = cco_vString_new(optarg);
				break;
			case 'i':
				cco_safeRelease(sheetid_string);
				sheetid_string = cco_vString_new(optarg);
				break;
			case '?':
			case 'v':
				help = 1;
				break;
#ifdef KOCR
			case 'l':
				ocrdb_dir = strdup(optarg);
				break;
#else
			case 'l':
				ocrdb_dir = strdup(optarg);
				break;
#endif
			}
			optarg = NULL;
		}
		if (argc < 2 || optind != (argc - 1) || help != 0)
		{
			sheetreader_usage();
			result = -1;
			break;
		}

		srAnalyzer = cco_srAnalyzer_new();
		srAnalyzer->srAnalyzer_debug = debug;
		cco_safeRelease(srAnalyzer->srAnalyzer_save_prefix);
		srAnalyzer->srAnalyzer_save_prefix = cco_get(saveprefix_string);
		cco_safeRelease(srAnalyzer->srAnalyzer_sender);
		srAnalyzer->srAnalyzer_sender = cco_get(sender_string);

		cco_safeRelease(srAnalyzer->srAnalyzer_receiver);
		srAnalyzer->srAnalyzer_receiver = cco_get(receiver_string);

		cco_safeRelease(srAnalyzer->srAnalyzer_uid);
		srAnalyzer->srAnalyzer_uid = cco_get(userid_string);

		cco_safeRelease(srAnalyzer->srAnalyzer_sid);
		srAnalyzer->srAnalyzer_sid = cco_get(sheetid_string);

		cco_safeRelease(srAnalyzer->srAnalyzer_ocr);
		srAnalyzer->srAnalyzer_ocr = cco_get(ocr_string);

		if (configdir == NULL)
		{
			tmp_string = cco_vString_newWithFormat("%s/etc/sheetreader", PREFIX);
			configdir = tmp_string->v_getCstring(tmp_string);
			cco_safeRelease(tmp_string);
		}
		tmp_string = cco_vString_newWithFormat("%s/srml", configdir);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		cco_srMl_readDirectory(srAnalyzer->srAnalyzer_srml, tmp_cstring, 5);
		free(tmp_cstring);
		cco_safeRelease(tmp_string);

		tmp_string = cco_vString_newWithFormat("%s/config.xml", configdir);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		cco_srAnalyzer_readSrconf(srAnalyzer, tmp_cstring);
		free(tmp_cstring);
		cco_safeRelease(tmp_string);

		tmp_string = cco_vString_newWithFormat("%s/marker-sample.png", configdir);
		tmp_cstring = tmp_string->v_getCstring(tmp_string);
		analyzer_result = cco_srAnalyzer_setMarkerSampleImageFile(tmp_cstring);
		if (analyzer_result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			fprintf(stderr, "Error: Sample marker image file(%s) is not found\n", tmp_cstring);
			result = analyzer_result;
		}
		free(tmp_cstring);
		cco_safeRelease(tmp_string);
		if (analyzer_result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			break;
		}

		/* Counts usable srml. */
		if (cco_srAnalyzer_countSheets(srAnalyzer) <= 0)
		{
			fprintf(stderr, "Error: The Srml file could not be retrieved in %s/etc/sheetreader.\n",
					PREFIX);
			result = SHEETREADER_STATUS_HAVE_NO_SRMLS;
		}
		/* Analyzes an image. */
		analyzer_result = cco_srAnalyzer_readImage(srAnalyzer, argv[optind]);
		if (analyzer_result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			fprintf(stderr, "Error: The inputed image could not be loaded.\n");
			result = SHEETREADER_STATUS_NOTREAD_IMAGE;
			break;
		}
		analyzer_result = cco_srAnalyzer_backupImage(srAnalyzer);
		if (analyzer_result != CCOSRANALYZER_STATUS_SUCCESS)
		{
			fprintf(stderr, "Error: The inputed image could not be saved.\n");
			result = CCOSRANALYZER_STATUS_CANNOT_SAVE_IMAGE;
			break;
		}
		do {
			analyzer_result = cco_srAnalyzer_adjustImage(srAnalyzer);
			if (analyzer_result != CCOSRANALYZER_STATUS_SUCCESS)
			{
				fprintf(stderr, "Error: Three patterns could not be retrieved in the inputed image(%s).\n", argv[optind]);
				result = SHEETREADER_STATUS_NOTFOUND_3PATTERNS;
				break;
			}
			analyzer_result = cco_srAnalyzer_ocr(srAnalyzer);
			if (analyzer_result != CCOSRANALYZER_STATUS_SUCCESS)
			{
				fprintf(stderr, "Error: The Sheet ID could not be retrieved in the inputed image(%s).\n", argv[optind]);
				result = SHEETREADER_STATUS_NOTFOUND_SID;
				break;
			}
		} while (0);
		/* Put analyzed data. */
		snprintf(sr_result, sizeof(sr_result), "%d", result);
		cco_srAnalyzer_resultPrint(srAnalyzer, mode, sr_result);
	} while (0);

	if (configdir != NULL) free(configdir);
	cco_safeRelease(ocr_string);
	cco_safeRelease(userid_string);
	cco_safeRelease(sheetid_string);
	cco_safeRelease(saveprefix_string);
	cco_safeRelease(receiver_string);
	cco_safeRelease(sender_string);
	cco_release(srAnalyzer);
	return result;
}
