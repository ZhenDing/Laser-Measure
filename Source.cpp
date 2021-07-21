//#include "StdAfx.h"  
#include "Serial_C_T.h" 
int main(int argc, _TCHAR* argv[])
{
	CSerialPort mySerialPort;//首先将之前定义的类实例化
	int length = 8;//定义传输的长度
	unsigned char* temp = new unsigned char[8];//动态创建一个数组
	//unsigned char* ReadCMD = new unsigned char[4];//
	char recvBuf[11] = { 0 };

	if (!mySerialPort.InitPort(9, CBR_9600, 'N', 8, 1, EV_RXCHAR))//是否打开串口，3就是你外设连接电脑的com口，可以在设备管理器查看，然后更改这个参数
	{
		std::cout << "initPort fail !" << std::endl;
	}
	else
	{
		std::cout << "initPort success !" << std::endl;
	}


	//std::cout << mySerialPort.WriteData(BDCMD, 5) << "BD Command" << endl;
	//unsigned char cRecved[11];
	while (1)
	{
	std::cout << "open 0" << endl;
	std::cin.get();
	mySerialPort.OpenLaser(0);
	std::cout << "open 1" << endl;
	std::cin.get();
	mySerialPort.OpenLaser(1);
	////	//std::cout << "Close 0" << endl;
	//	//std::cin.get();
	//	//mySerialPort.CloseLaser(0);
	//	//std::cout << "close 1" << endl;
	//	//mySerialPort.CloseLaser(1);
	//	bool resu=0;
	//	while (true)
	//	{
	//		std::cout << "Read 0，";
	//		std::cout << mySerialPort.WriteData(BDCMD, 5) << "BD Command："<<endl;
	//		Sleep(100);
	//		resu=mySerialPort.readFromComm(0, cRecved, 4);
	//		if (resu == 1)
	//		{
	//			std::cout << "Process Data2:";
	//			for (int ii = 0; ii < 11; ii++)
	//			{
	//				int tm = cRecved[ii];
	//				std::cout << tm;
	//			}
	//			std::cout << endl;

	//		}


	//	}


	}
	if (!mySerialPort.OpenListenThread())//是否打开监听线程，开启线程用来传输返回值
	{
		std::cout << "OpenListenThread fail !" << std::endl;
	}
	else
	{
		std::cout << "OpenListenThread success !" << std::endl;
	}

	while (1)
	{
		Sleep(1000);
	}

	temp[0] = 0x80;
	temp[1] = 0x06;
	temp[2] = 0x85;
	temp[3] = 0x01;
	temp[4] = 0x74;



	//ReadCMD[0] = 0x80;
	//ReadCMD[1] = 0x06;
	//ReadCMD[2] = 0x07;
	//ReadCMD[3] = 0x73;

	//std::cout << mySerialPort.WriteData(BDCMD, 5) << "BD Command" << endl;
	//unsigned char cRecved[11];
	while (1)
	{
		//std::cout << mySerialPort.WriteData(BDCMD, 4) << "BD Command" << endl;
		std::cout << "Main In" << endl;

		Sleep(100);
		//mySerialPort.readFromComm(ReadCMD,cRecved, 4);
		std::cout << "Main process Out" << endl;

		std::cout << "Process Data2:";
		for (int ii = 0; ii < 11; ii++)
		{ 
			//int tm = cRecved[ii];
			//std::cout << tm;
		}
		std::cout<< endl;
	}
	//	std::cout << mySerialPort.WriteData(ReadCMD, 4) <<"READ Command"<< endl;
	//	Sleep(100);

	//	UINT BytesInQue = mySerialPort.GetBytesInCOM();
	//	//std::cout << "numeber in Que:" << BytesInQue << endl;
	//	if (BytesInQue != 0)
	//		{
	//			std::cout << mySerialPort.GetBytesInCOM() << endl;//这个函数就是显示返回值函数
	//			//cout << BytesInQue << endl;//这个函数就是显示返回值函数
	//			unsigned char cRecved = 0x00;
	//			do
	//			{
	//				cRecved = 0x00;
	//				if (mySerialPort.ReadChar(cRecved) == true)
	//				{

	//					std::stringstream  ss;
	//					int tm = cRecved;
	//					ss << std::hex << std::setw(2) << std::setfill('0') << tm;
	//					ss << " ";
	//					string a = ss.str();
	//					string b;
	//					transform(a.begin(), a.end(), back_inserter(b), ::toupper);
	//					std::cout <<b;
	//					continue;
	//				}
	//			} while (--BytesInQue);
	//			std::cout << endl;
	//		}		
	//		else
	//		{
	//			Sleep(1000);
	//		}
	//}
	////std::cout << mySerialPort.WriteData(temp, 5) << endl;//这个函数就是给串口发送数据的函数，temp就是要发送的数组。

	////}
	////delete temp[];
	////system("pause");
	return 0;
}