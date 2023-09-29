#include <Wire.h>

#define I2C_DEV_ADDR 0x95

// // Tabelas de decodificação
unsigned char xTable[59];
const unsigned char yDecTable[] = {
    0x00, 0x07, 0x04, 0x03, 0x00,
    0x02, 0x06, 0x07, 0x07, 0x01,
    0x05, 0x00, 0x03, 0x04, 0x07
};

void createXTable(void) {xTable[5] = 0x17; xTable[6] = 0x08; xTable[7] = 0x07; xTable[9] = 0x1B; xTable[10] = 0x04; xTable[11] = 0x14; xTable[12] = 0x18; xTable[13] = 0x0C; xTable[14] = 0x1C; xTable[17] = 0x1D; xTable[18] = 0x02; xTable[19] = 0x12; xTable[20] = 0x1F; xTable[21] = 0x0A; xTable[22] = 0x1A; xTable[23] = 0x0F; xTable[24] = 0x00; xTable[25] = 0x06; xTable[26] = 0x16; xTable[27] = 0x10; xTable[28] = 0x0E; xTable[29] = 0x01; xTable[30] = 0x1E; xTable[33] = 0x1E; xTable[34] = 0x01; xTable[35] = 0x11; xTable[36] = 0x10; xTable[37] = 0x09; xTable[38] = 0x19; xTable[39] = 0x00; xTable[40] = 0x0F; xTable[41] = 0x05; xTable[42] = 0x15; xTable[43] = 0x1F; xTable[44] = 0x0D; xTable[45] = 0x02; xTable[46] = 0x1D; xTable[49] = 0x03; xTable[50] = 0x13; xTable[51] = 0x18; xTable[52] = 0x0B; xTable[53] = 0x04; xTable[54] = 0x1B; xTable[56] = 0x07; xTable[57] = 0x08; xTable[58] = 0x17;}

unsigned char decode3B4B(unsigned char data) {
  unsigned char dec;
  return yDecTable[data];
}

unsigned char decode5B6B(unsigned char data) {
  unsigned char dec;
  return xTable[data];
}

unsigned char decode8B10B(unsigned int data) {
  
  unsigned int decoded;
  unsigned char data6B, dec5B, data4B, dec3B;
  
  data6B = (data & 0x3F0) >> 4; // 6 bits mais significativos  (abcdei0000; 0x3F0 = 11 1111 0000)
  data4B = data & 0x00F; // 4 bits menos significativos (000000fghj; 0x00F = 00 0000 1111)

  dec5B = decode5B6B(data6B);  // (abcdei)
  dec3B = decode3B4B(data4B);  // (fghj)

  decoded = (unsigned int) dec3B;
  decoded = decoded << 5;             // (HGF00000)
  decoded = decoded | dec5B;          // (HGFEDCBA)
   
  return decoded;
}

void onDataReceived() 
{
  int length = Wire.available() / 2;
  unsigned int received[length];

  for(int i = 0; i < length; i++) 
  {
    received[i] = Wire.read();
    received[i] = received[i] << 8;
    received[i] = received[i] | Wire.read();
  }

  Serial.println("Encoded: ");

  for(int i = 0; i < length; i++) 
  {
    Serial.print(received[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  unsigned char decoded[length];
  char message[length];
  
  Serial.println("Decoded: ");
  for(int i = 0; i < length; i++)
  {
    decoded[i] = decode8B10B(received[i]);
    char decodedChar = decoded[i];
    Serial.print(decoded[i], DEC);
    Serial.print(" "); 
    Serial.print("(");
    Serial.print(decodedChar);
    Serial.print(")");
    Serial.print(" -- ");
    message[i] = decodedChar;
  }
  
  Serial.println("");
  Serial.println("Mensagem recebida:");
  
  for(int i = 0; i < length; i++)
  {
    Serial.print(message[i]);
  }
}

void setup()
{
  Serial.begin(9600); 
  createXTable();
  Wire.begin(I2C_DEV_ADDR);
  Wire.onReceive(onDataReceived);
}

void loop() 
{
  if (Wire.available() > 0) {
      onDataReceived();
  }

}
