/* ---------- Variáveis globais necessárias para a decodificação ---------- */
unsigned char xTable[32], yTable[9];
char rd;

struct flag3B {
  char inv, y7Neg, y7Pos;
};

void createYTable(void) {
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

void createXTable(void) {
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

void createEncodeTables(void) {
  createXTable();
  createYTable();
}

char bitDisparity(unsigned int data, unsigned char bits) {
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

unsigned char encode3B4B(unsigned char data, struct flag3B flags) {
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

unsigned char encode5B6B(unsigned char data, struct flag3B *flags) {
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

void setup8B10B(void) {
  rd = -1; // rd deve sempre começar como -1
  createEncodeTables();
}

/* ---------------------------------------------------------------------- */

unsigned int encodedData[] = {
  // Add 10-bit encoded data here
};

unsigned char decodedData[sizeof(encodedData) / sizeof(encodedData[0])];
int dataLength = sizeof(encodedData) / sizeof(encodedData[0]);

/* ---------------------------------------------------------------------- */

unsigned char decode5B6B(unsigned char data, struct flag3B *flags) {
  unsigned char decodedData;

  // Check for the special cases for y = 7 and rd > 0 or rd < 0
  if (flags->y7Pos) {
    if (rd > 0 && data == (~xTable[11] & 0x3F))
      decodedData = 11;
    else
      decodedData = 7;
  } else if (flags->y7Neg) {
    if (rd < 0 && data == (~xTable[17] & 0x3F))
      decodedData = 17;
    else
      decodedData = 7;
  } else {
    // Check for disparity and invert flag
    char disparity = bitDisparity(data, 6);
    if (flags->inv) {
      if (disparity || rd < 0)
        data = ~data & 0x3F;
    } else if (disparity && rd < 0) {
      // Handle the special case for disparity and rd < 0

    }

    // Reverse lookup in xTable to get the original data
    for (unsigned char i = 0; i < 32; i++) {
      if (xTable[i] == data) {
        decodedData = i;
        break;
      }
    }
  }

  return decodedData;
}

unsigned char decode3B4B(unsigned char data, struct flag3B flags) {
  unsigned char decodedData;

  if (data == yTable[8]) {
    if (flags.y7Neg && rd < 0)
      decodedData = 7;
    else if (flags.y7Pos && rd > 0)
      decodedData = 7;
    else
      decodedData = 0;
  } else {
    if (flags.inv)
      data = ~data & 0xF;

    // Reverse lookup in yTable to get the original data
    for (unsigned char i = 0; i < 9; i++) {
      if (yTable[i] == data) {
        decodedData = i;
        break;
      }
    }
  }

  return decodedData;
}

void decode8B10B(unsigned int encodedMessage[], unsigned char decodedMessage[], int messageLength) {
  for (int i = 0; i < messageLength; i++) {
    // Split the 10-bit block into 5b and 3b parts
    unsigned char data5B = (encodedMessage[i] >> 3) & 0x1F; // Mask out the 5b data
    unsigned char control3B = encodedMessage[i] & 0x7;       // Mask out the 3b control
    
    // Create a struct to hold the flags
    struct flag3B flags;

    // Decode the 5b data and 3b control
    unsigned char decodedData = decode5B6B(data5B, &flags);
    unsigned char decodedControl = decode3B4B(control3B, flags);
    
    // Recombine the decoded data and control into an 8-bit byte
    decodedMessage[i] = (decodedControl << 5) | decodedData;
  }
}

void setup() {
  setup8B10B();
  Serial.begin(9600);
  Serial.print("Encoded Data: ");
  for (int i = 0; i < dataLength; i++) {
    Serial.print(encodedData[i], BIN);
    Serial.print(" ");
  }
  Serial.println();
}

void loop() {
  // Decode the 8b/10b encoded message
  decode8B10B(encodedData, decodedData, dataLength);

  Serial.print("Decoded Data: ");
  for (int i = 0; i < dataLength; i++) {
    Serial.print(decodedData[i], BIN);
    Serial.print(" ");
  }
  Serial.println();

  delay(1000);
}