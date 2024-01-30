#include <stdlib.h>

#include "interpreter.h"

/*
 * Table definition
 *
 * CREATE TABLE `t` (
 *   `a` bigint DEFAULT NULL,
 *   `b` double DEFAULT NULL,
 *   `c` bigint unsigned DEFAULT NULL,
 *   `d` double DEFAULT NULL,
 *   `e` varchar(20) DEFAULT NULL
 * ) ENGINE=ndbcluster DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci
 *
 */
/*
 * Aggregation
 *
 * select count(a), sum(a/b+c*d), max((a+b)*c/d), min(b%c+d), sum(a+c) from t group by a;
 */

const char* g_chars = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";

const uint32_t g_prog_len = 35;
const uint32_t ins_pos = 8;
uint32_t program[g_prog_len];

int main() {

  memset(program, 0, sizeof(program));
  program[0] = ((uint16_t)0x0721) << 16 | (uint16_t)g_prog_len;
  program[1] = ((uint16_t)1) << 16 | // num of cols used in group by
               ((uint16_t)5); // num of aggregation results

  program[2] = 0; // group by column 0
  program[3] = kTypeBigInt; // The first aggregation type BIGINT
  program[4] = kTypeDouble; // The second aggregation type DOUBLE
  program[5] = kTypeDouble; // The third aggregation type BIGINT
  program[6] = kTypeDouble; // The fouth aggregation type DOUBLE
  program[7] = kTypeBigInt; // The fifth aggregation type DOUBLE

  program[ins_pos] =
               ((uint8_t)kOpCount) << 26 |                              // COUNT
               1 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |            // unsigned kTypeBigInt
               (uint16_t)0;                                             // agg_result 0

  program[ins_pos + 1] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |             // kTypeBigInt
               ((uint8_t)kReg1 & 0x0F) << 16 |                           // Register 1
               (uint16_t)0;                                              // Column 0

  program[ins_pos + 2] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |             // kTypeDouble
               ((uint8_t)kReg2 & 0x0F) << 16 |                           // Register 2
               (uint16_t)1;                                              // Column 1

  program[ins_pos + 3] =
                ((uint8_t)kOpDiv) << 26 |                                    // DIV
                0 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |                // kTypeBigInt (Reg 1)
                0 << 20 | (uint8_t)(kTypeDouble << 4) << 12 |                // kTypeDouble (Reg 2)
                ((uint8_t)kReg1 & 0x0F) << 12 | ((uint8_t)kReg2 & 0xF) << 8; // Register 1, Register 2

  program[ins_pos + 4] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               1 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |             // unsigned kTypeBigInt
               ((uint8_t)kReg2 & 0x0F) << 16 |                           // Register 2
               (uint16_t)2;                                              // Column 2

  program[ins_pos + 5] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |             // kTypeDouble
               ((uint8_t)kReg3 & 0x0F) << 16 |                           // Register 3
               (uint16_t)3;                                              // Column 3

  program[ins_pos + 6] =
                ((uint8_t)kOpMul) << 26 |                                    // MUL
                1 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |                // unsigned kTypeBigInt (Reg 2)
                0 << 20 | (uint8_t)(kTypeDouble << 4) << 12 |                // kTypeDouble (Reg 3)
                ((uint8_t)kReg2 & 0x0F) << 12 | ((uint8_t)kReg3 & 0xF) << 8; // Register 2, Register 3

  program[ins_pos + 7] =
                ((uint8_t)kOpPlus) << 26 |                                   // PLUS
                0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |                // kTypeDouble (Reg 1)
                0 << 20 | (uint8_t)(kTypeDouble << 4) << 12 |                // kTypeDouble (Reg 2)
                ((uint8_t)kReg1 & 0x0F) << 12 | ((uint8_t)kReg2 & 0xF) << 8; // Register 1, Register 2

  program[ins_pos + 8] =
                ((uint8_t)kOpSum) << 26 |                                // SUM
                0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |            // kTypeDouble (Reg 1)
                ((uint8_t)kReg1 & 0x0F) << 16 |                          // Register 1
                (uint16_t)1;                                             // agg_result 1

  program[ins_pos + 9] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |             // kTypeBigInt
               ((uint8_t)kReg1 & 0x0F) << 16 |                           // Register 1
               (uint16_t)0;                                              // Column 0

  program[ins_pos + 10] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |             // kTypeDouble
               ((uint8_t)kReg2 & 0x0F) << 16 |                           // Register 2
               (uint16_t)1;                                              // Column 1

  program[ins_pos + 11] =
                ((uint8_t)kOpPlus) << 26 |                                   // PLUS
                0 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |                // kTypeBigInt (Reg 1)
                0 << 20 | (uint8_t)(kTypeDouble << 4) << 12 |                // kTypeDouble (Reg 2)
                ((uint8_t)kReg1 & 0x0F) << 12 | ((uint8_t)kReg2 & 0xF) << 8; // Register 1, Register 2

  program[ins_pos + 12] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               1 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |             // unsigned kTypeBigInt
               ((uint8_t)kReg2 & 0x0F) << 16 |                           // Register 2
               (uint16_t)2;                                              // Column 2

  program[ins_pos + 13] =
                ((uint8_t)kOpMul) << 26 |                                    // MUL
                0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |                // kTypeDouble (Reg 1)
                1 << 20 | (uint8_t)(kTypeBigInt << 4) << 12 |                // unsigned kTypeBigInt (Reg 2)
                ((uint8_t)kReg1 & 0x0F) << 12 | ((uint8_t)kReg2 & 0xF) << 8; // Register 1, Register 2
                                                                             
  program[ins_pos + 14] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |             // kTypeDouble
               ((uint8_t)kReg2 & 0x0F) << 16 |                           // Register 2
               (uint16_t)3;                                              // Column 3

  program[ins_pos + 15] =
                ((uint8_t)kOpDiv) << 26 |                                    // DIV
                0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |                // kTypeDouble (Reg 1)
                0 << 20 | (uint8_t)(kTypeDouble << 4) << 12 |                // kTypeDouble (Reg 2)
                ((uint8_t)kReg1 & 0x0F) << 12 | ((uint8_t)kReg2 & 0xF) << 8; // Register 1, Register 2

  program[ins_pos + 16] =
                ((uint8_t)kOpMax) << 26 |                                // MAX
                0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |            // kTypeDouble (Reg 1)
                ((uint8_t)kReg1 & 0x0F) << 16 |                          // Register 1
                (uint16_t)2;                                             // agg_result 10

  program[ins_pos + 17] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |             // kTypeDouble
               ((uint8_t)kReg1 & 0x0F) << 16 |                           // Register 1
               (uint16_t)1;                                              // Column 1

  program[ins_pos + 18] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               1 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |             // unsigned kTypeBigInt
               ((uint8_t)kReg2 & 0x0F) << 16 |                           // Register 2
               (uint16_t)2;                                              // Column 2

  program[ins_pos + 19] =
                ((uint8_t)kOpMod) << 26 |                                    // MOD
                0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |                // kTypeDouble (Reg 1)
                1 << 20 | (uint8_t)(kTypeBigInt << 4) << 12 |                // unsigned kTypeBigInt (Reg 2)
                ((uint8_t)kReg1 & 0x0F) << 12 | ((uint8_t)kReg2 & 0xF) << 8; // Register 1, Register 2

  program[ins_pos + 20] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |             // kTypeDouble
               ((uint8_t)kReg2 & 0x0F) << 16 |                           // Register 2
               (uint16_t)3;                                              // Column 3

  program[ins_pos + 21] =
                ((uint8_t)kOpMinus) << 26 |                                  // MINUS
                0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |                // kTypeDouble (Reg 1)
                0 << 20 | (uint8_t)(kTypeDouble << 4) << 12 |                // kTypeDouble (Reg 2)
                ((uint8_t)kReg1 & 0x0F) << 12 | ((uint8_t)kReg2 & 0xF) << 8; // Register 1, Register 2

  program[ins_pos + 22] =
                ((uint8_t)kOpMin) << 26 |                                // MIN
                0 << 25 | (uint8_t)(kTypeDouble << 4) << 17 |            // kTypeDouble (Reg 1)
                ((uint8_t)kReg1 & 0x0F) << 16 |                          // Register 1
                (uint16_t)3;                                             // agg_result 3

  program[ins_pos + 23] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               0 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |             // kTypeBigInt
               ((uint8_t)kReg1 & 0x0F) << 16 |                           // Register 1
               (uint16_t)0;                                              // Column 0

  program[ins_pos + 24] =
               ((uint8_t)kOpLoadCol) << 26 |                             // LOADCOL
               1 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |             // unsigned kTypeBigInt
               ((uint8_t)kReg2 & 0x0F) << 16 |                           // Register 2
               (uint16_t)2;                                              // Column 2

  program[ins_pos + 25] =
                ((uint8_t)kOpPlus) << 26 |                                   // Plus
                0 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |                // kTypeBigInt (Reg 1)
                1 << 20 | (uint8_t)(kTypeBigInt << 4) << 12 |                // unsigned kTypeBigInt (Reg 2)
                ((uint8_t)kReg1 & 0x0F) << 12 | ((uint8_t)kReg2 & 0xF) << 8; // Register 1, Register 2

  program[ins_pos + 26] =
                ((uint8_t)kOpSum) << 26 |                                // SUM
                0 << 25 | (uint8_t)(kTypeBigInt << 4) << 17 |            // kTypeBigInt (Reg 1)
                ((uint8_t)kReg1 & 0x0F) << 16 |                          // Register 1
                (uint16_t)4;                                             // agg_result 4



  AggInterpreter agg(program, g_prog_len);
  agg.Init();
  Record rec1(1, 1.11, 10, 10.1010, g_chars + (rand() % 40), 12);
  rec1.Print();
  agg.ProcessRec(&rec1);
  Record rec2(1, 1.12, 2, 0.11, g_chars + (rand() % 40), 12);
  rec2.Print();
  agg.ProcessRec(&rec2);
  Record rec3(2, 2.22, 1, 1.0, g_chars + (rand() % 40), 12);
  rec3.Print();
  agg.ProcessRec(&rec3);
  Record rec4(2, -4.22, 3, -3.4, g_chars + (rand() % 40), 12);
  rec4.Print();
  agg.ProcessRec(&rec4);
  Record rec5(2, -5.111, 5, -5.6, g_chars + (rand() % 40), 12);
  rec5.Print();
  agg.ProcessRec(&rec5);

  agg.Print();

  return 0;
}
