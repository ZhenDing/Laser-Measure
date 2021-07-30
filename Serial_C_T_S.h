#ifndef SERIALPORT_H_  
#define SERIALPORT_H_  
#include <process.h>    
#include "TChar.h"
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <Windows.h>  
using namespace std;

class CSerialPort
{
public:
	CSerialPort(void);
	~CSerialPort(void);

public:
	//const unsigned char BDCMD[4] = { 0xFA, 0x06, 0x06, 0xFA };
	int ID = 0;
	int com = 5;
	//bool InitPort(UINT  portNo = 3, UINT  baud = CBR_19200, char  parity = 'N', UINT  databits = 8, UINT  stopsbits = 1, DWORD dwCommEvents = EV_RXCHAR);
	bool InitPort(UINT  portNo, UINT  baud, char  parity, UINT  databits, UINT  stopsbits, DWORD dwCommEvents);
	bool InitPort(UINT  portNo, const LPDCB& plDCB);
	bool OpenListenThread();
	bool CloseListenTread();
	bool WriteData(unsigned char* pData, int length);
	UINT GetBytesInCOM();
	bool ReadChar(unsigned char& cRecved);
	bool readFromComm(unsigned char* pData /*ReadCMD*/, unsigned char(&readCom)[11] /*Return of Laser*/, int length);
	bool Read_Multi_Char(unsigned char(&cRecved)[11], int length);
	bool OpenLaser(int Num);  // Num is the ID of Laser;
	bool CloseLaser(int Num);
	bool readFromComm(unsigned char(&readCom)[11] /*Return of Laser*/, int length);
	float Distance =0;
private:
	//const unsigned char ReadCMD3[4] = {0x82,0x06,0x07,0x73};
	bool openPort(UINT  portNo);
	void ClosePort();
	static UINT WINAPI ListenThread(void* pParam);
	static UINT WINAPI Listen_Laser_Thread(void* pParam);
	bool decoder(unsigned char(&cRecved)[11], float& res);
	int Read_Flag[4];
	int find_zero(int RF[4], int value);
	bool _ConMeaLaser(int Num);  // Num is the ID of Laser;
private:
	HANDLE  m_hComm;
	static bool s_bExit;
	volatile HANDLE    m_hListenThread;
	CRITICAL_SECTION   m_csCommunicationSync;
};

#endif //SERIALPORT_H_

