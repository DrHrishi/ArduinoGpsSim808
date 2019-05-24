#include <SoftwareSerial.h>

extern uint8_t SmallFont[];

#define rxPin 50
#define txPin 51
SoftwareSerial mySerial(rxPin, txPin);
char url[] = "http://ypurUrl.com/";
char response[200];
char latitude[15];
char longitude[15];
char altitude[16];
char date[24];
char TTFF[3];
char satellites[3];
char speedOTG[10];
char course[15];

void setup()
{
    mySerial.begin(9600);
    Serial.begin(9600);
    Serial.println("Starting...");
    power_on();
    startGps();
    while (HitCommand("AT+CREG?", "+CREG: 0,1", 2000) == 0);
    HitCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 2000);
    while (HitCommand("AT+SAPBR=1,1", "OK", 20000) == 0)
    {
        delay(5000);
    }
}

void loop()
{
    get_GPS();
    send_HTTP();
}

void swithOn()
{
    uint8_t answer = 0;
    answer = HitCommand("AT", "OK", 2000);
    if (answer == 0)
    {
        while (answer == 0)
        {
            answer = HitCommand("AT", "OK", 2000);
        }
    }
}

int8_t startGps()
{
    while (HitCommand("AT+CGPSPWR=1", "OK", 2000) == 0)
        ;
    while (HitCommand("AT+CGPSRST=0", "OK", 2000) == 0)
        ;
    return 1;
}

int8_t get_GPS()
{
    int8_t answer;
    char *auxChar;
    HitCommand("AT+CGPSINF=0", "O", 8000);
    auxChar = strstr(response, "+CGPSINF:");

    if (auxChar != NULL)
    {
        memset(longitude, '\0', 15);
        memset(latitude, '\0', 15);
        memset(altitude, '\0', 16);
        memset(date, '\0', 24);
        memset(TTFF, '\0', 3);
        memset(satellites, '\0', 3);
        memset(speedOTG, '\0', 10);
        memset(course, '\0', 15);
        strcpy(response, auxChar);
        Serial.println(response);
        strtok(response, ",");
        strcpy(longitude, strtok(NULL, ","));
        strcpy(latitude, strtok(NULL, ","));
        strcpy(altitude, strtok(NULL, ","));
        strcpy(date, strtok(NULL, ","));
        strcpy(TTFF, strtok(NULL, ","));
        strcpy(satellites, strtok(NULL, ","));
        strcpy(speedOTG, strtok(NULL, ","));
        strcpy(course, strtok(NULL, "\r"));
        answer = 1;
    }
    else
        answer = 0;
    return answer;
}

void sendNMEALocation(char *cellPhoneNumber, char *message)
{
    char ctrlZString[2];
    char sendSMSString[100];
    memset(ctrlZString, '\0', 2);
    ctrlZString[0] = 26;
    memset(sendSMSString, '\0', 100);
    sprintf(sendSMSString, "AT+CMGS=\"%s\"", cellPhoneNumber);
    HitCommand(sendSMSString, ">", 2000);
    mySerial.println(message);
    HitCommand(ctrlZString, "OK", 6000);
}

int8_t send_HTTP()
{

    int8_t answer;
    char aux_str[200];
    char frame[200];
    answer = HitCommand("AT+HTTPINIT", "OK", 10000);
    if (answer == 1)
    {
        answer = HitCommand("AT+HTTPPARA=\"CID\",1", "OK", 5000);
        if (answer == 1)
        {
            memset(aux_str, '\0', 200);
            sprintf(aux_str, "AT+HTTPPARA=\"URL\",\"%s", url);
            mySerial.print(aux_str);
            Serial.println(aux_str);
            memset(frame, '\0', 200);
            sprintf(frame, "?bus_id=1&lat=%s&lon=%s&alt=%s&time=%s&TTFF=%s&sat=%s&speedOTG=%s&course=%s", latitude, longitude, altitude, date, TTFF, satellites, speedOTG, course);
            Serial.println(frame);
            mySerial.print(frame);
            answer = HitCommand("\"", "OK", 5000);
            if (answer == 1)
            {
                answer = HitCommand("AT+HTTPACTION=0", "+HTTPACTION: 0,200", 30000);
                if (answer == 1)
                {
                    Serial.println(F("Done!"));
                }
                else
                {
                    Serial.println(F("Error getting url"));
                }
            }
            else
            {
                Serial.println(F("Error setting the url"));
            }
        }
        else
        {
            Serial.println(F("Error setting the CID"));
        }
    }
    else
    {
        Serial.println(F("Error initializating"));
    }

    HitCommand("AT+HTTPTERM", "OK", 5000);
    return answer;
}

int8_t HitCommand(char *ATcommand, char *expected_answer1, unsigned int timeout)
{

    uint8_t x = 0, answer = 0;
    unsigned long previous;
    char readVar[200];
    char *auxChar;
    memset(response, '\0', 200);
    memset(readVar, '\0', 200);
    while (mySerial.available() > 0)
        mySerial.read();
    while (Serial.available() > 0)
        Serial.read();
    mySerial.write(ATcommand);
    mySerial.write("\r\n\r\n");
    Serial.println(ATcommand);
    x = 0;
    previous = millis();
    do
    {
        if (mySerial.available() != 0)
        {
            readVar[x] = mySerial.read();
            x++;
            auxChar = strstr(readVar, expected_answer1);
            if (auxChar != NULL)
            {
                if (strstr(readVar, "+CGPSINF:") == NULL)
                    strcpy(response, auxChar);
                else
                    strcpy(response, readVar);
                answer = 1;
            }
        }
    } while ((answer == 0) && ((millis() - previous) < timeout));
    if (auxChar == NULL)
        Serial.println(readVar);
    return answer;
}