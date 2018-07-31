#pragma once
#include <string>
using namespace std;

HANDLE ConfigureSerialPort(HANDLE m_hSerialComm, const LPCTSTR m_pszPortName);

string ReadSerialPort(HANDLE m_hSerialComm);

void WriteSerialPort(HANDLE m_hSerialComm, char message[]);

void CloseSerialPort(HANDLE m_hSerialComm);

HANDLE ConfigureSerialPortGRBL(HANDLE m_hSerialComm, const LPCTSTR m_pszPortName);

void Home(HANDLE m_hSerialCommGRBL, char GCODEmessage[]);

void PositionAbsolute(HANDLE m_hSerialCommGRBL, char GCODEmessage[], int Xpos, int Ypos);

void PositionRelative(HANDLE m_hSerialCommGRBL, char GCODEmessage[], int Xpos, int Ypos);

