#include <Arduino.h>;
#include <SoftwareSerial.h>

#define DEBUG
#define PIN_DETECTOR_1 5
#define PIN_DETECTOR_2 6

SoftwareSerial sim800(7, 8);

//String modulPhoneNumber = "79236181237";
String modulPhoneNumber = "79617021794";
//String modulPhoneNumber = "79130744802";
String masterPhone = "+79130744802";
String serverIP = "2.95.83.94";//
String serverPort = "80";
String operatorGSM = "beeline";
String incomingNumber = "---";
String response = "";

int alarmSMSInterval = 600000; //Интервал между тревожными СМС
long alarmSentMoment = 0; // момент последнего СМС
int detectorStatus_1;
int detectorStatus_2;
bool isAlarm = false;
int responseTimeout = 3000;
int alarmMessageCount = 10; //Количество СМС до выключения тревоги
int alarmMessageSent = 0; //отправлено СМС

void debug(String debugStr)
{
#ifdef DEBUG
	{
		debugStr.replace("\n", "");
		debugStr.replace("\r", "");
		Serial.println(debugStr);
	}
#endif
}

void refreshAlarm()
{
	detectorStatus_1 = digitalRead(PIN_DETECTOR_1);
	detectorStatus_2 = digitalRead(PIN_DETECTOR_2);
	isAlarm = detectorStatus_1 == HIGH || detectorStatus_2 == HIGH || isAlarm;
}

String waitResponse()
{
	String _resp = "";
	long _timeout = millis() + responseTimeout;
	while (!sim800.available() && millis() < _timeout)
	{
	};
	if (sim800.available())
	{
		_resp = sim800.readString();
	}
	else
	{
		debug("Timeout...");
	}
	return _resp;
}

String sendATCommand(String cmd, bool waiting)
{
	String _resp = "";
	debug(cmd);
	sim800.println(cmd);
	if (waiting)
	{
		_resp = waitResponse();

		if (_resp.startsWith(cmd))
		{
			_resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
		}
		debug(_resp);

	}
	return _resp;
}

String prepareStatusStringForServer()
{
	int detectorStatus_1 = digitalRead(PIN_DETECTOR_1);
	int detectorStatus_2 = digitalRead(PIN_DETECTOR_2);

	String res = "";
	if (detectorStatus_1 == HIGH || detectorStatus_2 == HIGH)
	{
		res += "{";
		if (detectorStatus_1 == HIGH)
		{
			res += "" "detector1" ": true";
		}
		if (detectorStatus_2 == HIGH)
		{
			res += "" "detector2" ": true";
		}
		res += "}";
	}
	return res;
}

String prepareStatusStringForSMS()
{

	String res = "";
	if (isAlarm)
	{
		if (detectorStatus_1 == HIGH)
		{
			res += "" "detector1" ": true";
		}
		if (detectorStatus_2 == HIGH)
		{
			res += "\n" "detector2" ": true";

		}
		res += "\n ALARM!!!!!";
	}
	else
	{
		res = "it's OK";
	}
	return res;
}

bool sendStatusOnServer()
{
	String command = "guests/checkDoor";
//	String params = "?door=" + modulPhoneNumber;
	//https://kem.megafon.ru/
	String resp = sendATCommand("AT+HTTPPARA=\"USERDATA\",\"Authorization: Basic VXNlcjE6cGFzcw==\"", true);

	//AT + HTTPPARA = "CONTENT", "приложения/JSON"
	resp = sendATCommand("AT+HTTPPARA=\"URL\",\"http://" + serverIP + ":" + serverPort + "/" + command + "\"", true);

	if (resp.indexOf("OK") >= 0)
	{
		resp = sendATCommand("AT+HTTPACTION=1", true);
		if (resp.indexOf("OK") >= 0)
		{
//			resp = waitResponse();
			return true;
		}
		else
			return false;
	}
	else
		return false;
	//String resp = sendATCommand("AT+HTTPPARA=\"URL\",\"http://mysite.ru/?a=" + data + "\"",true);
	debug(resp);
}

void initGPRS()
{
	int d = 500;
	int ATsCount = 7;

	String ATs[] =
	{  //������ �� ������
			"AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "AT+SAPBR=3,1,\"APN\",\"internet." + operatorGSM + ".ru\"",
					"AT+SAPBR=3,1,\"USER\",\"" + operatorGSM + "\"", "AT+SAPBR=3,1,\"PWD\",\"" + operatorGSM + "\"",
					"AT+SAPBR=1,1", "AT+HTTPINIT", "AT+HTTPPARA=\"CID\",1" };
	int ATsDelays[] =
	{ 6, 1, 1, 1, 5, 5, 1 }; //������ ��������
	Serial.println("GPRS init start");
	for (int i = 0; i < ATsCount; i++)
	{
		Serial.println(ATs[i]);  //�������� � ������� �����
		//GSMport.println(ATs[i]);  //�������� � GSM ������
		String resp = sendATCommand(ATs[i], true);
		//delay(d * ATsDelays[i]);
		//Serial.println(ReadGSM());  //���������� ����� �� GSM ������
		debug(resp);
		//delay(d);
	}
	Serial.println("GPRS init complete");

}

void sendSMS(String phone, String message)
{

	//sendATCommand("AT+CMGS=\"" + phone + "\"", true); // Переходим в режим ввода текстового сообщения
	//sendATCommand(message + "\r\n" + (String) ((char) 26), true); // Ожидаем отправки

	//debug(message);
}

String getCommandFromSMS()
{

	sendATCommand("AT+CMGL=" "REC UNREAD" ",1", true);
	return "";
}

void sendStatusBySMS()
{
	//sendATCommand("AT+CMGS=\"" + masterPhone + "\"", true); // Переходим в режим ввода текстового сообщения
	//sendATCommand(prepareStatusStringForSMS() + "\r\n" + (String) ((char) 26), true);
	debug(prepareStatusStringForSMS());
}

void setup()
{
	sim800.begin(19200);
	Serial.begin(19200);

	pinMode(PIN_DETECTOR_1, INPUT_PULLUP);
	pinMode(PIN_DETECTOR_2, INPUT_PULLUP);

#ifdef DEBUG
	sendATCommand("ATE0", true);
	sendATCommand("AT+CMEE=2", true);  //Пусть выводит подробное описание ошибки
#endif
	sendATCommand("AT", true);
	sendATCommand("AT+CMGF=1;&W", true);
	initGPRS();
}

void loop()
{
	String response = waitResponse();
	if (Serial.available())
	{
		sim800.print(Serial.readString());
	}


	if (sim800.available())
	{
		Serial.print(sim800.readString());
		String gg = sim800.readString();

		debug(gg);
	}
	refreshAlarm();
	if (isAlarm)
	{
		if (alarmSentMoment + alarmSMSInterval < millis())
		{
			if (alarmMessageSent < alarmMessageCount)
			{
				alarmSentMoment = millis();
				sendStatusBySMS();
				alarmMessageSent++;
			}
			else
			{
				//Отправили alarmMessageCount раз СМС и выключили сирену
				isAlarm = false;
			}
		}
	}

}
