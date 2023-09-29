#include <Wire.h>

#define I2C_DEV_ADDR 0x55
#define TX_PIN  12

unsigned int inX, inY;
unsigned char xTable[32], yTable[9], xDecTable[58], yDecTable[14];
char rd;

struct flag3B
{
  char inv, y7Neg, y7Pos;
};

void createYTable(void)
{
  yTable[0] = 0xB;
  yTable[1] = 0x9;
  yTable[2] = 0x5;
  yTable[3] = 0xC;
  yTable[4] = 0xD;
  yTable[5] = 0xA;
  yTable[6] = 0x6;
  yTable[7] = 0xE;
  yTable[8] = 0x7;
}

void createXTable(void)
{
  xTable[0] = 0x27;
  xTable[1] = 0x1D;
  xTable[2] = 0x2D;
  xTable[3] = 0x31;
  xTable[4] = 0x35;
  xTable[5] = 0x29;
  xTable[6] = 0x19;
  xTable[7] = 0x38;
  xTable[8] = 0x39;
  xTable[9] = 0x25;
  xTable[10] = 0x15;
  xTable[11] = 0x34;
  xTable[12] = 0xD;
  xTable[13] = 0x2C;
  xTable[14] = 0x1C;
  xTable[15] = 0x17;
  xTable[16] = 0x1B;
  xTable[17] = 0x23;
  xTable[18] = 0x13;
  xTable[19] = 0x32;
  xTable[20] = 0xB;
  xTable[21] = 0x2A;
  xTable[22] = 0x1A;
  xTable[23] = 0x3A;
  xTable[24] = 0x33;
  xTable[25] = 0x26;
  xTable[26] = 0x16;
  xTable[27] = 0x36;
  xTable[28] = 0xE;
  xTable[29] = 0x2E;
  xTable[30] = 0x1E;
  xTable[31] = 0x2B;
}

void createEncodeTables(void)
{
  createXTable();
  createYTable();
}

char bitDisparity(unsigned int data, unsigned char bits)
{
  unsigned char ones = 0, zeros = 0;
  unsigned int i;

  for (i = 0x1; bits > 0; i = i << 1, bits--) {
  
    if (data & i)
      ones++;
    else
      zeros++;
  }

  return (ones != zeros);
}

unsigned char encode3B4B(unsigned char data, struct flag3B flags)
{
  unsigned char enc;
  
  if (data == 7 && flags.y7Neg && rd < 0)
    enc = yTable[8];
  else if (data == 7 && flags.y7Pos && rd > 0)
    enc = ~yTable[8] & 0xF;
  else {
    enc = yTable[data];
    if (flags.inv && (enc == 0xC || bitDisparity(enc, 4)))
      enc = ~enc & 0xF; // complemento de enc (4 bits -> 0xF = 0000 1111)
  }
 
  return enc;
}

unsigned char encode5B6B(unsigned char data, struct flag3B *flags)
{
  unsigned char enc;
  char disparity;
  
  enc = xTable[data];
  disparity = bitDisparity(enc, 6);
  
  // Flags utilizados para codificação 3B4B
  flags->inv   = (disparity && rd < 0) || !(disparity || rd < 0); // (disparity) XOR (rd < 0)
  flags->y7Pos = (data == 11 || data == 13 || data == 14);        // casos especiais para y = 7 e rd > 0 caracteres especiais K
  flags->y7Neg = (data == 17 || data == 18 || data == 20);        // casos especiais para y = 7 e rd < 0 caracteres especiais K
  
  if (rd > 0 && (enc == 0x38 || disparity))
    enc = ~enc & 0x3F; // complemento de enc (6 bits -> 0x3F = 0011 1111)
  
  return enc;
}

unsigned int encode8B10B(unsigned char data)
{
  unsigned int encoded;
  unsigned char data5B, enc6B, data3B, enc4B;
  struct flag3B flags;
  
  data5B = data & 0x1F; // 5 bits menos significativos (EDCBA; 0x1F = 0001 1111)
  data3B = data & 0xE0; // 3 bits mais significativos  (HGF;   0xE0 = 1110 0000)
  data3B = data3B >> 5;
  
  enc6B = encode5B6B(data5B, &flags); // (abcdei)
  enc4B = encode3B4B(data3B, flags);  // (fghj)
  
  encoded = (unsigned int) enc6B;
  encoded = encoded << 4;             // (abcdei0000)
  encoded = encoded | enc4B;          // (abcdeifghj)
  
  if (bitDisparity(encoded, 10))
    rd = -rd;
  
  return encoded;
}

void setup8B10B(void)
{
  rd = -1; // rd deve sempre começar como -1
  createEncodeTables();
}

void serialEvent()
{
  String message = Serial.readString();
  int length = message.length() +1;
  int total_Lenght = length;
  int leght_remaining = length;
  Serial.println("--------------------------------------------------------------------------------------------------------------");

  Serial.print("Mensagem a ser enviada:");
  Serial.print(message);
  Serial.println("Tamanho: ");
  Serial.println(length);

  unsigned char dataPreEncode[length]; 
  strcpy((char*)dataPreEncode, message.c_str());

  unsigned int dataPostEncode[length];
  
  Serial.println("Dado bufferizado antes do encode:");
  for(int i = 0; i < length; i++)
  {
    Serial.print(dataPreEncode[i]);
    Serial.print("(");
    char t = dataPreEncode[i];
    Serial.print(t);
    Serial.print(")");
    Serial.print(" -- ");
  }
  Serial.println("");

  Serial.println("Dado codificado para ser enviado:");
  for(int i = 0; i < length; i++)
  {
    dataPostEncode[i] = encode8B10B(dataPreEncode[i]);
    Serial.print(dataPostEncode[i], HEX);
    Serial.print(" -- ");
  }
  
  Serial.println("");
  Wire.beginTransmission(I2C_DEV_ADDR);
  for(int i = 0; i < length; i++){
    Wire.write(highByte(dataPostEncode[i]));
    Wire.write(lowByte(dataPostEncode[i]));
  }
  Wire.endTransmission();

  Serial.println("--------------------------------------------------------------------------------------------------------------");
}

void setup()
{
  pinMode(TX_PIN, OUTPUT);
  setup8B10B();
  inX = inY = 0;
  Serial.begin(9600);
  Serial.print("  INICIO DO ENCODER ");
  Wire.begin();
}

void loop()
{
  if (Serial.available() > 0)
    serialEvent();
}