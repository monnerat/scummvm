// HACK: Instead of using the full st_i.h (and then st.h and stconfig.h etc.)
// from SoX, we use this minimal variant which is just sufficient to make
// resample.c and rate.c compile.

#ifndef RATE_H
#define RATE_H

#include <stdio.h>
#include <assert.h>
#include "scummsys.h"
#include "common/engine.h"
#include "common/util.h"

#include "audiostream.h"

typedef int16 st_sample_t;
typedef uint32 st_size_t;
typedef uint32 st_rate_t;

typedef struct {
	bool used;
	byte priv[1024];
} eff_struct;
typedef eff_struct *eff_t;

/* Minimum and maximum values a sample can hold. */
#define ST_SAMPLE_MAX 0x7fffL
#define ST_SAMPLE_MIN (-ST_SAMPLE_MAX - 1L)

#define ST_EOF (-1)
#define ST_SUCCESS (0)

/* here for linear interp.  might be useful for other things */
static st_rate_t st_gcd(st_rate_t a, st_rate_t b)
{
	if (b == 0)
		return a;
	else
		return st_gcd(b, a % b);
}

static inline void clampedAdd(int16& a, int b) {
	int val = a + b;

	if (val > ST_SAMPLE_MAX)
		a = ST_SAMPLE_MAX;
	else if (val < ST_SAMPLE_MIN)
		a = ST_SAMPLE_MIN;
	else
		a = val;
}

// Q&D hack to get this SOX stuff to work
#define st_report warning
#define st_warn warning
#define st_fail error


// Resample (high quality)
int st_resample_getopts(eff_t effp, int n, char **argv);
int st_resample_start(eff_t effp, st_rate_t inrate, st_rate_t outrate);
int st_resample_flow(eff_t effp, AudioInputStream &input, st_sample_t *obuf, st_size_t *osamp);
int st_resample_drain(eff_t effp, st_sample_t *obuf, st_size_t *osamp);
int st_resample_stop(eff_t effp);

// Rate (linear filter, low quality)
int st_rate_getopts(eff_t effp, int n, char **argv);
int st_rate_start(eff_t effp, st_rate_t inrate, st_rate_t outrate);
int st_rate_flow(eff_t effp, AudioInputStream &input, st_sample_t *obuf, st_size_t *osamp);
int st_rate_stop(eff_t effp);

#if 1
class RateConverter {
protected:
	eff_struct effp;
public:
	RateConverter() {}
	virtual ~RateConverter() {}
	virtual int flow(AudioInputStream &input, st_sample_t *obuf, st_size_t *osamp) = 0;
	virtual int drain(st_sample_t *obuf, st_size_t *osamp) = 0;
};

class LinearRateConverter : public RateConverter {
public:
	LinearRateConverter(st_rate_t inrate, st_rate_t outrate) {
		st_rate_getopts(&effp, 0, NULL);
		st_rate_start(&effp, inrate, outrate);
	}
	~LinearRateConverter() {
		st_rate_stop(&effp);
	}
	virtual int flow(AudioInputStream &input, st_sample_t *obuf, st_size_t *osamp) {
		return st_rate_flow(&effp, input, obuf, osamp);
	}
	virtual int drain(st_sample_t *obuf, st_size_t *osamp) {
		return (ST_SUCCESS);
	}
};

class ResampleRateConverter : public RateConverter {
public:
	ResampleRateConverter(st_rate_t inrate, st_rate_t outrate) {
		st_resample_getopts(&effp, 0, NULL);
		st_resample_start(&effp, inrate, outrate);
	}
	~ResampleRateConverter() {
		st_resample_stop(&effp);
	}
	virtual int flow(AudioInputStream &input, st_sample_t *obuf, st_size_t *osamp) {
		return st_resample_flow(&effp, input, obuf, osamp);
	}
	virtual int drain(st_sample_t *obuf, st_size_t *osamp) {
		return st_resample_drain(&effp, obuf, osamp);
	}
};

#endif


#endif
