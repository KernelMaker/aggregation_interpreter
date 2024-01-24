#ifndef INCLUDE_RECORD_H_
#define INCLUDE_RECORD_H_

#include "column.h"

class Record {
 public:
  static const uint32_t n_cols = 3;
  static const uint32_t raw_length_ = 8 + 8 + 12;
  static const uint32_t encoded_length_ = raw_length_ + sizeof(uint32_t);

  Record(uint32_t var_int, double var_double,
        const char* var_varchar, uint32_t varchar_length) {
    cols_type_[0] = kTypeBigInt;
    cols_type_[1] = kTypeDouble;
    cols_type_[2] = kTypeVarchar;
    memset(buf_, 0, encoded_length_);
    uint32_t pos = 0;
    for (int i = 0; i < n_cols; i++) {
      switch (cols_type_[i]) {
        case kTypeBigInt:
          cols_[i] = new ColumnInteger(var_int, (char*)buf_ + pos);
          break;
        case kTypeDouble:
          cols_[i] = new ColumnDouble(var_double, (char*)buf_ + pos);
          break;
        case kTypeVarchar:
          cols_[i] = new ColumnVarchar(var_varchar, varchar_length,
                          (char*)buf_ + pos);
        default:
          break;
      }
      pos += cols_[i]->raw_length();
    }
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
  char buf_[encoded_length_];
  Column* cols_[n_cols];
  ColumnType cols_type_[n_cols];
};

#endif  // INCLUDE_RECORD_H_
