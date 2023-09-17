/* ---------- Variáveis globais necessárias para a codificação ---------- */
unsigned char xTable[32], yTable[9];
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

  for (i = 0x1; bits > 0; i = i << 1, bits--) 
  {
  
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
  else 
  {
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
  flags->inv = (disparity && rd < 0) || !(disparity || rd < 0); // (disparity) XOR (rd < 0)
  flags->y7Pos = (data == 11 || data == 13 || data == 14);      // casos especiais para y = 7 e rd > 0
  flags->y7Neg = (data == 17 || data == 18 || data == 20);      // casos especiais para y = 7 e rd < 0
  
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
  
  encoded = (unsigned int)enc6B;
  encoded = encoded << 4;    // (abcdei0000)
  encoded = encoded | enc4B; // (abcdeifghj)
  
  if (bitDisparity(encoded, 10))
    rd = -rd;
  
  return encoded;
}

void setup8B10B(void) 
{
  rd = -1; // rd deve sempre começar como -1
  createEncodeTables();
}

/* ---------------------------------------------------------------------- */

unsigned int inX, inY;

void setup()
{
  
  setup8B10B();
  inX = inY = 0;
  Serial.begin(9600);
  Serial.print("    \t Y \t  X  \tCURRENT RD- \tCURRENT RD+ \t  \n");
  Serial.print("DX.Y\tHGF\tEDCBA\tabcdei\tfghj\tabcdei\tfghj\tRD-\tRD+\n");
}

unsigned int fixedInput = 0x2A;  // Fixed input value, 42

void loop() 
{
  unsigned int input, output;
  unsigned int a, f;
  char r;
  
  struct flag3B flags;

  Serial.print("Fixed Input: ");
  Serial.print(fixedInput, BIN);
  Serial.print("\t");

  // Extract 5 most significant bits (EDCBA)
  unsigned char data5B = (fixedInput >> 1) & 0x1F;

  // Extract 3 least significant bits (HGF)
  unsigned char data3B = (fixedInput & 0x7) << 5;

  // Encode 5b/6b
  unsigned char enc6B = encode5B6B(data5B, &flags);

  // Encode 3b/4b
  unsigned char enc4B = encode3B4B(data3B, flags);

  // Combine the encoded values
  output = (enc6B << 4) | enc4B;

  a = (output & 0x3F0) >> 4;
  f = output & 0xF;

  Serial.print(a, BIN);
  Serial.print('\t');
  Serial.print(f, BIN);
  Serial.print('\t');

  r = rd = 1;
  output = encode8B10B(input);

  a = (output & 0x3F0) >> 4;
  f = output & 0xF;

  Serial.print(a, BIN);
  Serial.print('\t');
  Serial.print(f, BIN);
  Serial.print('\t');

  if (r == rd)
    Serial.print("same\n");
  else
    Serial.print("flip\n");

  // Print the output
  Serial.print("Encoded Output: ");
  Serial.print(output, BIN);
  Serial.println();

  //while (1)
  //  ;
}


