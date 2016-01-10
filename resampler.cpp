#include "resampler.h"
#include <memory>

//Matlab
//elliptic IIR 12th order Fs=595613Hz, Fpass=20kHz, Fstop=24kHz, Apass=.1, Astop=106
//butterworth 4th order IIR Fs=595613Hz, Fc=16kHz
//float c_resampler::g[8] = {
//	0.700196027755737f,
//	0.610604345798492f,
//	0.430021554231644f,
//	0.216706827282906f,
//	0.0754440650343895f,
//	0.00240386393852532f,
//	0.00667608808726072f,
//	0.00615068571642041f
//};
//float c_resampler::b2[8] = {
//	-1.93192768096924f,
//	-1.91813635826111f,
//	-1.88090300559998f,
//	-1.74616909027100f,
//	-0.604862213134766f,
//	-1.93690979480743f,
//	2.0f,
//	2.0f
//};
//float c_resampler::a2[8] = { 
//	-1.93866455554962f,
//	-1.92644941806793f,
//	-1.91097700595856f,
//	-1.89396166801453f,
//	-1.88183104991913f,
//	-1.94914519786835f,
//	-1.85249114036560f,
//	-1.70670175552368f
//};
//float c_resampler::a3[8] = {
//	0.980413556098938f,
//	0.961689352989197f,
//	0.936190366744995f,
//	0.907393574714661f,
//	0.886675179004669f,
//	0.994050323963165f,
//	0.879195451736450f,
//	0.731304466724396f
//};

//elliptic IIR 12th order Fs=1786840Hz, Fpass=20kHz, Fstop=24kHz, Apass=.1, Astop=106
//butterworth 4th order IIR Fs=1786840Hz, Fc=16kHz
//float c_resampler::g[8] = {
//	0.700683474540710f,
//	0.612114369869232f,
//	0.430209249258041f,
//	0.211649417877197f,
//	0.0541417300701141f,
//	0.00239885691553354f,
//	0.000774474814534187f,
//	0.000752069521695375f,
//
//};
//float c_resampler::b2[8] = {
//	-1.99237072467804f,
//	-1.99079644680023f,
//	-1.98649656772614f,
//	-1.97030615806580f,
//	-1.77672874927521f,
//	-1.99293708801270f,
//	2.0f,
//	2.0f,
//
//};
//float c_resampler::a2[8] = {
//	-1.98870587348938f,
//	-1.98303222656250f,
//	-1.97535407543182f,
//	-1.96661865711212f,
//	-1.96026897430420f,
//	-1.99298274517059f,
//	-1.95477044582367f,
//	-1.89821958541870f,
//
//};
//float c_resampler::a3[8] = {
//	0.993388116359711f,
//	0.987003862857819f,
//	0.978214025497437f,
//	0.968153297901154f,
//	0.960825324058533f,
//	0.997999370098114f,
//	0.957868397235870f,
//	0.901227831840515f,
//
//};

c_resampler::c_resampler(float m, const float *g, const float *b2, const float *a2, const float *a3)
{
	this->m = m;
	mf = 0.0f;
	sample_buf_index = 0;
	filtered_buf_index = FILTERED_BUF_LEN - 1;

	mf = m - (int)m;
	samples_required = (int)m + 2;
	lpf = new c_biquad8(g, b2, a2, a3);

	sample_buf = new short[SAMPLE_BUF_LEN];
	filtered_buf = new float[FILTERED_BUF_LEN * 2];

	memset(sample_buf, 0, sizeof(sample_buf));

	for (int i = 0; i < FILTERED_BUF_LEN * 2; i++)
		filtered_buf[i] = 0.0f;
}


c_resampler::~c_resampler()
{
	delete lpf;
	delete[] sample_buf;
	delete[] filtered_buf;
}

void c_resampler::set_m(float m)
{
	this->m = m;
}

int c_resampler::get_sample_buf(const short **sample_buf)
{
	*sample_buf = this->sample_buf;
	int x = sample_buf_index;
	sample_buf_index = 0;
	return x;
}
