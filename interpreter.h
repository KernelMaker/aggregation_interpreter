/*
 * Copyright [2024] <Copyright Hopsworks AB>
 *
 * Author: Zhao Song
 */
#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <math.h>
#include <map>

#include "my_byteorder.h"
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
  kOpLoadCol,
  kOpStore,
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

union DataValue {
  int64_t val_int64;
  uint64_t val_uint64;
  double val_double;
  void* val_ptr;
};

typedef int32_t DataType;
struct Register {
  DataType type;
  DataValue value;
  bool is_unsigned;
  bool is_null;
};

struct AggResItem {
  DataType type;
  DataValue value;
  bool is_unsigned;
  bool inited;  // used by Min/Max
};

struct GBColInfo {
  ColumnType type;
  bool is_unsigned;
};

class AggInterpreter {
 public:
  AggInterpreter(const uint32_t* prog, uint32_t prog_len):
    prog_(prog), prog_len_(prog_len), cur_pos_(0),
    inited_(false), n_gb_cols_(0), gb_cols_(nullptr),
    n_agg_results_(0),
    agg_results_(nullptr), agg_prog_start_pos_(0),
    gb_map_(nullptr), n_groups_(0),
    gb_cols_type_inited_(false), gb_cols_info_(nullptr) {
  }
  ~AggInterpreter() {
    delete[] gb_cols_;
    delete[] agg_results_;
    if (gb_map_) {
      for (auto iter = gb_map_->begin(); iter != gb_map_->end(); iter++) {
        delete[] iter->first.ptr;
      }
      delete gb_map_;
      delete[] gb_cols_info_;
    }
  }

  bool Init();

  bool ProcessRec(Record* rec);
  void Print();

 private:
  const uint32_t* prog_;
  uint32_t prog_len_;
  uint32_t cur_pos_;
  bool inited_;
  Register registers_[kRegTotal];

  uint32_t n_gb_cols_;
  uint32_t* gb_cols_;
  uint32_t n_agg_results_;
  AggResItem* agg_results_;
  uint32_t agg_prog_start_pos_;

  std::map<Entry, Entry, EntryCmp>* gb_map_;
  uint32_t n_groups_;
  bool gb_cols_type_inited_;
  GBColInfo* gb_cols_info_;
};
#endif  // INTERPRETER_H_
