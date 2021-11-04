#pragma once
#include <cmath>
class c_pid
{
public:
	c_pid() {};
	~c_pid() {};
	double process(double val);
	double kc = 2.0;
	double Ti = 0.1;
	double Td = 0.0;
	double dv = 0.0;
private:
	double err_1 = 0.0;
	double err_2 = 0.0;
	double last_out = 0.0;
};

inline double c_pid::process(double val)
{
	double err = dv - val;
	/*if (abs(err) < 960.0)
		return 0.0;*/
	//double out = kc * ((1 + (1 / Ti) + Td) * err + (-1 - 2 * Td) * err_1 + Td * err_2);
	//double out = kc * ((err - err_1 + (1 / Ti) * err) + Td * (err - 2 * err_1 + err_2));//
	
	//https://folk.ntnu.no/skoge/puublications_others/2005_pannochio_mpc-pid_aichej.pdf
	//page 1185, algorithm 3 (pid with anti-windup)

	double out = kc * (err - err_1) + (kc / Ti) * err + (kc * Td) * (err - 2 * err_1 + err_2);

	//out = last_out + out;
	err_2 = err_1;
	err_1 = err;

	/*const double MAX = 1.0;
	if (out > MAX)
		out = MAX;
	else if (out < -MAX)
		out = -MAX;*/
	return out;
}
