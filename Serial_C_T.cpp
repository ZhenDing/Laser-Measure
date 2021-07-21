
//#include "StdAfx.h"  
#include "Serial_C_T.h"  
/** 线程退出标志 */
bool CSerialPort::s_bExit = false;
/** 当串口无数据时,sleep至下次查询间隔的时间,单位:秒 */
const UINT SLEEP_TIME_INTERVAL = 5;

CSerialPort::CSerialPort(void)
	: m_hListenThread(INVALID_HANDLE_VALUE)
{
	m_hComm = INVALID_HANDLE_VALUE;
	m_hListenThread = INVALID_HANDLE_VALUE;
	InitializeCriticalSection(&m_csCommunicationSync);
}

CSerialPort::~CSerialPort(void)
{
	CloseListenTread();
	ClosePort();
	DeleteCriticalSection(&m_csCommunicationSync);
}

//初始化串口函数
bool CSerialPort::InitPort(UINT portNo /*= 1*/, UINT baud /*= CBR_9600*/, char parity /*= 'N'*/,
	UINT databits /*= 8*/, UINT stopsbits /*= 1*/, DWORD dwCommEvents /*= EV_RXCHAR*/)
{

	/** 临时变量,将制定参数转化为字符串形式,以构造DCB结构 */
	char szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */
	if (!openPort(portNo))
	{
		return false;
	}

	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 是否有错误发生 */
	BOOL bIsSuccess = TRUE;

	/** 在此可以设置输入输出的缓冲区大小,如果不设置,则系统会设置默认值.
	*  自己设置缓冲区大小时,要注意设置稍大一些,避免缓冲区溢出
	*/
	/*if (bIsSuccess )
	{
	bIsSuccess = SetupComm(m_hComm,10,10);
	}*/

	/** 设置串口的超时时间,均设为0,表示不使用超时限制 */
	COMMTIMEOUTS  CommTimeouts;
	CommTimeouts.ReadIntervalTimeout = 0;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	CommTimeouts.ReadTotalTimeoutConstant = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant = 0;
	if (bIsSuccess)
	{
		bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);
	}

	DCB  dcb;
	if (bIsSuccess)
	{
			//DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, NULL, 0);
			//wchar_t *pwText = new wchar_t[dwNum];
			//if (!MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
			//{
			//	bIsSuccess = TRUE;
			//}

			/** 获取当前串口配置参数,并且构造串口DCB参数 */
		bool TempBuile = BuildCommDCB(szDCBparam, &dcb);
		bIsSuccess = GetCommState(m_hComm, &dcb) && TempBuile;
		/** 开启RTS flow控制 */
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		/** 释放内存空间 */
		/*delete[] pwText;*/
	}

	if (bIsSuccess)
	{
		/** 使用DCB参数配置串口状态 */
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}

	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** 离开临界段 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess == TRUE;
}

//初始化串口函数
bool CSerialPort::InitPort(UINT portNo, const LPDCB& plDCB)
{
	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */
	if (!openPort(portNo))
	{
		return false;
	}

	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 配置串口参数 */
	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}

	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** 离开临界段 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

//关闭关闭串口
void CSerialPort::ClosePort()
{
	/** 如果有串口被打开，关闭它 */
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

//打开出串口
bool CSerialPort::openPort(UINT portNo)
{
	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 把串口的编号转换为设备名 */
	char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/** 打开指定的串口 */
	m_hComm = CreateFileA(szPort,  /** 设备名,COM1,COM2等 */
		GENERIC_READ | GENERIC_WRITE, /** 访问模式,可同时读写 */
		0,                            /** 共享模式,0表示不共享 */
		NULL,                         /** 安全性设置,一般使用NULL */
		OPEN_EXISTING,                /** 该参数表示设备必须存在,否则创建失败 */
		0,
		0);

	/** 如果打开失败，释放资源并返回 */
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	/** 退出临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

//打开监听线程
bool CSerialPort::OpenListenThread()
{
	/** 检测线程是否已经开启了 */
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** 线程已经开启 */
		return false;
	}
	s_bExit = false;
	/** 线程ID */
	UINT threadId;
	/** 开启串口数据监听线程 */
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, Listen_Laser_Thread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** 设置线程的优先级,高于普通线程 */
	if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		return false;
	}

	return true;
}
//关闭监听线程
bool CSerialPort::CloseListenTread()
{
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** 通知线程退出 */
		s_bExit = true;

		/** 等待线程退出 */
		Sleep(10);

		/** 置线程句柄无效 */
		CloseHandle(m_hListenThread);
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}
//获取串口缓冲区的字节数
UINT CSerialPort::GetBytesInCOM()
{
	DWORD dwError = 0;  /** 错误码 */
	COMSTAT  comstat;   /** COMSTAT结构体,记录通信设备的状态信息 */
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** 在调用ReadFile和WriteFile之前,通过本函数清除以前遗留的错误标志 */
	if (ClearCommError(m_hComm, &dwError, &comstat))
	{
		BytesInQue = comstat.cbInQue; /** 获取在输入缓冲区中的字节数 */
	}

	return BytesInQue;
}
//串口监听线程
UINT WINAPI CSerialPort::ListenThread(void* pParam)
{
	/** 得到本类的指针 */
	CSerialPort* pSerialPort = reinterpret_cast<CSerialPort*>(pParam);

	// 线程循环,轮询方式读取串口数据  
	while (!pSerialPort->s_bExit)
	{
		UINT BytesInQue = pSerialPort->GetBytesInCOM();
		/** 如果串口输入缓冲区中无数据,则休息一会再查询 */
		if (BytesInQue == 0)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}

		/** 读取输入缓冲区中的数据并输出显示 */
		unsigned char cRecved = 0x00;
		do
		{
			cRecved = 0x00;
			if (pSerialPort->ReadChar(cRecved) == true)
			{

				std::stringstream  ss;
				int tm = cRecved;
				ss << std::hex << std::setw(2) << std::setfill('0') << tm;
				ss << " ";
				string a = ss.str();
				string b;
				transform(a.begin(), a.end(), back_inserter(b), ::toupper);
				cout << b;
				continue;
			}
		} while (--BytesInQue);
	}

	return 0;
}
//读取串口接收缓冲区中一个字节的数据
bool CSerialPort::ReadChar(unsigned char& cRecved)
{
	BOOL  bResult = TRUE;
	DWORD BytesRead = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** 临界区保护 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 从缓冲区读取一个字节的数据 */
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{
		/** 获取错误码,可以根据该错误码查出错误原因 */
		DWORD dwError = GetLastError();

		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return (BytesRead == 1);

}

bool CSerialPort::Read_Multi_Char(unsigned char (&cRecved)[11],int length)
{
	BOOL  bResult = TRUE;
	DWORD BytesRead = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** 临界区保护 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 从缓冲区读取一个字节的数据 */
	bResult = ReadFile(m_hComm, &cRecved, 11, &BytesRead, NULL);
	if ((!bResult))
	{
		/** 获取错误码,可以根据该错误码查出错误原因 */
		DWORD dwError = GetLastError();

		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);
	//std::cout << cRecved;
	//return (BytesRead == 1);
	return 1;
}

bool CSerialPort::readFromComm(unsigned char* pData /*ReadCMD*/, unsigned char(&readCom)[11], int length)
{
	this->WriteData(pData,4);
	std::cout << "Process IN" << endl;
	Sleep(100);
	bool Flag = 1;
	UINT BytesInQue = this->GetBytesInCOM();
	int count=0;
	while (BytesInQue == 0  && Flag==1)
	{
		//std::cout << "Process IN,No byte in buff" << endl;
		Sleep(100);
		this->WriteData(pData, 4);
		count++;
		if (count > 20)
		{
			return 0;
		}
	}
	std::cout << "Process if buff null IN:" << BytesInQue<< endl;
	unsigned char cRecved[11] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	if (this->Read_Multi_Char(cRecved, 11) == true)
	{
		std::cout << "Process Data1:";
		for (int i = 0; i < 11; i++)
		{
			readCom[i] = cRecved[i];
			//int tempp = cRecved[i];
			//std::cout << tempp;
		}
		//std::cout << endl;
	}
	else
	{
		std::cout << "" << endl;
		//PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		return 0;
	}
	std::cout << "Process Out" << endl;
	return 1;
}

bool CSerialPort::readFromComm(int ID /*ID of laser*/, unsigned char(&readCom)[11] /*Return of Laser*/, int length)
{
	//std::cout << "readFromComm In" << endl;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	//unsigned char ReadCMD[4] = {0x80, 0x06,0x07,0x73}
	unsigned char* ReadCMD = new unsigned char[4];//
	ReadCMD[0] = 0x80;
	ReadCMD[1] = 0x06;
	ReadCMD[2] = 0x07;
	ReadCMD[3] = 0x73;
	byte IID = 0x80;
	byte Chec = 0x73;
	IID = IID + ID;
	Chec = Chec - ID;
	ReadCMD[0] = IID;
	ReadCMD[3] = Chec;
	this->WriteData(ReadCMD, 4);
	//std::cout << "Process IN" << endl;
	Sleep(100);
	bool Flag = 1;
	int count = 0;
	while (Flag == 1)
	{
		UINT BytesInQue = this->GetBytesInCOM();
		if (BytesInQue == 0)
		{
			while (BytesInQue == 0)
			{
				//std::cout << "Process IN,No byte in buff" << endl;
				count++;
				Sleep(100);
				if (count % 5 == 0)
				{
					this->WriteData(ReadCMD, 4);
				}
				BytesInQue = this->GetBytesInCOM();
				if (count > 20)
				{
					std::cout << "持续收不到数据" << endl;
					return 0;
				}
			}
		}
		if (BytesInQue != 11)
		{
			std::cout << "收到的数不是11" << endl;
			PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
			this->WriteData(ReadCMD, 4);
		}
		if (BytesInQue == 11)
		{
			//std::cout << "收到的数:" << BytesInQue << endl;
			unsigned char cRecved[11] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
			if (this->Read_Multi_Char(cRecved, 11) == true)
			{
				std::cout << "Process Data1:";
				for (int i = 0; i < 11; i++)
				{
					readCom[i] = cRecved[i];
					//int tempp = cRecved[i];
					//std::cout << tempp;
				}
				//std::cout << endl;
				//std::cout << "Process Out" << endl;
				return 1;
			}
			else
			{
				std::cout << "读取失败" << endl;
				return 0;
			}
		}


	}




}

// 向串口写数据, 将缓冲区中的数据写入到串口
bool CSerialPort::WriteData(unsigned char* pData, int length)
{
	int* pData1 = new int;
	BOOL   bResult = TRUE;
	DWORD  BytesToSend = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** 临界区保护 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 向缓冲区写入指定量的数据 */
	bResult = WriteFile(m_hComm,/*文件句柄*/pData,/*用于保存读入数据的一个缓冲区*/ length,/*要读入的字符数*/ &BytesToSend,/*指向实际读取字节数的指针*/ NULL);
	if (!bResult)
	{
		DWORD dwError = GetLastError();
		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

UINT WINAPI CSerialPort::Listen_Laser_Thread(void* pParam)
{
	unsigned char* BDCMD = new unsigned char[4];
	BDCMD[0] = 0xFA;
	BDCMD[1] = 0x06;
	BDCMD[2] = 0x06;
	BDCMD[3] = 0xFA;
	/** 得到本类的指针 */
	CSerialPort* pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	unsigned char cRecved[11];
	bool resu = 0;
	float Result;
	bool Flagg;
	if (!pSerialPort->InitPort(pSerialPort->com, CBR_9600, 'N', 8, 1, EV_RXCHAR))//是否打开串口，3就是你外设连接电脑的com口，可以在设备管理器查看，然后更改这个参数
	{
		std::cout << "initPort fail !" << std::endl;
	}
	else
	{
		std::cout << "initPort success !" << std::endl;
	}
	Sleep(10000);
	std::cout << "open 0" << endl;
	//std::cin.get();
	pSerialPort->OpenLaser(0);
	Sleep(3000);
	std::cout << "open 1" << endl;
	//std::cin.get();
	pSerialPort->OpenLaser(1);
	Sleep(3000);
	std::cout << "open 2" << endl;
	//std::cin.get();
	pSerialPort->OpenLaser(2);
	Sleep(3000);
	std::cout << "open 3" << endl;
	//std::cin.get();
	pSerialPort->OpenLaser(3);
	Sleep(3000);
	// 线程循环,轮询方式读取串口数据  
	while (!pSerialPort->s_bExit)
	{  
		Flagg = 1;
		pSerialPort->WriteData(BDCMD, 5);
		Sleep(100);
		while (Flagg)
		{
			int Pos = pSerialPort->find_zero(pSerialPort->Read_Flag, 0);
			if (Pos == -1)
			{
				Flagg = 0;
				memset(pSerialPort->Read_Flag,0,4);
				std::cout << pSerialPort->Read_Flag << endl;
			}
			else
			{
				resu = pSerialPort->readFromComm(Pos, cRecved, 4);
				if (resu == 1)
				{
					bool res_de = pSerialPort->decoder(cRecved, Result);
					if (res_de)
					{
						int read_id = int(cRecved[0]) - 48;
						pSerialPort->Distance[read_id] = Result;
						pSerialPort->Read_Flag[read_id] = 1;
					}
					std::cout << Result << endl;
				}

			}



		}

	}
	return 0;
}

bool CSerialPort::OpenLaser(int Num)
{
	/* 打开激光器，Num为激光器号*/
	unsigned char OpenCMD[5] = { 0x80,0x06,0x05,0x01,0x74};
	byte IID = 0x80;
	byte Chec = 0x74;
	IID = IID + Num;
	Chec = Chec - Num;
	OpenCMD[0] = IID;
	OpenCMD[4] = Chec;
	this->WriteData(OpenCMD,5);
	return 1;
}
bool CSerialPort::CloseLaser(int Num) 
{
	unsigned char CloseCMD[5] = { 0x80,0x06,0x05,0x00,0x75 };
	byte IID = 0x80;
	byte Chec = 0x75;
	IID = IID + Num;
	Chec = Chec - Num;
	CloseCMD[0] = IID;
	CloseCMD[4] = Chec;
	this->WriteData(CloseCMD, 5);
	return 1;
}
bool CSerialPort::decoder(unsigned char(&cRecved)[11],float& res)
{
	if (int(cRecved[1]) == 6 && int(cRecved[2]) == 130)
	{
		if (int(cRecved[3]) > 57)
		{
			std::cout << "测量Error" << endl;
			return 0;
		}
		else
		{
			int Baiwei = int(cRecved[3]) - 48;
			int Shiwei= int(cRecved[4]) - 48;
			int Gewei = int(cRecved[5]) - 48;
			int DotGewei = int(cRecved[7]) - 48;
			int DotShiwei = int(cRecved[8]) - 48;
			int DotBaiwei = int(cRecved[9]) - 48;
			res = Baiwei * 100 + Shiwei * 10 + Gewei + DotGewei * 0.1 + DotShiwei * 0.01 + DotBaiwei * 0.001;
			//std::cout << "Baiwei:" << Baiwei<<";";
			//std::cout << "Shiwei:" << Shiwei<<";";
			//std::cout << "Gewei:" << Gewei << ";";
			//std::cout << "DotGewei:" << DotGewei << ";";
			//std::cout << "DotShiwei:" << DotShiwei << ";";
			//std::cout << "DotBaiwei:" << DotBaiwei << ";";
			//std::cout << "数据：" << res;
			//std::cout << endl;
			return 1;
		}
	}
	else
	{
		std::cout << "非测量码" << endl;
		return 0;
	}

}
int CSerialPort::find_zero(int RF[4],int value)
{
	for (int ia = 0; ia < 4; ia++)
	{
		if (RF[ia] == value)
		{
			return ia;
		}
	}
	return -1;
}


