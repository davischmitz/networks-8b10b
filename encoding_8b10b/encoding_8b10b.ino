#include <Wire.h>

#define I2C_DEV_ADDR 0x55
#define TX_PIN 12

char rd;

// Tabelas de codificação
const unsigned char yTable[] = {0xB, 0x9, 0x5, 0xC, 0xD, 0xA, 0x6, 0xE, 0x7};
const unsigned char xTable[] = {0x27, 0x1D, 0x2D, 0x31, 0x35, 0x29, 0x19, 0x38, 0x39, 0x25, 0x15, 0x34, 0xD, 0x2C, 0x1C, 0x17, 0x1B, 0x23, 0x13, 0x32, 0xB, 0x2A, 0x1A, 0x3A, 0x33, 0x26, 0x16, 0x36, 0xE, 0x2E, 0x1E, 0x2B};

struct Flag3B
{
  bool inv, y7Neg, y7Pos;
};

bool bitDisparity(unsigned int data, unsigned char bits)
{
  unsigned char ones = 0, zeros = 0;
  for (unsigned int i = 1; bits > 0; i <<= 1, bits--)
  {
    if (data & i)
      ones++;
    else
      zeros++;
  }
  return (ones != zeros);
}

unsigned char encode3B4B(unsigned char data, Flag3B flags)
{
  unsigned char enc;

  if (data == 7 && flags.y7Neg && rd < 0)
    enc = yTable[8];
  else if (data == 7 && flags.y7Pos && rd > 0)
    enc = ~yTable[8] & 0xF;
  else
  {
    enc = yTable[data];
    if (flags.inv && (enc == 0xC || bitDisparity(enc, 4)))
      enc = ~enc & 0xF;
  }

  return enc;
}

unsigned char encode5B6B(unsigned char data, Flag3B &flags)
{
  unsigned char enc;
  char disparity;

  enc = xTable[data];
  disparity = bitDisparity(enc, 6);

  flags.inv = (disparity && rd < 0) || !(disparity || rd < 0); // (disparity) XOR (rd < 0)
  flags.y7Pos = (data == 11 || data == 13 || data == 14); // casos especiais para y = 7 e rd > 0 caracteres especiais K
  flags.y7Neg = (data == 17 || data == 18 || data == 20); // casos especiais para y = 7 e rd < 0 caracteres especiais K

  if (rd > 0 && (enc == 0x38 || disparity))
    enc = ~enc & 0x3F; // complemento de enc (6 bits -> 0x3F = 0011 1111)

  return enc;
}

unsigned int encode8B10B(unsigned char data)
{
  unsigned int encoded;
  unsigned char data5B, enc6B, data3B, enc4B;
  Flag3B flags;

  data5B = data & 0x1F; // 5 bits menos significativos (EDCBA; 0x1F = 0001 1111)
  data3B = (data >> 5) & 0x07; // 3 bits mais significativos  (HGF;   0xE0 = 1110 0000)

  enc6B = encode5B6B(data5B, flags); // (abcdei)
  enc4B = encode3B4B(data3B, flags); // (fghj)

  encoded = enc6B << 4; // (abcdei0000)
  encoded |= enc4B; // (abcdeifghj)

  if (bitDisparity(encoded, 10))
    rd = -rd;

  return encoded;
}

void setup8B10B()
{
  rd = -1; // rd deve sempre começar como -1
}

void serialEvent()
{
  String message = Serial.readString();
  int length = message.length() + 1;
  int total_Length = length;
  int length_remaining = length;

  Serial.println("--------------------------------------------------------------------------------------------------------------");

  Serial.print("Mensagem a ser enviada:");
  Serial.print(message);
  Serial.print(" Tamanho: ");
  Serial.println(length);

  unsigned char dataPreEncode[length];
  strcpy((char *)dataPreEncode, message.c_str());

  unsigned int dataPostEncode[length];

  Serial.println("Dado bufferizado antes do encode:");
  for (int i = 0; i < length; i++)
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
  for (int i = 0; i < length; i++)
  {
    dataPostEncode[i] = encode8B10B(dataPreEncode[i]);
    Serial.print(dataPostEncode[i], HEX);
    Serial.print(" -- ");
  }

  Serial.println("");
  Wire.beginTransmission(I2C_DEV_ADDR);
  for (int i = 0; i < length; i++)
  {
    Wire.write(highByte(dataPostEncode[i]));
    Wire.write(lowByte(dataPostEncode[i]));
  }
  Wire.endTransmission();

  Serial.println("--------------------------------------------------------------------------------------------------------------");
}

void setup()
{
  Serial.begin(9600);

  pinMode(TX_PIN, OUTPUT);

  setup8B10B();

  Wire.begin();
}

void loop()
{
  if (Serial.available() > 0)
    serialEvent();
}
