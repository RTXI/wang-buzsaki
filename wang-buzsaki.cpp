/*
* Copyright (C) 2011 Georgia Institute of Technology
*
*  This program is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public License as
*  published by the Free Software Foundation; either version 2 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Risa Lin
// Tue 12 Apr 2011 11:08:18 AM EDT


// Wang Buzsaki model modified by Sharon Norman to have parameters not given in
// units/cm^2. April 19, 2011
// Cm = 1 nF, so the surface area of the cell is 0.001 cm^2, because 1 nF = SA*(1 uF/cm^2)
// conductances in uS, current in nA, voltage in mV

// Mirrors the WangBuzsaki plugin that Risa made fairly well - looks to be accurate


#include <wang-buzsaki.h>
#include <math.h>

// Model Functions

static inline double alpha_m(double V) {
	double x = -(V + 35.0);
	double y = 10.0;
	
	if (fabs(x / y) < 1e-6)
		return 0.1 * y * (1 - x / y / 2.0);
	else
	return 0.1 * x / (exp(x / y) - 1.0);
}

static inline double beta_m(double V) {
	return 4 * exp(-0.0556 * (V + 60));
}

static inline double m_inf(double V) {
	return alpha_m(V) / (alpha_m(V) + beta_m(V));
}

static inline double alpha_h(double V) {
	return 0.07 * exp(-0.05 * (V + 58.0));
}

static inline double beta_h(double V) {
	return 1.0 / (1.0 + exp(-0.1 * (V + 28)));
}

static inline double alpha_n(double V) {
	double x = -(V + 34);
	double y = 10.0;
	
	if (fabs(x / y) < 1e-6)
		return 0.01 * y * (1 - x / y / 2.0);
	else
	return 0.01 * x / (exp(x / y) - 1.0);
}

static inline double beta_n(double V) {
	return 0.125 * exp(-0.0125 * (V + 44));
}

extern "C" Plugin::Object *createRTXIPlugin(void) {
	return new wbscaled();
}

static DefaultGUIModel::variable_t vars[] = 
{
	{ "Vm", "Membrane Potential (V)", DefaultGUIModel::OUTPUT, },
	{ "Istim", "Input current (A)", DefaultGUIModel::INPUT, },
	{ "Iapp (nA)", "Applied Current (nA)",
		DefaultGUIModel::PARAMETER, },
	{ "V0 (mV)", "Initial membrane potential (mV)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Cm (nF)", "Specific membrane capacitance (nF)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "G_Na_max (uS)", "Maximum Na+ conductance density (uS)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "E_Na (mV)", "Sodium reversal potential (mV)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "G_K_max (uS)",
		"Maximum delayed rectifier conductance density (uS)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "E_K (mV)", "K+ reversal potential (mV)", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "G_L (uS)", "Maximum leak conductance density uS",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "E_L (mV)", "Leak reversal potential (mV)",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Rate (Hz)", "Rate of integration (Hz)", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::UINTEGER, },
	{ "h", "Sodium Inactivation", DefaultGUIModel::STATE, },
	{ "n", "Potassium Activation", DefaultGUIModel::STATE, },
	{ "Time (s)", "Time (s)", DefaultGUIModel::STATE, }, };

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

/*
* Macros for making the code below a little bit cleaner.
*/

#define V (y[0])
#define h (y[1])
#define n (y[2])
#define dV (dydt[0])
#define dh (dydt[1])
#define dn (dydt[2])
#define G_Na (G_Na_max*m_inf(V)*m_inf(V)*m_inf(V)*h)
#define G_K  (G_K_max*n*n*n*n)


wbscaled::wbscaled(void) : DefaultGUIModel("Wang Buzsaki - scaled", ::vars, ::num_vars)
{
	initParameters();
	createGUI(vars, num_vars);
	update(INIT);
	refresh();
	resizeMe();
}

wbscaled::~wbscaled(void)
{
}

void wbscaled::execute(void) {
	systime = count * period; // time in seconds for display
	for (int i = 0; i < steps; ++i)
		solve(period / steps * 1000, y); // period in ms
	output(0) = V*1e-3; // convert to V
	count++;
}

void wbscaled::update(DefaultGUIModel::update_flags_t flag) {
	switch (flag) {
		case INIT:
			setParameter("V0 (mV)", QString::number(V0)); // initialized in mV, display in mV
			setParameter("Cm (nF)", QString::number(Cm));
			setParameter("G_Na_max (uS)", QString::number(G_Na_max));
			setParameter("E_Na (mV)", QString::number(E_Na)); // initialized in mV, display in mV
			setParameter("G_K_max (uS)", QString::number(G_K_max));
			setParameter("E_K (mV)", QString::number(E_K)); // initialized in mV, display in mV
			setParameter("G_L (uS)", QString::number(G_L));
			setParameter("E_L (mV)", QString::number(E_L)); // initialized in mV, display in mV
			setParameter("Iapp (nA)", QString::number(Iapp));
			setParameter("Rate (Hz)", rate);
			setState("h", h);
			setState("n", n);
			setState("Time (s)", systime);
			break;

		case MODIFY:
			V0 = getParameter("V0 (mV)").toDouble();
			Cm = getParameter("Cm (nF)").toDouble();
			G_Na_max = getParameter("G_Na_max (uS)").toDouble();
			E_Na = getParameter("E_Na (mV)").toDouble();
			G_K_max = getParameter("G_K_max (uS)").toDouble();
			E_K = getParameter("E_K (mV)").toDouble();
			G_L = getParameter("G_L (uS)").toDouble();
			E_L = getParameter("E_L (mV)").toDouble();
			Iapp = getParameter("Iapp (nA)").toDouble();
			rate = getParameter("Rate (Hz)").toDouble();
			steps = static_cast<int> (ceil(period * rate));
			V = V0;
			h = 0.9379;
			n = 0.1224;
			break;

		case PERIOD:
			period = RT::System::getInstance()->getPeriod() * 1e-9; // time in s
			steps = static_cast<int> (ceil(period * rate));
			break;

		default:
			break;
	}
}

void wbscaled::initParameters() {
	V0 = -55.0456; // mV
	Cm = 1;
	Iapp = 1;
	G_Na_max = 35;
	E_Na = 55.0; // mV
	phi = 5;
	G_K_max = 9;
	E_K = -90.0;
	G_L = 0.1;
	E_L = -65.0;
	rate = 40000;
	
	V = V0;
	h = 0.9379;
	n = 0.1224;
	count = 0;
	systime = 0;
	period = RT::System::getInstance()->getPeriod() * 1e-9; // time in s
	steps = static_cast<int> (ceil(period * rate)); // calculate how many integrations to perform per execution step
}

void wbscaled::solve(double dt, double *y) {
	double dydt[3];
	derivs(y, dydt);
	for (size_t i = 0; i < 3; ++i)
		y[i] += dt * dydt[i];
}

void wbscaled::derivs(double *y, double *dydt) {
	dV = (Iapp + input(0) * 1e9 - G_Na * (V - E_Na) - G_K * (V - E_K) - G_L * (V
	- E_L)) / Cm;
	dh = phi * (alpha_h(V) * (1-h) - beta_h(V) * h);
	dn = phi * (alpha_n(V) * (1-n) - beta_n(V) * n);
}


