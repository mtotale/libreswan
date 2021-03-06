/* time objects and functions, for libreswan
 *
 * Copyright (C) 1998, 1999, 2000  Henry Spencer.
 * Copyright (C) 1999, 2000, 2001  Richard Guy Briggs
 * Copyright (C) 2019 Andrew Cagney <cagney@gnu.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <https://www.gnu.org/licenses/lgpl-2.1.txt>.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 *
 */

#include <inttypes.h>		/* for imaxabs() */

#include "deltatime.h"
#include "lswlog.h"

static const deltatime_t deltatime_zero;

/*
 * Rather than deal with the 'bias' in a -ve timeval, this code
 * convers everything into +ve timevals.
 */

static deltatime_t negate_deltatime(deltatime_t d)
{
	deltatime_t res;
	timersub(&deltatime_zero.dt, &d.dt, &res.dt);
	return res;
}

deltatime_t deltatime(time_t secs)
{
	return (deltatime_t) DELTATIME_INIT(secs);
}

deltatime_t deltatime_ms(intmax_t ms)
{
	/*
	 * C99 defines '%' thus:
	 *
	 * [...] the result of the % operator is the remainder. [...]
	 * If the quotient a/b is representable, the expression (a/b)*b
	 * + a%b shall equal a.
	 */
	intmax_t ams = imaxabs(ms);
	deltatime_t res = {
		.dt = {
			.tv_sec = ams / 1000,
			.tv_usec = ams % 1000 * 1000,
		},
	};
	if (ms < 0) {
		res = negate_deltatime(res);
	}
	return res;
}

deltatime_t deltatime_timevals_diff(struct timeval a, struct timeval b)
{
	deltatime_t res;
	timersub(&a, &b, &res.dt);
	return res;
}

int deltatime_cmp(deltatime_t a, deltatime_t b)
{
	/* sign(l - r) */
	if (timercmp(&a.dt, &b.dt, <)) {
		return -1;
	} else if (timercmp(&a.dt, &b.dt, >)) {
		return 1;
	} else {
		return 0;
	}
}

deltatime_t deltatime_max(deltatime_t a, deltatime_t b)
{
	if (timercmp(&a.dt, &b.dt, >)) {
		return a;
	} else {
		return b;
	}
}

deltatime_t deltatime_add(deltatime_t a, deltatime_t b)
{
	deltatime_t res;
	timeradd(&a.dt, &b.dt, &res.dt);
	return res;
}

deltatime_t deltatime_mulu(deltatime_t a, unsigned scalar)
{
	return deltatime_ms(deltamillisecs(a) * scalar);
}

deltatime_t deltatime_divu(deltatime_t a, unsigned scalar)
{
	return deltatime_ms(deltamillisecs(a) / scalar);
}

intmax_t deltamillisecs(deltatime_t d)
{
	return ((intmax_t) d.dt.tv_sec) * 1000 + d.dt.tv_usec / 1000;
}

intmax_t deltasecs(deltatime_t d)
{
	/* XXX: ignore .tv_usec's bias, don't round */
	return d.dt.tv_sec;
}

deltatime_t deltatimescale(int num, int denom, deltatime_t d)
{
	/* ??? should check for overflow */
	return deltatime(deltasecs(d) * num / denom);
}

bool deltaless(deltatime_t a, deltatime_t b)
{
	return timercmp(&a.dt, &b.dt, <);
}

bool deltaless_tv_dt(const struct timeval a, const deltatime_t b)
{
	return timercmp(&a, &b.dt, <);
}

struct timeval deltatimeval(deltatime_t d)
{
	return d.dt;
}

/*
 * Try to be smart by only printing the precision necessary.  For
 * instance 1, 0.5, ...
 */
static size_t frac(struct lswlog *buf, intmax_t usec)
{
	int precision = 6;
	while (usec % 10 == 0 && precision > 1) {
		precision--;
		usec = usec / 10;
	}
	return lswlogf(buf, ".%0*jd", precision, usec);
}

/* fmt_deltatime() */
size_t lswlog_deltatime(struct lswlog *buf, deltatime_t d)
{
	size_t s = 0;
	if (d.dt.tv_sec < 0) {
		s += lswlogf(buf, "-");
		d = negate_deltatime(d);
	}
	s += lswlogf(buf, "%jd", (intmax_t)d.dt.tv_sec);
	if (d.dt.tv_usec != 0) {
		frac(buf, d.dt.tv_usec);
	}
	return s;
}

const char *str_deltatime(deltatime_t d, deltatime_buf *out)
{
	jambuf_t buf = ARRAY_AS_JAMBUF(out->buf);
	lswlog_deltatime(&buf, d);
	return out->buf;
}
