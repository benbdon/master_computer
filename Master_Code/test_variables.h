// test_variables.h
#pragma once
#include <string>
using namespace std;

class Test_Variables {
public:
	Test_Variables();
	void E_set(string current_line);
	void S_set(string current_line);
	void display_updated_parameters(string current_line);
	char IDENTIFIER;
	int SAVEDSIGNAL,
		FREQ,
		VERT_AMPL, HORIZ_AMPL_X, HORIZ_AMPL_Y,
		VERT_ALPHA, HORIZ_ALPHA_X, HORIZ_ALPHA_Y,
		PHASE_OFFSET,
		FPS_Side, NUMIMAGES_Side, INTEGER_MULTIPLE,
		PULSETIME,
		PPOD_RESET,
		CAM_MOVE,
		XPOS, YPOS;
	float DELAYTIME;
};
// 1. IDENTIFIER - S & E - S for saved signal, E for PPOD equation
// 2. SAVED SIGNAL - Just S - Determines which saved signal is run
// 3. FREQ - S & E - PPOD vibration frequency in HZ
// 4. VERT_AMPL - Just E - Z axis accelerations of PPOD table in m/s^2
// 5. HORIZ_AMPL_X - Just E - X axis acceleration of PPOD table in m/s^2
// 6. HORIZ_AMPL_Y - Just E - Y axis acceleration of PPOD table in m/s^2
// 7. VERT_ALPHA - Just E - Amplitude of frequency rotating surface about Z axis
// 8. HORIZ_ALPHA_X - Just E - Amplitude of the frequency rotating PPOD about gantry x axis
// 9. HORIZ_ALPHA_Y - Just E - Amplitude of frequency roating PPOD about gantry y axis
// 10. PHASE_OFFSET - Just E - Decouples horizontal and vertical frequencies for adjusting bouncing behavior (0 - 360)
// 11. FPS_Side - S & E - Frame rate of Mikrotron (side) camera in frames per second
// 12. NUMIMAGES_Side - S & E - Number of frames the side camera will capture in a given experiment
// 13. INTEGER_MULTIPLE - S & E - Determines the frame rate for the TrackCam (overhead)
// 14. PULSETIME - S & E - Pulse time for piezeo in droplet generator in microseconds (680-900)
// 15. DELAYTIME - S & E - Delay from when cameras start capture to when the droplet is made in seconds (0.000 - 0.0500)
// 16. PPOD_RESET - S & E - Use to turn off PPOD after a given experiment completes
// 17. Camera_Move - S & E - (1) Moves the cam out of the cam frame (0) doesn't move the camera
// 18. XPOS - S & E - Location of the droplet in x axis relative to the center of the bath (-15 - 15)
// 19. YPOS - S & E - Location of the droplet in y axis relative to the center of the bath (-15 - 15)
