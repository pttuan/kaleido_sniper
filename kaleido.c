// By Tuan Phan <pttuan@gmailcom>

#include "BluetoothSerial.h"

#ifdef USE_PIN
const char *pin = "1234";
#endif

String device_name = "Kaleido-M2";

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

//kaleido global variables
int mState, mStatePad, mNum, mNumPad, mTimeout;
int mKey, mKaleidoPad;
char mArray[200];
char mArrayPad[32];
int mETmp, mBTmp;
int mDrum, mSmoke, mPower, mSV;
int mInitDelay;

//artisan global variables
char mBTCmd[64];
int mBTCur;

void setup() {
  Serial.begin(115200);
  Serial1.begin(57600, SERIAL_8N1, 25, 26);
  Serial2.begin(57600, SERIAL_8N1, 16, 17);
  SerialBT.begin(device_name); //Bluetooth device name
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n\r", device_name.c_str());
  //Serial.printf("The device with name \"%s\" and MAC address %s is started.\nNow you can pair it with Bluetooth!\n\r", device_name.c_str(), SerialBT.getMacString()); // Use this after the MAC method is implemented
  #ifdef USE_PIN
    SerialBT.setPin(pin);
    Serial.printf("Using PIN");
  #endif

  // global setup
  mState = mStatePad = 0;
  mNum = mNumPad = 0;
  mTimeout = 0;
  mKaleidoPad = 0;
  mETmp = mBTmp = mPower = mSmoke = mDrum = mSV = 0;
  
  // artisan setup
  mBTCur = 0;
}

/* For catching communication between Kaleido and its pad */
// void snipper() {
//   int avail;
//   int c, flag = 0;
//   avail = Serial2.available();
//   if (avail > 0) {
//     Serial.printf("Serial2:\n\r");
//     flag = 1;
//   }
//   while (avail--) {
//     c = Serial2.read();
//     Serial1.write(c);
//     Serial.printf("%02x ", c);
//   }
//   if (flag) {
//     Serial.printf("\n\r");
//   }
//   flag = 0;
//   avail = Serial1.available();
//   if (avail > 0) {
//     Serial.printf("Serial1:\n\r");
//     flag = 1;
//   }
//   while (avail--) {
//     c = Serial1.read();
//     Serial2.write(c);
//     Serial.printf("%02x ", c);
//   }
//   if (flag) {
//     Serial.printf("\n\r");
//   }
//   delay(20);
// }

void kaleido_send(char array[], int len)
{
  Serial.printf("Sending to Kaleido:\n\r");
  for (int i = 0; i < len; i++) {
    Serial2.write(array[i]);
    Serial.printf("%02x ", array[i]);
  }
  Serial.printf("\n\r");
}

void kaleido_send_encode(char array[], int len)
{
  for (int i = 0; i < len; i++) {
    array[i] = array[i] ^ mKey;
  }
  kaleido_send(array, len);
}

void kaleido_pad_send_char(char c)
{
  if (mKaleidoPad)
    Serial1.write(c);
}

char kaleido_decode(char c)
{
  return c ^ mKey;
}

char kaleido_encode(char c)
{
  return c ^ mKey;
}

int kaleido_encode_digits(int val, char array[], int idx, bool encode)
{
  char encode_digit[]  = {0x6b, 0x4b, 0x2b, 0x0b, 0x6f, 0x4f, 0x2f, 0x0f, 0x63, 0x43};
  int tmp, tmp1, num_digit;

  // count number of digit needed
  num_digit = 1;
  tmp = val;
  tmp1 = 1;
  while (tmp > 9) {
    tmp /= 10;
    tmp1 *=10;
    num_digit++;
  }
  while (tmp1 > 0) {
    tmp = val / tmp1;
    for (int i = 0; i < sizeof(encode_digit); i++) {
      if (i == tmp) {
        if (encode)
          array[idx++] = encode_digit[i];
        else
          array[idx++] = i + '0';
      }
    }
    val -= (tmp * tmp1);
    tmp1 /= 10;
  };
  return num_digit;
}

enum KALEIDO_INFO_IDX {
  SV_IDX = 1,
  BT_IDX = 3,
  ET_IDX = 4,
  POWER_IDX = 6,
  DRUM_IDX = 7,
  SMOKE_IDX = 8,
}; // base 1

int kaleido_decode_digits(int idx, int num_digit)
{
  char encode_digit[]  = {0x6b, 0x4b, 0x2b, 0x0b, 0x6f, 0x4f, 0x2f, 0x0f, 0x63, 0x43};
  int i, pos, ret = 0; 
  char c;

  pos = 0;
  for (i = 0; i < mNum; i++) {
    if (mArray[i] == 0x23)
      pos++;
    if (pos == idx) {
      pos = i + 1;
      break;
    }
  }
  if (i == mNum)
    return -1;

  while (num_digit-- > 0 && (pos < mNum)) {
    c = mArray[pos++];
    for (i = 0; i < sizeof(encode_digit); i++) {
      if (encode_digit[i] == c) {
        ret = ret * 10 + i;
        break;
      }
    }
    if (i == sizeof(encode_digit))
      return ret;
  }

  return ret;
}

void kaleido_parse_info()
{
  int val;

  if (mArray[0] != 0)
    return;

  /* Read temperature
     Bean temp at offset 22
     ET temp at offset 33 */
  val = kaleido_decode_digits(BT_IDX, 3);
  if (val == -1) {
    Serial.printf("Error decode Bean temp\n\r");
    return;
  }
  mBTmp = val;

  val = kaleido_decode_digits(ET_IDX, 3);
  if (val == -1) {
    Serial.printf("Error decode Enviroment temp\n\r");
    return;
  }
  mETmp = val;

  val = kaleido_decode_digits(DRUM_IDX, 3);
  if (val != -1)
    mDrum = val;
  else
    mDrum = 0;

  val = kaleido_decode_digits(SMOKE_IDX, 3);
  if (val != -1)
    mSmoke = val;
  else
    mSmoke = 0;

  val = kaleido_decode_digits(POWER_IDX, 3);
  if (val != -1)
    mPower = val;
  else
    mPower = 0;

  val = kaleido_decode_digits(SV_IDX, 3);
  if (val != -1)
    mSV = val;
  else
    mSV = 0;

  Serial.printf("Parsed info:\n\r");
  Serial.printf("\tET: %d BT: %d Power: %d Smoke: %d Drum %d SV %d\n\r",
                  mETmp, mBTmp, mPower, mSmoke, mDrum, mSV);
}

void kaleido_service()
{
  int avail, c, flag = 0;
  char reset0[] = {0xff};
  char reset1[] = {0x08,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f};
  char reset2[] = {0x16, 0x20};

  avail = Serial2.available();
  if (avail > 0) {
    Serial.printf("Serial2(mState:%d mNum:%d):\n\r", mState, mNum);
    flag = 1;
  }
  while (avail--) {
    c = Serial2.read();
    kaleido_pad_send_char(c);
    Serial.printf("%02x ", c);
    switch(mState) {
    case 0:
    case 1:
    case 2:
      if (mNum < sizeof(reset1)) {
        if (reset1[mNum] == c)
          mNum++;
        else
          mNum = 0;
      } 
      if (mNum == sizeof(reset1)) {
        if (!mKaleidoPad)
          kaleido_send(reset2, sizeof(reset2));
        mNum = 0;
        mState++;
      }
      break;
    case 3:
      if (mNum == 0) {
        if (reset2[mNum] == c)
          mNum++;
      } else {
        mNum = 0;
        if (mKaleidoPad)
          mState = 7;
        else
          mState++;
      }
      break;
    case 4:
      /* First char will be mKey */ 
      mKey = c;
      mArray[mNum++] = kaleido_decode(c);
      mInitDelay = 0;
      mState++;
      break;
    case 5:
      /* reset sequence done by KaleidoPad */
      if (mInitDelay == 400)
      {
        char seq0[] = {0,0x1,0x38,0x6C,0x8,0xC,0x7B,0x14,0x3C,0x3C,0x45,0x44,0x77};
        char seq1[] = {0,0x1,0x68,0x18,0x4F,0x2C,0x4,0x4B,0x7B,0x6B,0x45,0x44,0x77,0x0,0x1,0x68,0x18,0x4F,0x2C,0x4,0x2B,0x7B,0x6B,0x45,0x44,0x77};
        char seq2[] = {0,0x1,0x8,0x5C,0x6C,0x3C,0x58,0x34,0x60,0xB,0x8,0x68,0x5C,0x5C,0x7C,0x7B,0x6B,0x45,0x44,0x77,0x0,0x1,0x68,0x28,0x14,0x7C,0x18,0x7B,0x2B,0x2B,0x2B,0x4B,0xF,0x63,0x4B,0x4B,0x4B,0xB,0x45,0x44,0x77,0x0,0x1,0x1C,0x5C,0x6C,0x3C,0xC,0x2C,0x5C,0x28,0x45,0x44,0x77};
        kaleido_send_encode(seq0, sizeof(seq0));
        kaleido_send_encode(seq1, sizeof(seq1));
        kaleido_send_encode(seq2, sizeof(seq2));
        mInitDelay = 0;
        mState++;
      } else
        mInitDelay++;
      break;
    case 6:
    case 7:
      if (mInitDelay == 50)
      {
        char seq0[] = {0,0x1,0x1C,0x5C,0x6C,0x3C,0xC,0x2C,0x5C,0x28,0x45,0x44,0x77};
        kaleido_send_encode(seq0, sizeof(seq0));
        mInitDelay = 0;
        mState++;
      } else
        mInitDelay++;
      break;
    case 8:
      mArray[mNum++] = kaleido_decode(c);
      if (kaleido_decode(c) == 0x77) {
        kaleido_parse_info();
        mNum = 0;
      }
      break;
    }
  }
  if (flag) {
    Serial.printf("\n\r");
  }

  flag = 0;
  avail = Serial1.available();
  if (avail > 0) {
    Serial.printf("Serial1(mStatePad:%d mNumPad:%d):\n\r", mStatePad, mNumPad);
    mKaleidoPad = 1;
    flag = 1;
  }
  while (avail--) {
    c = Serial1.read();
    Serial2.write(c);
    Serial.printf("%02x ", c);
    switch (mStatePad) {
    case 0:
      mStatePad++;
      mNumPad = 0;
      break;
    case 1:
      if (reset1[mNumPad] == c)
        mNumPad++;
      else
        mNumPad = 0;
      if (mNumPad == sizeof(reset1)) {
        mNumPad = 0;
        mStatePad++;
        mNum = 0;
        mState = 2;
      }
      break;
    case 2:
      if (mNumPad == 0) {
        if (reset2[mNumPad] == c)
          mNumPad++;
      } else {
        mNumPad = 0;
        mStatePad++;
      }
      break;
    case 3:
      /* normaly operation */
      mArrayPad[mNumPad++] = c;
      if (mNumPad >= sizeof(reset1)) {
        for (int i = 0; i < mNumPad - sizeof(reset1);) {
          int j;
          for (j = 0; j < sizeof(reset1);) {
            if (mArrayPad[i++] != reset1[j++])
              break;
          }
          if (j == sizeof(reset1)) {
            /* Pad requests reset */
            mState = 2;
            mStatePad = 2;
            mNumPad = 0;
            mNum = 0;
          }
        }
      }
      if (kaleido_decode(c) == 0x77 || mNumPad == sizeof(mArrayPad))
        mNumPad = 0;
      break;
    }
  }
  if (flag) {
    Serial.printf("\n\r");
  }

  if (!mKaleidoPad) {
    if (mState == 0) {
      kaleido_send(reset0, sizeof(reset0));
      mState++;
    } else if (mState == 1) {
      kaleido_send(reset1, sizeof(reset1));
      mState++;
      mNum = 0;
    } else if (mState == 2) {
      kaleido_send(reset2, sizeof(reset2));
      mState++;
      mNum = 0;
    }
  }

  if (!mNum) {
    if (mTimeout == 400) {
      //Reset Kaleido due to timeout
      if (!mKaleidoPad) {
        kaleido_send(reset1, sizeof(reset1));
        mState = 2;
      }
      mTimeout = 0;
    } else
      mTimeout++;
  } else
    mTimeout = 0;
}

enum {
  CMD_SV,
  CMD_HPM,
  CMD_OT1,
  CMD_OT2,
  CMD_IO3,
  CMD_COOLER,
};

int kaleido_cmd_inject_value(char array[], int len, int val, int idx, int num_digit)
{
  int count = kaleido_encode_digits(val, array, idx, true);
  int new_len = len - (num_digit - count);
  for (int i = idx + count; i < new_len; i++)
    array[i] = array[i + (num_digit - count)];

  return new_len;
}

void kaleido_send_cmd(int cmd, int val)
{
  switch (cmd) {
  case CMD_SV:
    {
      char data[] = {0,0x1,0x8,0x6C,0x58,0x28,0x6C,0x58,0x4C,0x6C,0x14,0x70,0x5C,0x58,0x6C,0x7B,0x0,0x0,0x0,0x45,0x44,0x77};
      if (val > 230) {
        Serial.printf("Set SV: %d not valid\n\r", val);
        return;
      }
      Serial.printf("Sending sv:%d\n\r", val);
      int new_len = kaleido_cmd_inject_value(data, sizeof(data), val, 16, 3);
      kaleido_send_encode(data, new_len);
    }
    break;
  case CMD_HPM:
    {
      char data[] = {0,0x1,0x8,0x5C,0x6C,0x70,0x68,0x54,0x7B,0x58,0x45,0x44,0x77};
      if (!val)
        data[9] = 0x54;
      Serial.printf("Sending HPM:%d\n\r", val);
      kaleido_send_encode(data, sizeof(data));
    }
    break;
  case CMD_OT1:
    {
      char data[] = {0,0x1,0x8,0x5C,0x6C,0x70,0x68,0x54,0x7B,0x54,0x45,0x44,0x77,0,0x1,0x8,0x5C,0x6C,0x70,0x5C,0x58,0x6C,0x50,0x34,0x1C,0x68,0x14,0xC,0x5C,0x28,0x7B,0x4B,0x7B,0x0,0x0,0x0,0x45,0x44,0x77};
      if (val > 100 || val < 0) {
        Serial.printf("Set OT1: %d not valid\n\r", val);
        return;
      }
      Serial.printf("Sending OT1(power):%d\n\r", val);
      int new_len = kaleido_cmd_inject_value(data, sizeof(data), val, 33, 3);
      kaleido_send_encode(data, new_len);
    }
    break;
  case CMD_OT2:
    {
      char data[] = {0,0x1,0x8,0x5C,0x6C,0x8,0x54,0x14,0x10,0x5C,0x8,0x68,0x5C,0x5C,0x7C,0x7B,0x0,0x0,0x0,0x45,0x44,0x77};
      if (val > 100 || val < 0) {
        Serial.printf("Set OT2: %d not valid\n\r", val);
        return;
      }
      Serial.printf("Sending OT2(smoke):%d\n\r", val);
      int new_len = kaleido_cmd_inject_value(data, sizeof(data), val, 16, 3);
      kaleido_send_encode(data, new_len);
    }
    break;
  case CMD_IO3:
    {
      char data[] = {0,0x1,0x8,0x5C,0x6C,0x28,0x14,0x74,0x74,0x5C,0x28,0x8,0x68,0x5C,0x5C,0x7C,0x7B,0x0,0x0,0x0,0x45,0x44,0x77};
      if (val > 100 || val < 0) {
        Serial.printf("Set IO3: %d not valid\n\r", val);
        return;
      }
      Serial.printf("Sending IO3(drum):%d\n\r", val);
      int new_len = kaleido_cmd_inject_value(data, sizeof(data), val, 17, 3);
      kaleido_send_encode(data, new_len);
    }
    break;
  case CMD_COOLER:
    {
      char data_on[] = {0,0x1,0x8,0x5C,0x6C,0x3C,0x58,0x34,0x60,0xB,0x8,0x68,0x5C,0x5C,0x7C,0x7B,0x4B,0x6B,0x6B,0x45,0x44,0x77};
      char data_off[] = {0,0x1,0x8,0x5C,0x6C,0x3C,0x58,0x34,0x60,0xB,0x8,0x68,0x5C,0x5C,0x7C,0x7B,0x6B,0x45,0x44,0x77};
      Serial.printf("Sending cooler:%d\n\r", val);
      if (val)
        kaleido_send_encode(data_on, sizeof(data_on));
      else
        kaleido_send_encode(data_off, sizeof(data_off));
    }
    break;
  }
}

void artisan_send_ok()
{
  Serial.printf("Sending init ok\n\r");
  SerialBT.write('#');
  SerialBT.write('1');
  SerialBT.write('\n');
}

void artisan_send_err()
{
  Serial.printf("Sending init error\n\r");
  SerialBT.write('@');
  SerialBT.write('1');
  SerialBT.write('\n');
}

int artisan_info_inject_value(char array[], int len, int val, int idx, int num_digit)
{
  int count = kaleido_encode_digits(val, array, idx, false);
  int new_len = len - (num_digit - count);
  for (int i = idx + count; i < new_len; i++)
    array[i] = array[i + (num_digit - count)];

  return new_len;
}

void artisan_send_current_info()
{
  char array[] = {'0',',','0','0','0',',','0','0','0',',','0','0','0',',','0','0','0',',','0','0','0',',','C','\n'};
  int new_len;

  new_len = artisan_info_inject_value(array, sizeof(array), mETmp, 2, 3);
  new_len = artisan_info_inject_value(array, new_len, mBTmp, 6 - (sizeof(array) - new_len) , 3);
  new_len = artisan_info_inject_value(array, new_len, mPower, 10 - (sizeof(array) - new_len) , 3);
  new_len = artisan_info_inject_value(array, new_len, mSmoke, 14 - (sizeof(array) - new_len) , 3);
  new_len = artisan_info_inject_value(array, new_len, mSV, 18 - (sizeof(array) - new_len) , 3);
  Serial.printf("Sending current info :0,%d,%d,%d,%d,%d\n\r", mETmp, mBTmp, mPower, mSmoke, mSV);
  for (int i = 0; i < new_len; i++)
    Serial.printf("%c ", array[i]);
  Serial.printf("\n\r");

  for (int i = 0; i < new_len; i++)
    SerialBT.write(array[i]);
}

void artisan_cmd_set_sv(String sv)
{
  Serial.printf("Parsing sv:%s\n\r", sv);
  //Set SV temp
  kaleido_send_cmd(CMD_SV, sv.toInt());
}

void artisan_cmd_set_hpm(String mode)
{
  Serial.printf("Parsing hpa mode:%s\n\r", mode);
  if (mode == "A") {
    kaleido_send_cmd(CMD_HPM, 1);
  } else {
    kaleido_send_cmd(CMD_HPM, 0);
  }
}

void artisan_cmd_set_ot1(String val)
{
  Serial.printf("Parsing ot1:%s\n\r", val);
  if (val == "UP") {
    // increase power 5
    mPower += 5;
    if (mPower > 100)
      mPower = 100;
    // set power
    kaleido_send_cmd(CMD_OT1, mPower);
  } else if (val == "DOWN") {
    // decrease power 5
    mPower -= 5;
    if (mPower < 0)
      mPower = 0;
    // set power
    kaleido_send_cmd(CMD_OT1, mPower);
  } else
    // set power
    kaleido_send_cmd(CMD_OT1, val.toInt());
}

void artisan_cmd_set_ot2(String val)
{
  Serial.printf("Parsing ot2:%s\n\r", val);
  if (val == "UP") {
    // increase smoke 5
    mSmoke += 5;
    if (mSmoke > 100)
      mSmoke = 100;
    // set smoke
    kaleido_send_cmd(CMD_OT2, mSmoke);
  } else if (val == "DOWN") {
    // decrease smoke 5
    mSmoke -= 5;
    if (mSmoke < 0)
      mSmoke = 0;
    // set smoke
    kaleido_send_cmd(CMD_OT2, mSmoke);
  } else
    // set smoke
    kaleido_send_cmd(CMD_OT2, val.toInt());
}

void artisan_cmd_set_io3(String val)
{
  Serial.printf("set io3:%s\n\r", val);
  // set roller
  kaleido_send_cmd(CMD_IO3, val.toInt());
}

void artisan_cmd_set_cooler(String mode)
{
  Serial.printf("set cooler mode:%s\n\r", mode);
  if (mode == "ON") {
    // set cooler on
    kaleido_send_cmd(CMD_COOLER, 1);
  } else {
    // set cooler off
    kaleido_send_cmd(CMD_COOLER, 0);
  }
}

void artisan_process_cmd(String cmd)
{
  String type, parameters;
  int pos;

  if (cmd == "READ") {
    return artisan_send_current_info();
  }

  pos = cmd.indexOf(';');
  if (pos == -1) {
    pos = cmd.indexOf(',');
    if (pos == -1) {
      Serial.printf("Failed to parse cmd: %s\n\r", cmd);
      return;
    }
  }

  type = cmd.substring(0, pos);
  parameters = cmd.substring(pos + 1);

  Serial.printf("type:%s parameters:%s\n\r", type, parameters);

  if (type == "CHAN") {
    if (parameters != "2100") {
      Serial.printf("%s not valid\n\r");
      return artisan_send_err();
    } else
      return artisan_send_ok();
  }
  if (type == "EVT") {
    if (parameters == "1") {
      Serial.printf("Charged event, reduce power to 20\n\r");
      kaleido_send_cmd(CMD_OT1, 20);
      return;
    }
    if (parameters == "8") {
      Serial.printf("Turn off heater and on cooler\n\r");
      kaleido_send_cmd(CMD_SV, 0);
      kaleido_send_cmd(CMD_COOLER, 1);
      return;
    }
    if (parameters == "9") {
      Serial.printf("Cool end, stop cooler\n\r");
      kaleido_send_cmd(CMD_COOLER, 0);
      return;
    }
  }
  if (type == "UNITS") {
    // just accept anythings
    return artisan_send_ok();
  }
  if (type == "FILT") {
    // just accept anythings
    return artisan_send_ok();
  }
  if (type == "SV")
    return artisan_cmd_set_sv(parameters);
  if (type == "HPM")
    return artisan_cmd_set_hpm(parameters);
  if (type == "OT1")
    return artisan_cmd_set_ot1(parameters);
  if (type == "OT2")
    return artisan_cmd_set_ot2(parameters);
  if (type == "IO3")
    return artisan_cmd_set_io3(parameters);
  if (type == "CLDN")
    return artisan_cmd_set_cooler(parameters);
  if (type == "PID") {
    pos = parameters.indexOf(';');
    if (pos == -1) {
      Serial.printf("Failed to parse cmd: %s\n\r", cmd);
      return;
    }
    type = parameters.substring(0, pos);
    parameters = parameters.substring(pos + 1);
    if (type == "SV")
      return artisan_cmd_set_sv(parameters);
  }

  Serial.printf("Unknown type %s %s\n\r", type, parameters);
}

void artisan_service()
{
  int c;

  while (SerialBT.available()) {
    if (mBTCur > (sizeof(mBTCmd) - 1)) {
      mBTCmd[mBTCur] = '\0';
      Serial.printf("BT command oversize: %s <- %d\n\r", mBTCmd, mBTCur);
    }
    c = SerialBT.read();
    if (c == '\n') {
      //Process command
      mBTCmd[mBTCur] = '\0';
      Serial.printf("Received: ");
      for (c = 0; c < mBTCur; c++) {
        Serial.printf("%c", mBTCmd[c]);
      }
      Serial.printf("\n\r");
      String cmd = String(mBTCmd);
      artisan_process_cmd(cmd);
      Serial.printf("Processed command\n\r");
      mBTCur = 0;      
    } else
      mBTCmd[mBTCur++] = c;
  }
}

/* Enumulating communicaton with kaleido pad */
// void snipper_cmd() {
//   int avail, c, idx;
//   int flag = 0;
//   char array1[] = {0,0x6F,0x44,0x0,0x58,0x70,0x6C,0x23,0x57,0x44,0x0,0x70,0x68,0x54,0x23,0x58,0x44,0x0,0x6C,0x18,0x4B,0x23,0x4B,0x4B,0x37,0x6B,0x6B,0x44,0x0,0x6C,0x18,0x2B,0x23,0x4B,0x63,0x37,0x4F,0x6B,0x44,0x0,0x70,0x6C,0x4B,0x68,0x23,0x6B,0x44,0x0,0x70,0x6C,0x4B,0x68,0x54,0x70,0x2C,0x23,0x6B,0x44,0x0,0x28,0x74,0x9,0x69,0x23,0x6B,0x44,0x0,0x8,0x54,0x9,0x69,0x23,0x6B,0x44,0x0,0x3C,0xB,0x9,0x69,0x23,0x6B,0x44,0x77};
//   char array2[] = {1,0x3C,0xC,0x2C,0x5C,0x28,0x23,0x54,0x58,0x37,0x2C,0x2B,0x6B,0x6F,0x2B,0x4F,0x45,0x77};
//   int array_len;

//   avail = Serial1.available();
//   if (avail > 0) {
//     Serial.printf("Serial1 state %d num %d:\n\r", mState, mNum);
//     flag = 1;
//   }
//   while (avail--) {
//     c = Serial1.read();
//     Serial.printf("%02x ", c);
//     switch (mState) {
//     case 0:
//       if (c == 0xff)
//         mState++;
//       break;
//     case 1:
//     case 2:
//       if (mState == 1)
//         array_len = 8;
//       if (mState == 2)
//         array_len = 2;
//       if (mNum < array_len) {
//         mArray[mNum++] = c;
//       }
//       if (mNum >= array_len) {
//         if (mState == 2) {
//           mKey = mArray[1] & 0xf0;
//           mKey = 0xc0 | (mArray[1] & 0x0f);
//           mArray[1] = (mArray[1] & 0xf0);
//         }
//         for (idx = 0; idx < array_len; idx++) {
//           Serial1.write(mArray[idx]);
//         } 
//         Serial.printf("\n\rSerial1 out:");
//         for (idx = 0; idx < array_len; idx++) {
//           Serial.printf("%02x ", mArray[idx]);
//         } 
//         Serial.printf("\n\r");
//         mState++;
//         mNum = 0;
//       }
//       break;
//     case 3:
//     case 4:
//     case 5:
//     case 6:
//     case 7:
//     case 8:
//     case 10:
//     case 12:
//       mArray[mNum] = c ^ mKey;
//       if (mArray[mNum] == 0x77) {
//         mState++;
//         mNum = 0;
//       } else
//         mNum++;
//       break;
//     }
//   }
//   if (flag) {
//     Serial.printf("\n\r");
//   }
//   if (mState == 9 || mState == 11 || mState == 13) {
//     for (idx = 0; idx < sizeof(array2)/sizeof(array2[0]); idx++) {
//       c = array2[idx] ^ mKey;
//       Serial1.write(c);
//     }
//     Serial.printf("\n\rSerial1 out:");
//     for (idx = 0; idx < sizeof(array2)/sizeof(array2[0]); idx++) {
//       c = array2[idx] ^ mKey;
//       Serial.printf("%02x ", c);
//     }
//     Serial.printf("\n\r");
//     mState++;
//     mNum = 0;
//     if (mState == 14)
//       array1[1] = 0x6b;    
//   } else if (mState == 3 || mState == 4 || mState == 14) {
//     for (idx = 0; idx < sizeof(array1)/sizeof(array1[0]); idx++) {
//       c = array1[idx] ^ mKey;
//       Serial1.write(c);
//     }
//     Serial.printf("\n\rSerial1 out:");
//     for (idx = 0; idx < sizeof(array1)/sizeof(array1[0]); idx++) {
//       c = array1[idx] ^ mKey;
//       Serial.printf("%02x ", c);
//     }
//     Serial.printf("\n\r");
//     delay(200);
//   }
// }

void loop()
{
  artisan_service();
  kaleido_service();
  //snipper();
  delay(20);
}
