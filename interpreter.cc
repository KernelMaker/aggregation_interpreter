#include <assert.h>
#include <stdio.h>

#include "interpreter.h"

union TypeUnion {
  int64_t val_int64;
  int64_t val_uint64;
  double val_double;
};

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
  OperItem{kOpLoad, kOpLoad, nullptr, nullptr, nullptr},
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
    while(i < n_gb_cols_ && cur_pos_ < prog_len_) {
      gb_cols_[i++] = prog_[cur_pos_++];
    }

    gb_map_ = new std::map<Entry, Entry, EntryCmp>;
  }

  /*
   * 4. Get all aggregation results types
   */
  if (n_agg_results_) {

    agg_results_ = new int64_t[n_agg_results_];
    agg_res_types_ = new uint32_t[n_agg_results_];
    uint32_t i = 0;
    while (i < n_agg_results_ && cur_pos_ < prog_len_) {
      agg_res_types_[i++] = prog_[cur_pos_++];
    }
  }

  inited_ = true;
  agg_prog_start_pos_ = cur_pos_;
  memset(registers_, 0, sizeof(registers_));

  return true;
}

bool AggInterpreter::ProcessRec(Record* rec) {

  int64_t* agg_res_ptr = nullptr;

  if (n_gb_cols_) {
    uint32_t agg_rec_len = 0;
    for (uint32_t i = 0; i < n_gb_cols_; i++) {
      Column* col = rec->GetColumn(i);
      agg_rec_len += col->encoded_length();
    }
    agg_rec_len += (n_agg_results_ * sizeof(int64_t));
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
      agg_res_ptr = reinterpret_cast<int64_t*>(iter->second.ptr);
      delete[] agg_rec;
    } else {
      gb_map_->insert(std::make_pair<Entry, Entry>(std::move(entry),
          std::move(Entry{agg_rec + pos,
            static_cast<uint32_t>(n_agg_results_ * sizeof(int64_t))})));
      n_groups_ = gb_map_->size();
      agg_res_ptr = reinterpret_cast<int64_t*>(agg_rec + pos);
    }
  } else {
    agg_res_ptr = agg_results_;
  }

  Column* col;
  uint32_t value;
  uint32_t type;
  uint32_t agg_index;
  uint32_t col_index;
  uint32_t reg_index;
  uint32_t reg_index_2;
  uint32_t exec_pos = agg_prog_start_pos_;
  TypeUnion* agg_res_field_ptr = nullptr;
  TypeUnion* reg_ptr = nullptr;
  TypeUnion* reg_ptr_2 = nullptr;
  while (exec_pos < prog_len_) {
    value = prog_[exec_pos++];
    uint8_t op = (value & 0xFF000000) >> 24;
    switch(op) {
      case kOpPlus:
      case kOpMinus:
      case kOpMul:
      case kOpDiv:
      case kOpMod:
        type = (value & 0x00F00000) >> 20;
        reg_index = (value & 0x0000F000) >> 12;
        reg_index_2 = (value & 0x00000F00) >> 8;
        reg_ptr = reinterpret_cast<TypeUnion*>(&registers_[reg_index]);
        reg_ptr_2 = reinterpret_cast<TypeUnion*>(&registers_[reg_index_2]);
        switch(type) {
          case kTypeBigInt:
            (OperArray[op].FuncBigInt)(registers_[reg_index],
                registers_[reg_index_2],
                &registers_[reg_index]);
            break;

          case kTypeDouble:
            (OperArray[op].FuncDouble)(
                reg_ptr->val_double,
                reg_ptr_2->val_double,
                reinterpret_cast<double*>(&registers_[reg_index]));
            break;

          default:
            break;
        }
        break;

      case kOpLoad:
        type = (value & 0x00F00000) >> 20;
        reg_index = (value & 0x000F0000) >> 16;
        col_index = (value & 0x0000FFFF);
        col = rec->GetColumn(col_index);
        assert(type == col->type() && col->raw_length() == sizeof(int64_t));
        memcpy((char*)&registers_[reg_index], col->data(), sizeof(int64_t));
        break;

      case kOpCount:
        type = (value & 0x00F00000) >> 20;
        agg_index = (value & 0x0000FFFF);
        assert(type == agg_res_types_[agg_index]);
        (OperArray[op].FuncBigInt)(agg_res_ptr[agg_index], 1,
            &agg_res_ptr[agg_index]);
        break;

       case kOpSum:
        type = (value & 0x00F00000) >> 20;
        reg_index = (value & 0x000F0000) >> 16;
        agg_index = (value & 0x0000FFFF);
        assert(type == agg_res_types_[agg_index]);
        agg_res_field_ptr = reinterpret_cast<TypeUnion*>(&agg_res_ptr[agg_index]);
        reg_ptr = reinterpret_cast<TypeUnion*>(&registers_[reg_index]);
        switch(type) {
          case kTypeBigInt:
            (OperArray[op].FuncBigInt)(agg_res_ptr[agg_index],
                registers_[reg_index],
                &agg_res_ptr[agg_index]);
            break;

          case kTypeDouble:
            (OperArray[op].FuncDouble)(
                agg_res_field_ptr->val_double,
                reg_ptr->val_double,
                reinterpret_cast<double*>(&agg_res_ptr[agg_index]));
            break;

          default:
            break;
        }
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
        // printf("Group  %ld, Aggregation result: [",
        //     static_cast<int64_t>(*iter->first.ptr));
        printf("Group [%p, %u], Aggregation result: [",
            (void*)iter->first.ptr, iter->first.len);
        for (int i = 0; i < n_agg_results_; i++) {
          switch(agg_res_types_[i]) {
            case kTypeBigInt:
              printf("%ld ", *reinterpret_cast<int64_t*>(
                    &iter->second.ptr[i * sizeof(int64_t)]));
              break;
            case kTypeDouble:
              printf("%lf ", *reinterpret_cast<double*>(
                    &iter->second.ptr[i * sizeof(int64_t)]));
              break;
            default:
              break; 
          }
        }
        printf("]\n");
      }
    }
  } else {
  }
}

