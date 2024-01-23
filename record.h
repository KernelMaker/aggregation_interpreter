#include "column.h"


class Record {
 public:
  static const uint32_t n_cols = 3;
  static const uint32_t raw_length_ = 8 + 8 + 12;
  static const uint32_t encoded_length_ = raw_length_ + sizeof(uint32_t);

  Record(uint32_t var_int, double var_double,
        const char* var_varchar, uint32_t varchar_length) {
    cols_type_[0] = kTypeInteger;
    cols_type_[1] = kTypeDouble;
    cols_type_[2] = kTypeVarchar;
    memset(buf_, 0, encoded_length_);
    uint32_t pos = 0;
    for (int i = 0; i < n_cols; i++) {
      switch (cols_type_[i]) {
        case kTypeInteger:
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

  void Print() {
    printf("------Record------\n");
    for (int i = 0; i < n_cols; i++) {
      switch (cols_type_[i]) {
        case kTypeInteger:
          printf("  column [%u], raw_length: %u, encoded_length: %u, "
                 "type: INTEGER, value: %ld\n", i,
                 cols_[i]->raw_length(), cols_[i]->encoded_length(),
                 *reinterpret_cast<const int64_t* >(cols_[i]->data()));
          break;
        case kTypeDouble:
          printf("  column [%u], raw_length: %u, encoded_length: %u, "
                 "type: DOUBLE, value: %lf\n", i,
                 cols_[i]->raw_length(), cols_[i]->encoded_length(),
                 *reinterpret_cast<const double* >(cols_[i]->data()));
          break;
        case kTypeVarchar:
          printf("  column [%u], raw_length: %u, encoded_length: %u, "
                 "type: VARCHAR, value: %12s\n", i,
                 cols_[i]->raw_length(), cols_[i]->encoded_length(),
                 cols_[i]->data());
        default:
          break;
      }
    }
    printf("------------------\n");
  }

  
 private:
  char buf_[encoded_length_];
  Column* cols_[n_cols];
  ColumnType cols_type_[n_cols];
};
