
//#include "StdAfx.h"  
#include "Serial_C_T.h"  
/** �߳��˳���־ */
bool CSerialPort::s_bExit = false;
/** ������������ʱ,sleep���´β�ѯ�����ʱ��,��λ:�� */
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

//��ʼ�����ں���
bool CSerialPort::InitPort(UINT portNo /*= 1*/, UINT baud /*= CBR_9600*/, char parity /*= 'N'*/,
	UINT databits /*= 8*/, UINT stopsbits /*= 1*/, DWORD dwCommEvents /*= EV_RXCHAR*/)
{

	/** ��ʱ����,���ƶ�����ת��Ϊ�ַ�����ʽ,�Թ���DCB�ṹ */
	char szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	/** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */
	if (!openPort(portNo))
	{
		return false;
	}

	/** �����ٽ�� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �Ƿ��д����� */
	BOOL bIsSuccess = TRUE;

	/** �ڴ˿���������������Ļ�������С,���������,��ϵͳ������Ĭ��ֵ.
	*  �Լ����û�������Сʱ,Ҫע�������Դ�һЩ,���⻺�������
	*/
	/*if (bIsSuccess )
	{
	bIsSuccess = SetupComm(m_hComm,10,10);
	}*/

	/** ���ô��ڵĳ�ʱʱ��,����Ϊ0,��ʾ��ʹ�ó�ʱ���� */
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

			/** ��ȡ��ǰ�������ò���,���ҹ��촮��DCB���� */
		bool TempBuile = BuildCommDCB(szDCBparam, &dcb);
		bIsSuccess = GetCommState(m_hComm, &dcb) && TempBuile;
		/** ����RTS flow���� */
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		/** �ͷ��ڴ�ռ� */
		/*delete[] pwText;*/
	}

	if (bIsSuccess)
	{
		/** ʹ��DCB�������ô���״̬ */
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}

	/**  ��մ��ڻ����� */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** �뿪�ٽ�� */
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess == TRUE;
}

//��ʼ�����ں���
bool CSerialPort::InitPort(UINT portNo, const LPDCB& plDCB)
{
	/** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */
	if (!openPort(portNo))
	{
		return false;
	}

	/** �����ٽ�� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** ���ô��ڲ��� */
	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}

	/**  ��մ��ڻ����� */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** �뿪�ٽ�� */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

//�رչرմ���
void CSerialPort::ClosePort()
{
	/** ����д��ڱ��򿪣��ر��� */
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

//�򿪳�����
bool CSerialPort::openPort(UINT portNo)
{
	/** �����ٽ�� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �Ѵ��ڵı��ת��Ϊ�豸�� */
	char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/** ��ָ���Ĵ��� */
	m_hComm = CreateFileA(szPort,  /** �豸��,COM1,COM2�� */
		GENERIC_READ | GENERIC_WRITE, /** ����ģʽ,��ͬʱ��д */
		0,                            /** ����ģʽ,0��ʾ������ */
		NULL,                         /** ��ȫ������,һ��ʹ��NULL */
		OPEN_EXISTING,                /** �ò�����ʾ�豸�������,���򴴽�ʧ�� */
		0,
		0);

	/** �����ʧ�ܣ��ͷ���Դ������ */
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	/** �˳��ٽ��� */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

//�򿪼����߳�
bool CSerialPort::OpenListenThread()
{
	/** ����߳��Ƿ��Ѿ������� */
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** �߳��Ѿ����� */
		return false;
	}
	s_bExit = false;
	/** �߳�ID */
	UINT threadId;
	/** �����������ݼ����߳� */
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, Listen_Laser_Thread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** �����̵߳����ȼ�,������ͨ�߳� */
	if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		return false;
	}

	return true;
}
//�رռ����߳�
bool CSerialPort::CloseListenTread()
{
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** ֪ͨ�߳��˳� */
		s_bExit = true;

		/** �ȴ��߳��˳� */
		Sleep(10);

		/** ���߳̾����Ч */
		CloseHandle(m_hListenThread);
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}
//��ȡ���ڻ��������ֽ���
UINT CSerialPort::GetBytesInCOM()
{
	DWORD dwError = 0;  /** ������ */
	COMSTAT  comstat;   /** COMSTAT�ṹ��,��¼ͨ���豸��״̬��Ϣ */
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** �ڵ���ReadFile��WriteFile֮ǰ,ͨ�������������ǰ�����Ĵ����־ */
	if (ClearCommError(m_hComm, &dwError, &comstat))
	{
		BytesInQue = comstat.cbInQue; /** ��ȡ�����뻺�����е��ֽ��� */
	}

	return BytesInQue;
}
//���ڼ����߳�
UINT WINAPI CSerialPort::ListenThread(void* pParam)
{
	/** �õ������ָ�� */
	CSerialPort* pSerialPort = reinterpret_cast<CSerialPort*>(pParam);

	// �߳�ѭ��,��ѯ��ʽ��ȡ��������  
	while (!pSerialPort->s_bExit)
	{
		UINT BytesInQue = pSerialPort->GetBytesInCOM();
		/** ����������뻺������������,����Ϣһ���ٲ�ѯ */
		if (BytesInQue == 0)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}

		/** ��ȡ���뻺�����е����ݲ������ʾ */
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
//��ȡ���ڽ��ջ�������һ���ֽڵ�����
bool CSerialPort::ReadChar(unsigned char& cRecved)
{
	BOOL  bResult = TRUE;
	DWORD BytesRead = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** �ٽ������� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �ӻ�������ȡһ���ֽڵ����� */
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{
		/** ��ȡ������,���Ը��ݸô�����������ԭ�� */
		DWORD dwError = GetLastError();

		/** ��մ��ڻ����� */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */
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

	/** �ٽ������� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �ӻ�������ȡһ���ֽڵ����� */
	bResult = ReadFile(m_hComm, &cRecved, 11, &BytesRead, NULL);
	if ((!bResult))
	{
		/** ��ȡ������,���Ը��ݸô�����������ԭ�� */
		DWORD dwError = GetLastError();

		/** ��մ��ڻ����� */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */
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
					std::cout << "�����ղ�������" << endl;
					return 0;
				}
			}
		}
		if (BytesInQue != 11)
		{
			std::cout << "�յ���������11" << endl;
			PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
			this->WriteData(ReadCMD, 4);
		}
		if (BytesInQue == 11)
		{
			//std::cout << "�յ�����:" << BytesInQue << endl;
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
				std::cout << "��ȡʧ��" << endl;
				return 0;
			}
		}


	}




}

// �򴮿�д����, ���������е�����д�뵽����
bool CSerialPort::WriteData(unsigned char* pData, int length)
{
	int* pData1 = new int;
	BOOL   bResult = TRUE;
	DWORD  BytesToSend = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** �ٽ������� */
	EnterCriticalSection(&m_csCommunicationSync);

	/** �򻺳���д��ָ���������� */
	bResult = WriteFile(m_hComm,/*�ļ����*/pData,/*���ڱ���������ݵ�һ��������*/ length,/*Ҫ������ַ���*/ &BytesToSend,/*ָ��ʵ�ʶ�ȡ�ֽ�����ָ��*/ NULL);
	if (!bResult)
	{
		DWORD dwError = GetLastError();
		/** ��մ��ڻ����� */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */
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
	/** �õ������ָ�� */
	CSerialPort* pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	unsigned char cRecved[11];
	bool resu = 0;
	float Result;
	bool Flagg;
	if (!pSerialPort->InitPort(pSerialPort->com, CBR_9600, 'N', 8, 1, EV_RXCHAR))//�Ƿ�򿪴��ڣ�3�������������ӵ��Ե�com�ڣ��������豸�������鿴��Ȼ������������
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
	// �߳�ѭ��,��ѯ��ʽ��ȡ��������  
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
	/* �򿪼�������NumΪ��������*/
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
			std::cout << "����Error" << endl;
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
			//std::cout << "���ݣ�" << res;
			//std::cout << endl;
			return 1;
		}
	}
	else
	{
		std::cout << "�ǲ�����" << endl;
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


