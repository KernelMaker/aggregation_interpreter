#include "interpreter.h"

const char* g_chars = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";

const uint32_t g_prog_len = 8;
uint32_t program[g_prog_len];

int main() {

  memset(program, 0, sizeof(program));
  program[0] = ((uint16_t)0x0721) << 16 | (uint16_t)g_prog_len;
  program[1] = ((uint16_t)1) << 16 | ((uint16_t)2);

  program[2] = 0; // group by cols
  program[3] = kTypeInteger; // first aggregation type INTEGER
  program[4] = kTypeDouble; // first aggregation type DOUBLE

  program[5] = ((uint8_t)kOpCount) << 24 | (uint8_t)(kTypeInteger << 4) << 16 | (uint16_t)0; // COUNT +1 -> Agg0

  program[6] = ((uint8_t)kOpLoad) << 24 | (uint8_t)(kTypeDouble << 4) << 16 | ((uint8_t)kReg1 & 0x0F) << 16 | (uint16_t)1; // Load Col1 -> Reg1
  program[7] = ((uint8_t)kOpSum) << 24 | (uint8_t)(kTypeDouble << 4) << 16 | ((uint8_t)kReg1 & 0x0F) << 16 | (uint16_t)1; // Sum Reg1 -> Agg1


  AggInterpreter agg(program, g_prog_len);
  agg.Init();
  Record rec1(1, 1.11, g_chars + (rand() % 40), 12);
  rec1.Print();
  agg.ProcessRec(&rec1);
  Record rec2(1, 1.12, g_chars + (rand() % 40), 12);
  rec2.Print();
  agg.ProcessRec(&rec2);
  Record rec3(2, 2.22, g_chars + (rand() % 40), 12);
  rec3.Print();
  agg.ProcessRec(&rec3);

  agg.Print();

  return 0;
}
