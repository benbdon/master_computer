// file_IO.cpp

#include <iostream>
#include <fstream>
#include <string>
#include "file_IO.h"
using namespace std;

//int NUM_TESTS = 0;
// =================================================================================================================
// Check last four characters of argv[1] to ensure if a .txt file was provided
// =================================================================================================================
int file_verification(char* file_name) {
	string ext = "";
	string line;
	int CommentsInTextFile = 0;
	int LinesInTextFile = 0;
	char FirstChar;

	for (int i = strlen(file_name) - 1; i > strlen(file_name) - 5; i--) {
		ext += (file_name)[i];
	}
	if (ext == "txt.") {  //backwards .txt
		printf("First parameter is a filename.\r\n");

		ifstream myfile(file_name);

		// Count lines
		if (myfile.is_open()) {
			while (getline(myfile, line)) {

				// Increment line counter
				LinesInTextFile += 1;

				// Check to see if first character is for comments (#)				
				sscanf(line.c_str(), "%c", &FirstChar);
				if (FirstChar == '#') {
					// Increment tests counter
					CommentsInTextFile += 1;
				}
			}
		}
		else {
			cout << "Unable to open file.";
		}

		// Close file
		myfile.close();

		printf("  Lines in text file: %d\r\n", LinesInTextFile);
		printf("  Number of tests to perform: %d\r\n", (LinesInTextFile - CommentsInTextFile));
		NUM_TESTS = (LinesInTextFile - CommentsInTextFile);
	}
	return LinesInTextFile;
}

