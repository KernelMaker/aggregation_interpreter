#ifndef INCLUDE_COLUMN_H_
#define INCLUDE_COLUMN_H_

#include <cstring>
#include <cstdint>

enum ColumnType {
  kTypeUnknown = 0,
  kTypeTinyInt,
  kTypeSmallInt,
  kTypeMediumInt,
  kTypeInt,
  kTypeBigInt,
  kTypeFloat,
  kTypeDouble,
  kTypeVarchar
};

class Column {
 public:
  explicit Column(unsigned char* buf, uint32_t raw_length, uint32_t encoded_length)
    : buf_(buf), raw_length_(raw_length), encoded_length_(encoded_length),
    type_(kTypeUnknown), is_unsigned_(false) {
    }

  virtual ~Column() {}

  const unsigned char* buf() {
    return buf_;
  }
  virtual const unsigned char* data() = 0;

  uint32_t raw_length() {
    return raw_length_;
  }

  uint32_t encoded_length() {
    return encoded_length_;
  }

  ColumnType type() {
    return type_;
  }

  bool is_unsigned() {
    return is_unsigned_;
  }

 protected:
  unsigned char* buf_;
  uint32_t raw_length_;
  uint32_t encoded_length_;
  ColumnType type_;
  bool is_unsigned_;
};

class ColumnBigInt : public Column {
 public:
  explicit ColumnBigInt(int64_t value, unsigned char* buf, bool is_unsigned)
    : Column(buf, sizeof(value), sizeof(value)) {
      if (is_unsigned) {
        *(reinterpret_cast<uint64_t*>(buf_)) = value;
      } else {
        *(reinterpret_cast<int64_t*>(buf_)) = value;
      }
      type_ = kTypeBigInt;
      is_unsigned_ = is_unsigned;
    }

  ~ColumnBigInt() override {
  }

  const unsigned char* data() override {
    return buf_;
  }
};

class ColumnDouble : public Column {
 public:
  explicit ColumnDouble(double value, unsigned char* buf)
    : Column(buf, sizeof(value), sizeof(value)) {
      *(reinterpret_cast<double*>(buf_)) = value;
      type_ = kTypeDouble;
    }

  ~ColumnDouble() override {
  }

  const unsigned char* data() override {
    return buf_;
  }
};

class ColumnVarchar : public Column {
 public:
  explicit ColumnVarchar(const char* buffer, uint32_t buffer_len, unsigned char* buf)
    : Column(buf, buffer_len, sizeof(uint32_t) + buffer_len) {
      memcpy(buf_,
          reinterpret_cast<unsigned char*>(&raw_length_), sizeof(raw_length_));
      memcpy(buf_ + sizeof(raw_length_), buffer, buffer_len);
      buf_[encoded_length_ - 1] = '\0';
      type_ = kTypeVarchar;
    }

  const unsigned char* data() override {
    return buf_ + sizeof(raw_length_);
  }
};

#endif  // INCLUDE_COLUMN_H_
