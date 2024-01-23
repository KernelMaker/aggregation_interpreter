#include <assert.h>
#include <stdlib.h>
#include <cstring>
#include <stdio.h>
#include <map>

#include "record.h"

struct Entry {
 char *ptr;
 uint32_t len;
};

enum InterpreterOp {
  kOpUnknown = 0,
  kOpPlus,
  kOpMinus,
  kOpMul,
  kOpDiv,
  kOpMod,

  kOpLoad,

  kOpSum,
  kOpMax,
  kOpMin,
  kOpCount,
  
  kOpTotal
};

enum InterpreterRegisters {
  kReg1 = 0,
  kReg2,
  kReg3,
  kReg4,
  kReg5,
  kReg6,
  kReg7,
  kReg8,
  kRegTotal
};

struct OperItem {
  uint8_t op;
  uint8_t index;
  void (* FuncInteger)(int64_t, int64_t, int64_t*);
  void (*FuncDouble)(double, double, double*);
};

void PlusInteger(int64_t a, int64_t b, int64_t* res) {
  *res = a + b;
}
void PlusDouble(double a, double b, double* res) {
  *res = a + b;
}

OperItem OperArray[kOpTotal] = {
  OperItem{kOpUnknown, kOpUnknown, nullptr, nullptr},
  OperItem{kOpPlus, kOpPlus, nullptr, nullptr},
  OperItem{kOpMinus, kOpMinus, nullptr, nullptr},
  OperItem{kOpMul, kOpMul, nullptr, nullptr},
  OperItem{kOpDiv, kOpDiv, nullptr, nullptr},
  OperItem{kOpMod, kOpMod, nullptr, nullptr},
  OperItem{kOpLoad, kOpLoad, nullptr, nullptr},
  OperItem{kOpSum, kOpSum, &PlusInteger, &PlusDouble},
  OperItem{kOpMax, kOpMax, nullptr, nullptr},
  OperItem{kOpMin, kOpMin, nullptr, nullptr},
  OperItem{kOpCount, kOpCount, &PlusInteger, nullptr}
};



struct EntryCmp {
  bool operator() (const Entry& n1, const Entry& n2) const {
    uint32_t len = n1.len > n2.len ?
                    n2.len : n1.len;

    int ret = memcmp(n1.ptr, n2.ptr, len);
    if (ret == 0) {
      return n1.len < n2.len;
    } else {
      return ret < 0;
    }
  }
};

class AggInterpreter {
 public:
  AggInterpreter(const uint32_t* prog, uint32_t prog_len):
    prog_(prog), prog_len_(prog_len), cur_pos_(0),
    inited_(false), n_gb_cols_(0), gb_cols_(nullptr),
    n_agg_results_(0), agg_res_types_(nullptr),
    agg_results_(nullptr), agg_prog_start_pos_(0),
    gb_map_(nullptr), n_groups_(0) {
  }
  ~AggInterpreter() {
    delete[] gb_cols_;
    delete[] agg_results_;
    if (gb_map_) {
      for (auto iter = gb_map_->begin(); iter != gb_map_->end(); iter++) {
        delete[] iter->first.ptr;
      }
      delete gb_map_;
    }
  }
 
  bool Init() {
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
      n_groups_ = 0;
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

  bool ProcessRec(Record* rec) {
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
    uint32_t exec_pos = agg_prog_start_pos_;
    while (exec_pos < prog_len_) {
      value = prog_[exec_pos++];
      uint8_t op = (value & 0xFF000000) >> 24;
      switch(op) {
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
          (OperArray[op].FuncInteger)(agg_res_ptr[agg_index], 1,
              &agg_res_ptr[agg_index]);
          break;

         case kOpSum:
          type = (value & 0x00F00000) >> 20;
          reg_index = (value & 0x000F0000) >> 16;
          agg_index = (value & 0x0000FFFF);
          assert(type == agg_res_types_[agg_index]);
          switch(type) {
            case kTypeInteger:
              (OperArray[op].FuncInteger)(agg_res_ptr[agg_index],
                  registers_[reg_index],
                  &agg_res_ptr[agg_index]);
              break;

            case kTypeDouble:
              (OperArray[op].FuncDouble)(
                  *reinterpret_cast<double*>(&agg_res_ptr[agg_index]),
                  *reinterpret_cast<double*>(&registers_[reg_index]),
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

  void Print() {
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
              case kTypeInteger:
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
 
 private:
 const uint32_t* prog_;
 uint32_t prog_len_;
 uint32_t cur_pos_;
 bool inited_;
 int64_t registers_[kRegTotal];

 uint32_t n_gb_cols_;
 uint32_t* gb_cols_;
 uint32_t n_agg_results_;
 uint32_t* agg_res_types_;
 int64_t* agg_results_;
 uint32_t agg_prog_start_pos_;

 std::map<Entry, Entry, EntryCmp>* gb_map_;
 uint32_t n_groups_;
};
