/*
 * Copyright [2024] <Copyright Hopsworks AB>
 *
 * Author: Zhao Song
 */
#ifndef RECORD_H_
#define RECORD_H_

#include "column.h"

class Record {
 public:
  static const uint32_t n_cols = 5;
  static const uint32_t raw_length_ = 8 + 8 + 8 + 8 + 12;
  static const uint32_t encoded_length_ = raw_length_ + sizeof(uint32_t);

  Record(int64_t var_int, double var_double,
         uint64_t var_uint, double var_double2,
        const char* var_varchar, uint32_t varchar_length) {
    cols_type_[0] = kTypeBigInt;
    cols_type_[1] = kTypeDouble;
    cols_type_[2] = kTypeBigInt;
    cols_type_[3] = kTypeDouble;
    cols_type_[4] = kTypeVarchar;
    memset(buf_, 0, encoded_length_);
    uint32_t pos = 0;

    cols_[0] = new ColumnBigInt(var_int, (unsigned char*)buf_ + pos, false);
    pos += cols_[0]->raw_length();

    cols_[1] = new ColumnDouble(var_double, (unsigned char*)buf_ + pos);
    pos += cols_[1]->raw_length();

    cols_[2] = new ColumnBigInt(var_uint, (unsigned char*)buf_ + pos, true);
    pos += cols_[2]->raw_length();

    cols_[3] = new ColumnDouble(var_double2, (unsigned char*)buf_ + pos);
    pos += cols_[3]->raw_length();

    cols_[4] = new ColumnVarchar(var_varchar, varchar_length,
                               (unsigned char*)buf_ + pos);
    pos += cols_[4]->raw_length();
  }

  Column* GetColumn(int col) {
    if (col >= n_cols) {
      return nullptr;
    } else {
      return cols_[col];
    }
  }

  void Print();

 private:
  unsigned char buf_[encoded_length_];
  Column* cols_[n_cols];
  ColumnType cols_type_[n_cols];
};

#endif  // RECORD_H_
