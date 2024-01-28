/*
 * Copyright [2024] <Copyright Hopsworks AB>
 *
 * Author: Zhao Song
 */
#include <assert.h>
#include <stdio.h>
#include <climits>
#include <utility>

#include "interpreter.h"

bool TestIfSumOverflowsUint64(uint64_t arg1, uint64_t arg2) {
  return ULLONG_MAX - arg1 < arg2;
}

int32_t RegPlusReg(const Register& a, const Register& b, Register* res) {
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble || b.type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt && b.type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (res_type == kTypeBigInt) {
    int64_t val0 = a.value.val_int64;
    int64_t val1 = b.value.val_int64;
    int64_t res_val = static_cast<uint64_t>(val0) + static_cast<uint64_t>(val1);
    bool res_unsigned = false;

    if (a.is_unsigned) {
      if (b.is_unsigned || val1 >= 0) {
        if (TestIfSumOverflowsUint64((uint64_t)val0, (uint64_t)val1)) {
          // overflows;
          return -1;
        } else {
          res_unsigned = true;
        }
      } else {
        if ((uint64_t)val0 > (uint64_t)(LLONG_MAX)) {
          res_unsigned = true;
        }
      }
    } else {
      if (b.is_unsigned) {
        if (val0 >= 0) {
          if (TestIfSumOverflowsUint64((uint64_t)val0, (uint64_t)val1)) {
            // overflows;
            return -1;
          } else {
            res_unsigned = true;
          }
        } else {
          if ((uint64_t)val1 > (uint64_t)(LLONG_MAX)) {
            res_unsigned = true;
          }
        }
      } else {
        if (val0 >= 0 && val1 >= 0) {
          res_unsigned = true;
        } else if (val0 < 0 && val1 < 0 && res_val >= 0) {
          // overflow
          return -1;
        }
      }
    }

    // Check if res_val is overflow
    bool unsigned_flag = (a.is_unsigned != b.is_unsigned);
    if ((unsigned_flag && !res_unsigned && res_val < 0) ||
        (!unsigned_flag && res_unsigned &&
         (uint64_t)res_val > (uint64_t)LLONG_MAX)) {
      return -1;
    } else {
      if (unsigned_flag) {
        res->value.val_uint64 = res_val;
      } else {
        res->value.val_int64 = res_val;
      }
    }
    res->is_unsigned = unsigned_flag;
  } else {
    double val0 = (a.type == kTypeDouble) ?
                     a.value.val_double :
                     ((a.is_unsigned == true) ?
                       static_cast<double>(a.value.val_uint64) :
                       static_cast<double>(a.value.val_int64));
    double val1 = (b.type == kTypeDouble) ?
                     b.value.val_double :
                     ((b.is_unsigned == true) ?
                       static_cast<double>(b.value.val_uint64) :
                       static_cast<double>(b.value.val_int64));
    double res_val = val0 + val1;
    if (std::isfinite(res_val)) {
      res->value.val_double = res_val;
    } else {
      // overflow
      return -1;
    }
    res->is_unsigned = false;
  }

  res->type = res_type;

  return 0;
}

int32_t RegMinusReg(const Register& a, const Register& b, Register* res) {
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble || b.type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt && b.type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (res_type == kTypeBigInt) {
    int64_t val0 = a.value.val_int64;
    int64_t val1 = b.value.val_int64;
    int64_t res_val = static_cast<uint64_t>(val0) - static_cast<uint64_t>(val1);
    bool res_unsigned = false;

    if (a.is_unsigned) {
      if (b.is_unsigned) {
        if (static_cast<uint64_t>(val0) < static_cast<uint64_t>(val1)) {
          if (res_val >= 0) {
            // overflow
            return -1;
          } else {
            res_unsigned = true;
          }
        }
      } else {
        if (val1 >= 0) {
          if (static_cast<uint64_t>(val0) > static_cast<uint64_t>(val1)) {
            res_unsigned = true;
          }
        } else {
          if (TestIfSumOverflowsUint64((uint64_t)val0, (uint64_t)-val1)) {
            // overflow
            return -1;
          } else {
            res_unsigned = true;
          }
        }
      }
    } else {
      if (b.is_unsigned) {
        if (static_cast<uint64_t>(val0) - LLONG_MIN <
            static_cast<uint64_t>(val1)) {
          // overflow
          return -1;
        } else {
          if (val0 >= 0 && val1 < 0) {
            res_unsigned = true;
          } else if (val0 < 0 && val1 > 0 && res_val >= 0) {
            // overflow
            return -1;
          }
        }
      }
    }
    // Check if res_val is overflow
    bool unsigned_flag = (a.is_unsigned != b.is_unsigned);
    if ((unsigned_flag && !res_unsigned && res_val < 0) ||
        (!unsigned_flag && res_unsigned &&
         (uint64_t)res_val > (uint64_t)LLONG_MAX)) {
      return -1;
    } else {
      if (unsigned_flag) {
        res->value.val_uint64 = res_val;
      } else {
        res->value.val_int64 = res_val;
      }
    }
    res->is_unsigned = unsigned_flag;
  } else {
    assert(res_type == kTypeDouble);
    double val0 = (a.type == kTypeDouble) ?
                     a.value.val_double :
                     ((a.is_unsigned == true) ?
                       static_cast<double>(a.value.val_uint64) :
                       static_cast<double>(a.value.val_int64));
    double val1 = (b.type == kTypeDouble) ?
                     b.value.val_double :
                     ((b.is_unsigned == true) ?
                       static_cast<double>(b.value.val_uint64) :
                       static_cast<double>(b.value.val_int64));
    double res_val = val0 - val1;
    if (std::isfinite(res_val)) {
      res->value.val_double = res_val;
    } else {
      // overflow
      return -1;
    }
  }
  res->type = res_type;
  return 0;
}

int32_t RegStoreToAggResItem(const Register& a, AggResItem* res) {
  DataType type1 = a.type;
  DataType type2 = res->type;

  assert(type1 == type2 && type1 == kTypeBigInt);

  // case kTypeBigInt
  res->value.val_int64 = a.value.val_int64;
  return 0;
}

int32_t RegIncrToAggResItem(const Register& a, AggResItem* res) {
  assert(res != nullptr && a.type == res->type);

  switch (res->type) {
    case kTypeBigInt:
      res->value.val_int64 = a.value.val_int64 + res->value.val_int64;
      break;
    case kTypeDouble:
      res->value.val_double = a.value.val_double + res->value.val_double;
      break;
    default:
      assert(0);
  }
  return 0;
}

struct OperItem {
  uint8_t op;
  uint8_t index;
  void (*FuncBigInt)(int64_t, int64_t, int64_t*);
  void (*FuncBigUint)(uint64_t, uint64_t, uint64_t*);
  void (*FuncDouble)(double, double, double*);
};

void PlusBigInt(int64_t a, int64_t b, int64_t* res) {
  *res = a + b;
}
void PlusBigUint(uint64_t a, uint64_t b, uint64_t* res) {
  *res = a + b;
}
void PlusDouble(double a, double b, double* res) {
  *res = a + b;
}

void MinusBigInt(int64_t a, int64_t b, int64_t* res) {
  *res = a - b;
}
void MinusBigUint(uint64_t a, uint64_t b, uint64_t* res) {
  *res = a - b;
}
void MinusDouble(double a, double b, double* res) {
  *res = a - b;
}

void MulBigInt(int64_t a, int64_t b, int64_t* res) {
  *res = a * b;
}
void MulBigUint(uint64_t a, uint64_t b, uint64_t* res) {
  *res = a * b;
}
void MulDouble(double a, double b, double* res) {
  *res = a * b;
}

void DivBigInt(int64_t a, int64_t b, int64_t* res) {
  *res = a / b;
}
void DivBigUint(uint64_t a, uint64_t b, uint64_t* res) {
  *res = a / b;
}
void DivDouble(double a, double b, double* res) {
  *res = a / b;
}

void ModBigInt(int64_t a, int64_t b, int64_t* res) {
  *res = a % b;
}
void ModBigUint(uint64_t a, uint64_t b, uint64_t* res) {
  *res = a % b;
}
void ModDouble(double a, double b, double* res) {
  *res = std::fmod(a, b);
}

void MaxBigInt(int64_t a, int64_t b, int64_t* res) {
  *res = a > b ? a : b;
}
void MaxBigUint(uint64_t a, uint64_t b, uint64_t* res) {
  *res = a > b ? a : b;
}
void MaxDouble(double a, double b, double* res) {
  *res = a > b ? a : b;
}

void MinBigInt(int64_t a, int64_t b, int64_t* res) {
  *res = a < b ? a : b;
}
void MinBigUint(uint64_t a, uint64_t b, uint64_t* res) {
  *res = a < b ? a : b;
}
void MinDouble(double a, double b, double* res) {
  *res = a < b ? a : b;
}
OperItem OperArray[kOpTotal] = {
  OperItem{kOpUnknown, kOpUnknown, nullptr, nullptr, nullptr},
  OperItem{kOpPlus, kOpPlus, &PlusBigInt, &PlusBigUint, &PlusDouble},
  OperItem{kOpMinus, kOpMinus, &MinusBigInt, &MinusBigUint, &MinusDouble},
  OperItem{kOpMul, kOpMul, &MulBigInt, &MulBigUint, &MulDouble},
  OperItem{kOpDiv, kOpDiv, &DivBigInt, &DivBigUint, &DivDouble},
  OperItem{kOpMod, kOpMod, &ModBigInt, &ModBigUint, &ModDouble},
  OperItem{kOpLoadCol, kOpLoadCol, nullptr, nullptr, nullptr},
  OperItem{kOpSum, kOpSum, &PlusBigInt, &PlusBigUint, &PlusDouble},
  OperItem{kOpMax, kOpMax, &MaxBigInt, &MaxBigUint, &MaxDouble},
  OperItem{kOpMin, kOpMin, &MinBigInt, &MinBigUint, &MinDouble},
  OperItem{kOpCount, kOpCount, &PlusBigInt, &PlusBigUint, nullptr}
};

bool AggInterpreter::Init() {
  if (inited_) {
    return true;
  }

  uint32_t value = 0;

  /*
   * 1. Double check the magic num and  total length of program.
   */
  value = prog_[cur_pos_++];
  assert(((value & 0xFFFF0000) >> 16) == 0x0721);
  assert((value & 0xFFFF) == prog_len_);

  /*
   * 2. Get num of columns for group by and num of aggregation results;
   */
  value = prog_[cur_pos_++];
  n_gb_cols_ = (value >> 16) & 0xFFFF;
  n_agg_results_ = value & 0xFFFF;

  /*
   * 3. Get all the group by columns id.
   */
  if (n_gb_cols_) {
    gb_cols_ = new uint32_t[n_gb_cols_];

    uint32_t i = 0;
    while (i < n_gb_cols_ && cur_pos_ < prog_len_) {
      gb_cols_[i++] = prog_[cur_pos_++];
    }

    gb_map_ = new std::map<Entry, Entry, EntryCmp>;
  }

  /*
   * 4. Get all aggregation results types
   */
  if (n_agg_results_) {
    agg_results_ = new AggResItem[n_agg_results_];
    uint32_t i = 0;
    while (i < n_agg_results_ && cur_pos_ < prog_len_) {
      agg_results_[i].type = prog_[cur_pos_++];
      agg_results_[i++].value.val_int64 = 0;
    }
  }

  inited_ = true;
  agg_prog_start_pos_ = cur_pos_;
  memset(registers_, 0, sizeof(registers_));

  return true;
}

DataType CeilType(DataType type) {
  switch (type) {
    case kTypeTinyInt:
    case kTypeSmallInt:
    case kTypeMediumInt:
    case kTypeBigInt:
      return kTypeBigInt;
    case kTypeFloat:
    case kTypeDouble:
      return kTypeDouble;
    default:
      assert(0);
  }
}

bool DecodeRawType(uint8_t type, DataType* res) {
  *res = type & 0x0F;
  return type & 0x10;
}

bool AggInterpreter::ProcessRec(Record* rec) {
  AggResItem* agg_res_ptr = nullptr;

  if (n_gb_cols_) {
    uint32_t agg_rec_len = 0;
    for (uint32_t i = 0; i < n_gb_cols_; i++) {
      Column* col = rec->GetColumn(i);
      agg_rec_len += col->encoded_length();
    }
    agg_rec_len += (n_agg_results_ * sizeof(AggResItem));
    char* agg_rec = new char[agg_rec_len];
    memset(agg_rec, 0, agg_rec_len);

    uint32_t pos = 0;
    for (uint32_t i = 0; i < n_gb_cols_; i++) {
      Column* col = rec->GetColumn(i);
      memcpy(agg_rec + pos, col->buf(), col->encoded_length());
      pos += col->encoded_length();
    }
    Entry entry{agg_rec, pos};
    auto iter = gb_map_->find(entry);
    if (iter != gb_map_->end()) {
      agg_res_ptr = reinterpret_cast<AggResItem*>(iter->second.ptr);
      delete[] agg_rec;
    } else {
      gb_map_->insert(std::make_pair<Entry, Entry>(std::move(entry),
          std::move(Entry{agg_rec + pos,
            static_cast<uint32_t>(n_agg_results_ * sizeof(AggResItem))})));
      n_groups_ = gb_map_->size();
      agg_res_ptr = reinterpret_cast<AggResItem*>(agg_rec + pos);

      for (uint32_t i = 0; i < n_agg_results_; i++) {
        agg_res_ptr[i].type = agg_results_[i].type;
      }
    }
  } else {
    agg_res_ptr = agg_results_;
  }

  Column* col;
  uint32_t value;
  uint8_t raw_type;
  DataType type;
  bool is_unsigned;
  uint32_t reg_index;

  uint8_t raw_type2;
  DataType type2;
  bool is_unsigned2;
  uint32_t reg_index2;

  uint32_t agg_index;
  uint32_t col_index;

  uint32_t exec_pos = agg_prog_start_pos_;
  while (exec_pos < prog_len_) {
    value = prog_[exec_pos++];
    uint8_t op = (value & 0xFC000000) >> 26;
    int ret = 0;
    switch (op) {
      case kOpPlus:
        raw_type = (value & 0x03E00000) >> 21;
        raw_type2 = (value & 0x001F0000) >> 16;
        is_unsigned = DecodeRawType(raw_type, &type);
        is_unsigned2 = DecodeRawType(raw_type2, &type2);

        reg_index = (value & 0x0000F000) >> 12;
        reg_index2 = (value & 0x00000F00) >> 8;

        assert(registers_[reg_index].type == kTypeBigInt ||
              registers_[reg_index].type == kTypeDouble);
        assert(registers_[reg_index2].type == kTypeBigInt ||
              registers_[reg_index2].type == kTypeDouble);

        ret = RegPlusReg(registers_[reg_index], registers_[reg_index2],
                              &registers_[reg_index]);
        assert(ret == 0);
        break;

      case kOpMinus:
        raw_type = (value & 0x03E00000) >> 21;
        raw_type2 = (value & 0x001F0000) >> 16;
        is_unsigned = DecodeRawType(raw_type, &type);
        is_unsigned2 = DecodeRawType(raw_type2, &type2);

        reg_index = (value & 0x0000F000) >> 12;
        reg_index2 = (value & 0x00000F00) >> 8;

        assert(registers_[reg_index].type == kTypeBigInt ||
              registers_[reg_index].type == kTypeDouble);
        assert(registers_[reg_index2].type == kTypeBigInt ||
              registers_[reg_index2].type == kTypeDouble);

        ret = RegMinusReg(registers_[reg_index], registers_[reg_index2],
                              &registers_[reg_index]);
        assert(ret == 0);
        break;

      case kOpMul:
      case kOpDiv:
      case kOpMod:
        break;

      case kOpLoadCol:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        reg_index = (value & 0x000F0000) >> 16;
        col_index = (value & 0x0000FFFF);

        col = rec->GetColumn(col_index);
        assert(type == CeilType(col->type()) &&
            col->raw_length() == sizeof(Register::value));

        registers_[reg_index].type = type;
        registers_[reg_index].is_unsigned = is_unsigned;
        switch (type) {
          case kTypeBigInt:
            registers_[reg_index].value.val_int64 = longlongget(col->data());
            break;
          case kTypeDouble:
            registers_[reg_index].value.val_double = doubleget(col->data());
          default:
            break;
        }
        break;

      case kOpStore:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        reg_index = (value & 0x000F0000) >> 16;
        agg_index = (value & 0x0000FFFF);
        assert(type == registers_[reg_index].type);
        ret = RegStoreToAggResItem(registers_[reg_index],
                      &agg_res_ptr[agg_index]);
        assert(ret == 0);
        break;


      case kOpCount:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        agg_index = (value & 0x0000FFFF);
        assert(type == agg_results_[agg_index].type);
        // TODO(zhao) Uint
        ret = RegIncrToAggResItem(Register{kTypeBigInt, 1},
                      &agg_res_ptr[agg_index]);
        assert(ret == 0);
        break;

       case kOpSum:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        reg_index = (value & 0x000F0000) >> 16;
        agg_index = (value & 0x0000FFFF);
        assert(type == agg_results_[agg_index].type);

        ret = RegIncrToAggResItem(registers_[reg_index],
                      &agg_res_ptr[agg_index]);

        assert(ret == 0);
        break;

      default:
        break;
    }
  }
  return true;
}

void AggInterpreter::Print() {
  if (n_gb_cols_) {
    if (gb_map_) {
      printf("Group by columns: [");
      for (int i = 0; i < n_gb_cols_; i++) {
        printf("%u ", gb_cols_[i]);
      }
      printf("]\n");

      for (auto iter = gb_map_->begin(); iter != gb_map_->end(); iter++) {
        printf("Group [%p, %u], Aggregation result: [",
            reinterpret_cast<void*>(iter->first.ptr), iter->first.len);
        AggResItem* item = reinterpret_cast<AggResItem*>(iter->second.ptr);
        for (int i = 0; i < n_agg_results_; i++) {
          switch (item[i].type) {
            case kTypeBigInt:
              printf("(kTypeBigInt: %ld) ", item[i].value.val_int64);
              break;
            // case kTypeBigUint:
            //   printf("(kTypeBigUint: %lu) ", item[i].value.val_uint64);
            //   break;

            case kTypeDouble:
              printf("(kTypeDouble: %lf) ", item[i].value.val_double);
              break;
            default:
              assert(0);
          }
        }
        printf("]\n");
      }
    }
  } else {
    printf("Aggregation result: [");
    AggResItem* item = agg_results_;
    for (int i = 0; i < n_agg_results_; i++) {
      switch (item[i].type) {
        case kTypeBigInt:
          printf("(kTypeBigInt: %ld) ", item[i].value.val_int64);
          break;
        // case kTypeBigUint:
        //   printf("(kTypeBigUint: %lu) ", item[i].value.val_uint64);
        //   break;
        case kTypeDouble:
          printf("(kTypeDouble: %lf) ", item[i].value.val_double);
          break;
        default:
          assert(0);
      }
    }
    printf("]\n");
  }
}

