// test_variables.cpp

#include "test_variables.h"
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

Test_Variables::Test_Variables() : 
	IDENTIFIER('E'), 
	SAVEDSIGNAL(0), 
	FREQ(0),
	VERT_AMPL(0), HORIZ_AMPL_X(0), HORIZ_AMPL_Y(0),
	VERT_ALPHA(0), HORIZ_ALPHA_X(0), HORIZ_ALPHA_Y(0),
	PHASE_OFFSET(0),
	FPS_Side(0), NUMIMAGES_Side(0), INTEGER_MULTIPLE(0),
	PULSETIME(0),
	PPOD_RESET(0),
	CAM_MOVE(0),
	XPOS(0), YPOS(0), 
	DELAYTIME(0) {}

void Test_Variables::S_set(string line) {
	istringstream ss(line);
	ss >> IDENTIFIER >> SAVEDSIGNAL >> FREQ >> FPS_Side >> NUMIMAGES_Side >> INTEGER_MULTIPLE >> PULSETIME >> DELAYTIME >> PPOD_RESET >> CAM_MOVE >> XPOS >> YPOS;
};

void Test_Variables::E_set(string line) {
	istringstream ss(line);
	ss >> IDENTIFIER >> FREQ >> VERT_AMPL >> HORIZ_AMPL_X >> HORIZ_AMPL_Y >> VERT_ALPHA >> HORIZ_ALPHA_X >> HORIZ_ALPHA_Y >> PHASE_OFFSET >> FPS_Side >> NUMIMAGES_Side >> INTEGER_MULTIPLE >> PULSETIME >> DELAYTIME >> PPOD_RESET >> CAM_MOVE >> XPOS >> YPOS;
};

void Test_Variables::display_updated_parameters(string line) {
	// PPOD Saved Signal - [IDENTIFIER, SAVEDSIGNAL, FREQ, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE
	// PULSETIME, DELAYTIME, PPOD_RESET, CAM_MOVE, XPOS, YPOS]
	if (IDENTIFIER == 'S') {
		cout << "IDENTIFIER: " << IDENTIFIER << endl; 
		cout << "SAVEDSIGNAL: " << SAVEDSIGNAL << endl;
		cout << "FREQ: " << FREQ << endl;
		cout << "FPS_Side: " << FPS_Side << endl; 
		cout << "NUMIMAGES_Side: " << NUMIMAGES_Side << endl;
		cout << "INTEGER_MULTIPLE: " << INTEGER_MULTIPLE << endl;
		cout << "PULSETIME: " << PULSETIME << endl;
		cout <<	"DELAYTIME: " << DELAYTIME << endl;
		cout << "PPOD_RESET: " << PPOD_RESET << endl;
		cout << "CAM_MOVE: " << CAM_MOVE << endl;
		cout << "(XPOS, YPOS): (" << XPOS << ", " << YPOS << ")" << endl;
	}
	// PPOD Equations - [IDENTIFIER, FREQ, VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y, VERT_ALPHA, HORIZ_ALPHA_X, 
	// HORIZ_ALPHA_Y, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE, PULSETIME, DELAYTIME, PPOD_RESET, 
	// CAM_MOVE, XPOS, YPOS]
	else if (IDENTIFIER == 'E') {
		
		cout << "IDENTIFIER: " << IDENTIFIER << endl;
		cout << "FREQ: " << FREQ << endl;
		cout << "VERT_AMPL: " << VERT_AMPL << endl;
		cout << "HORIZ_AMPL_X: " << HORIZ_AMPL_X << endl;
		cout << "HORIZ_AMPL_Y: " << HORIZ_AMPL_Y << endl;
		cout << "VERT_ALPHA: " << VERT_ALPHA << endl;
		cout << "HORIZ_ALPHA_X: " << HORIZ_ALPHA_X << endl;
		cout << "HORIZ_ALPHA_Y: "<< HORIZ_ALPHA_Y << endl;
		cout << "PHASE_OFFSET: " << PHASE_OFFSET << endl;
		cout << "FPS_Side: " << FPS_Side << endl; 
		cout << "NUMIMAGES_Side: " << NUMIMAGES_Side << endl;
		cout << "INTEGER_MULTIPLE: " << INTEGER_MULTIPLE << endl;
		cout << "PULSETIME: " << PULSETIME << endl;
		cout << "DELAYTIME: " << DELAYTIME << endl;
		cout << "PPOD_RESET: " << PPOD_RESET << endl;
		cout << "CAM_MOVE: " << CAM_MOVE << endl;
		cout << "(XPOS, YPOS): (" << XPOS << ", " << YPOS << ")" << endl;
	}
};