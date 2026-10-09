String getResponse(bool wait = true) {
  static String response;
  const int MAX_RESPONSE_LENGTH = 24 + 4;
  response.reserve(MAX_RESPONSE_LENGTH);

  if (wait) while ( ! Serial.available() );
  response = Serial.readStringUntil('\n'); 
  response.trim();
  return response; 
}

String sendCommandF(const __FlashStringHelper* cmd) {
  Serial.println();
  delay(50);                                  ///+++++ 
  Serial.println(cmd);
  return getResponse(false);
}

String sendCommandPrintF(const __FlashStringHelper* cmd) {
  String response = sendCommandF(cmd);
  Serial.println(response);
  return response;
}

#define  sendCommand(cmd)       sendCommandF(F(cmd))
#define  sendCommandPrint(cmd)  sendCommandPrintF(F(cmd))

bool checkATisOK() {
  String response = sendCommand( "AT" );
  return response.startsWith(F( "+OK" ));
}
  
void printConfig_JDY_33() {
//  if ( !checkATisOK() ) return;
//  sendCommandPrint( "AT+VERSION"  );
  sendCommandPrint( "AT+NAME"  );
  sendCommandPrint( "AT+NAMB"  );
  sendCommandPrint( "AT+BAUD"  );
  sendCommandPrint( "AT+PIN"   );
  sendCommandPrint( "AT+ENLOG" );
}

bool setBaud9600()  {
  long Baud[] = {115200, 2400, 4800, 19200, 38400, 57600, 128000};
  for (int i = 0; i < 7; i++) { 
    Serial.begin(Baud[i]);
    if ( ! checkATisOK() ) continue;
    
    String result = sendCommand( "AT+BAUD4" ); 
    delay(10);
    
    Serial.begin(9600); 
    Serial.println();
    Serial.print( ">> Baudrate detected    : " );
    Serial.println( Baud[i] );
    
    Serial.print( ">> Baudrate set to 9600 : " );
    Serial.println( result );
    return true;
  }
  Serial.begin(9600); 
  return false;
}

void setup() {
  Serial.begin(9600);
  int t = 1000 ;
  
  if ( ! checkATisOK() ) {
    if ( ! setBaud9600() ) { 
      Serial.println("\r\n>> STOP : Can't find Baudrate");
      return; 
    }
  } else {
    Serial.println();
    Serial.println(">> Baudrate detected      : 9600");
  }

  sendCommandPrint("AT+DEFAULT");
  delay(t);
  sendCommandPrint("AT+RESET");
  delay(t);  

  printConfig_JDY_33();
  
  Serial.println(">> End");
}

void loop() {}