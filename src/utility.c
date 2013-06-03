/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int utility_strctl_nsearchn2s(int n, char *p, char *cmdsstr, char *buf, int bufsize)
{
	int count_cmd;
	char *valp;
	int count_size;
	int result;

	if (bufsize <= 0) {
		return(-1);
	}

	result = -1;
	count_cmd = 0;
	count_size = 1;
	valp = buf;
	while (*cmdsstr != 0) {
		if (n == count_cmd) {
			result = 0;
			while (*cmdsstr != 0 && *cmdsstr != *p) {
				if (count_size < bufsize) {
					*valp = *cmdsstr;
					valp ++;
					cmdsstr ++;
					count_size ++;
				} else {
					break;
				}
			}
			break;
		}
		if (*cmdsstr == *p) {
			count_cmd ++;
		}
		cmdsstr ++;
	}
	*valp = 0;
	return(result);
}

char *utility_strctl_strdup(const char *fmt, ...)
{
	va_list		ap;
	char *buf;
	int result = 0;
	char tmpbuf[10];

	buf = NULL;

	va_start(ap, fmt);
	result = vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, ap);
	va_end(ap);
	buf = malloc(result + 100);
	if (buf == NULL) {
		return(NULL);
	}
	va_start(ap, fmt);
	result = vsnprintf(buf, result + 1, fmt, ap);
	va_end(ap);
	return(buf);
}

int utility_mkdir(char *str) {
	char buf[256];
	char *dirstr;
	char *dirstr_tmp;
	int n;
	int c;
	DIR *dp;

	if (*str == '/') {
		n = 1;
		utility_strctl_nsearchn2s(n, "/", str, buf, sizeof(buf));
		dirstr = utility_strctl_strdup("/%s", buf);
	} else {
		n = 0;
		utility_strctl_nsearchn2s(n, "/", str, buf, sizeof(buf));
		dirstr = utility_strctl_strdup("%s", buf);
	}

	while (dirstr != NULL) {
		dp = opendir(dirstr);
		if (dp == NULL) {
			c = mkdir(dirstr, S_IRUSR | S_IWUSR | S_IXUSR |
			    S_IRGRP | S_IWGRP | S_IXGRP |
			    S_IROTH | S_IWOTH | S_IXOTH);

			if (c != 0) {
				free(dirstr);
				return(-1);
			}
		} else {
			closedir(dp);
		}
		n = n + 1;
		utility_strctl_nsearchn2s(n, "/", str, buf, sizeof(buf));
		if (strlen(buf) == 0) {
			free(dirstr);
			break;
		}
		dirstr_tmp = dirstr;
		dirstr = utility_strctl_strdup("%s/%s", dirstr, buf);
		free(dirstr_tmp);
	}
	return(0);
}
