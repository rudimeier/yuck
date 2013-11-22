/*** yuck.c -- generate umbrella commands
 *
 * Copyright (C) 2013 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of yuck.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***/
#if defined HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

#if !defined LIKELY
# define LIKELY(_x)	__builtin_expect((_x), 1)
#endif	/* !LIKELY */
#if !defined UNLIKELY
# define UNLIKELY(_x)	__builtin_expect((_x), 0)
#endif	/* UNLIKELY */
#if !defined UNUSED
# define UNUSED(_x)	_x __attribute__((unused))
#endif	/* !UNUSED */

#if !defined countof
# define countof(x)	(sizeof(x) / sizeof(*x))
#endif	/* !countof */

#define _paste(x, y)	x ## y
#define paste(x, y)	_paste(x, y)
#if !defined with
# define with(args...)							\
	for (args, *paste(__ep, __LINE__) = (void*)1;			\
	     paste(__ep, __LINE__); paste(__ep, __LINE__) = 0)
#endif	/* !with */

struct usg_s {
	char *umb;
	char *cmd;
	char *desc;
};

struct opt_s {
	char sopt;
	char *lopt;
	char *larg;
	char *desc;
};


static __attribute__((format(printf, 1, 2))) void
error(const char *fmt, ...)
{
	va_list vap;
	va_start(vap, fmt);
	vfprintf(stderr, fmt, vap);
	va_end(vap);
	if (errno) {
		fputc(':', stderr);
		fputc(' ', stderr);
		fputs(strerror(errno), stderr);
	}
	fputc('\n', stderr);
	return;
}

static inline __attribute__((unused)) void*
deconst(const void *cp)
{
	union {
		const void *c;
		void *p;
	} tmp = {cp};
	return tmp.p;
}

static FILE*
get_fn(int argc, char *argv[])
{
	FILE *res;

	if (argc > 1) {
		const char *fn = argv[1];
		if (UNLIKELY((res = fopen(fn, "r")) == NULL)) {
			error("cannot open file `%s'", fn);
		}
	} else {
		res = stdin;
	}
	return res;
}


static int
usagep(struct usg_s *restrict tgt, const char *line, size_t llen)
{
#define STREQLITP(x, lit)      (!strncasecmp((x), lit, sizeof(lit) - 1))
	static struct usg_s cur_usg;
	const char *sp;
	const char *up;
	const char *cp;
	const char *const ep = line + llen;

	if (!STREQLITP(line, "usage:")) {
		return 0;
	}
	/* overread whitespace then */
	for (sp = line + sizeof("usage:") - 1; sp < ep && isspace(*sp); sp++);
	/* first thing should name the umbrella, find its end */
	for (up = sp; sp < ep && !isspace(*sp); sp++);

	if (cur_usg.umb && !strncasecmp(cur_usg.umb, up, sp - up)) {
		/* nothing new and fresh */
		;
	} else {
		if (cur_usg.umb) {
			/* free the old guy */
			free(cur_usg.umb);
		}
		cur_usg.umb = strndup(up, sp - up);
	}

	/* overread more whitespace then */
	for (; sp < ep && isspace(*sp); sp++);
	/* time for the command innit */
	for (cp = sp; sp < ep && !isspace(*sp); sp++);

	if (cur_usg.cmd && !strncasecmp(cur_usg.cmd, cp, sp - cp)) {
		/* nothing new and fresh */
		;
	} else {
		if (cur_usg.cmd) {
			/* free the old guy */
			free(cur_usg.cmd);
		}
		cur_usg.cmd = strndup(cp, sp - cp);
	}

	*tgt = cur_usg;
	return 1;
}

static int
optionp(struct opt_s *restrict tgt, const char *line, size_t llen)
{
	static struct opt_s cur_opt;
	static size_t ndesc;
	static size_t zdesc;
	const char *sp;
	const char *const ep = line + llen;

	/* overread whitespace */
	for (sp = line; sp < ep && isspace(*sp); sp++);
	if (sp - line < 2) {
		/* reset res */
		return 0;
	} else if (sp - line >= 8) {
		goto desc;
	}
	/* otherwise *sp was '-'
	 * should be an option then, innit? */
	if (*++sp >= '0') {
		char sopt = *sp++;

		/* eat a comma as well */
		if (ispunct(*sp)) {
			sp++;
		}
		if (!isspace(*sp++)) {
			/* dont know -x.SOMETHING? */
			return 0;
		}
		cur_opt.sopt = sopt;
		if (*sp++ == '-') {
			/* must be a --long now, maybe */
			;
		}
	} else if (*sp == '-') {
		/* --option */
		cur_opt.sopt = '\0';
	} else {
		/* dont know what this is */
		return 0;
	}

	/* --option */
	if (cur_opt.lopt != NULL) {
		/* free the old guy */
		free(cur_opt.lopt);
		cur_opt.lopt = NULL;

		if (cur_opt.larg != NULL) {
			/* free the old guy */
			free(cur_opt.larg);
			cur_opt.larg = NULL;
		}
	}
	if (*sp++ == '-') {
		const char *op;

		for (op = sp; sp < ep && !isspace(*sp) && *sp != '='; sp++);
		cur_opt.lopt = strndup(op, sp - op);

		if (*sp++ == '=') {
			/* has got an arg */
			const char *ap;
			for (ap = sp; sp < ep && !isspace(*sp); sp++);
			cur_opt.larg = strndup(ap, sp - ap);
		}
	}
	/* require at least one more space? */
	;
	/* space eater */
	for (; sp < ep && isspace(*sp); sp++);
	/* dont free but reset the old guy */
	ndesc = 0U;
desc:
	with (size_t zp = llen - (sp - line)) {
		if (ndesc + zp > zdesc) {
			zdesc = ((ndesc + zp) / 256U + 1U) * 256U;
			cur_opt.desc = realloc(cur_opt.desc, zdesc);
		}
		memcpy(cur_opt.desc + ndesc, sp, zp);
		ndesc += zp;
		cur_opt.desc[ndesc] = '\0';
	}
	*tgt = cur_opt;
	return 1;
}


static int
snarf_ln(char *line, size_t llen)
{
	struct usg_s usg[1U];
	struct opt_s opt[1U];

	/* first keep looking for Usage: lines */
	if (UNLIKELY(usagep(usg, line, llen))) {
		/* new umbrella, or new command */
		printf("set_umb(%s, %s)\n", usg->umb, usg->cmd);
	} else if (optionp(opt, line, llen)) {
		printf("set_opt(-%c, --%s%s, \"%s\")\n",
		       opt->sopt ?: '?', opt->lopt, opt->larg, opt->desc);
	}
	return 0;
}

static int
snarf_f(FILE *f)
{
	char *line = NULL;
	size_t llen = 0U;
	ssize_t nrd;

	while ((nrd = getline(&line, &llen, f)) > 0) {
		if (*line == '#') {
			continue;
		}
		snarf_ln(line, nrd);
	}

	free(line);
	return 0;
}


int
main(int argc, char *argv[])
{
	int rc = 0;
	FILE *yf;

	if (UNLIKELY((yf = get_fn(argc, argv)) == NULL)) {
		rc = -1;
	} else {
		/* let the snarfing begin */
		rc = snarf_f(yf);
		/* clean up */
		fclose(yf);
	}

	return -rc;
}

/* yuck.c ends here */