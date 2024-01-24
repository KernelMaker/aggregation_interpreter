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
  explicit Column(char* buf, uint32_t raw_length, uint32_t encoded_length)
    : buf_(buf), raw_length_(raw_length), encoded_length_(encoded_length),
    type_(kTypeUnknown) {
    }

  virtual ~Column() {}

  const char* buf() {
    return buf_;
  }
  virtual const char* data() = 0;

  uint32_t raw_length() {
    return raw_length_;
  }

  uint32_t encoded_length() {
    return encoded_length_;
  }

  ColumnType type() {
    return type_;
  }

 protected:
  char* buf_;
  uint32_t raw_length_;
  uint32_t encoded_length_;
  ColumnType type_;
};

class ColumnInteger : public Column {
 public:
  explicit ColumnInteger(int64_t value, char* buf)
    : Column(buf, sizeof(value), sizeof(value)) {
      *(reinterpret_cast<int64_t* >(buf_)) = value;
      type_ = kTypeBigInt;
    }

  ~ColumnInteger() override {
  }

  const char* data() override {
    return buf_;
  }
};

class ColumnDouble : public Column {
 public:
  explicit ColumnDouble(double value, char* buf)
    : Column(buf, sizeof(value), sizeof(value)) {
      *(reinterpret_cast<double* >(buf_)) = value;
      type_ = kTypeDouble;
    }

  ~ColumnDouble() override {
  }

  const char* data() override {
    return buf_;
  }
};

class ColumnVarchar : public Column {
 public:
  explicit ColumnVarchar(const char* buffer, uint32_t buffer_len, char* buf)
    : Column(buf, buffer_len, sizeof(uint32_t) + buffer_len) {
      memcpy(buf_,
          reinterpret_cast<char* >(&raw_length_), sizeof(raw_length_));
      memcpy(buf_ + sizeof(raw_length_), buffer, buffer_len);
      buf_[encoded_length_ - 1] = '\0';
      type_ = kTypeVarchar;
    }

  const char* data() override {
    return buf_ + sizeof(raw_length_);
  }
};

#endif  // INCLUDE_COLUMN_H_
