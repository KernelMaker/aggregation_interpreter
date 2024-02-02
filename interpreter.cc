/*
 * Copyright [2024] <Copyright Hopsworks AB>
 *
 * Author: Zhao Song
 */
#include <assert.h>
#include <stdio.h>
#include <climits>
#include <utility>
#include <limits>

#include "interpreter.h"

#define INT_MIN64 (~0x7FFFFFFFFFFFFFFFLL)
#define INT_MAX64 0x7FFFFFFFFFFFFFFFLL
#define INT_MIN32 (~0x7FFFFFFFL)
#define INT_MAX32 0x7FFFFFFFL
#define UINT_MAX32 0xFFFFFFFFL
#define INT_MIN24 (~0x007FFFFF)
#define INT_MAX24 0x007FFFFF
#define UINT_MAX24 0x00FFFFFF
#define INT_MIN16 (~0x7FFF)
#define INT_MAX16 0x7FFF
#define UINT_MAX16 0xFFFF
#define INT_MIN8 (~0x7F)
#define INT_MAX8 0x7F
#define UINT_MAX8 0xFF

bool TestIfSumOverflowsUint64(uint64_t arg1, uint64_t arg2) {
  return ULLONG_MAX - arg1 < arg2;
}

void SetRegisterNull(Register* reg) {
  reg->is_null = true;
  reg->value.val_int64 = 0;
  reg->is_unsigned = false;
}

void ResetRegister(Register* reg) {
  reg->type = kTypeUnknown;
  SetRegisterNull(reg);
}

int32_t RegPlusReg(const Register& a, const Register& b, Register* res) {
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble || b.type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt && b.type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (a.is_null || b.is_null) {
    SetRegisterNull(res);
    // NULL
    return 1;
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
    bool unsigned_flag = false;
    if (res_type == kTypeBigInt) {
      unsigned_flag = (a.is_unsigned | b.is_unsigned);
    } else {
      assert(res_type == kTypeDouble);
      unsigned_flag = (a.is_unsigned & b.is_unsigned);
    }
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

  if (a.is_null || b.is_null) {
    SetRegisterNull(res);
    // NULL
    return 1;
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
    bool unsigned_flag = false;
    if (res_type == kTypeBigInt) {
      unsigned_flag = (a.is_unsigned | b.is_unsigned);
    } else {
      assert(res_type == kTypeDouble);
      unsigned_flag = (a.is_unsigned & b.is_unsigned);
    }
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

int32_t RegMulReg(const Register& a, const Register& b, Register* res) {
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble || b.type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt && b.type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (a.is_null || b.is_null) {
    SetRegisterNull(res);
    // NULL
    return 1;
  }

  if (res_type == kTypeBigInt) {
    int64_t val0 = a.value.val_int64;
    int64_t val1 = b.value.val_int64;
    int64_t res_val;
    uint64_t res_val0;
    uint64_t res_val1;

    if (val0 == 0 || val1 == 0) {
      res->type = res_type;
      return 0;
    }

    const bool a_negative = (!a.is_unsigned && val0 < 0);
    const bool b_negative = (!b.is_unsigned && val1 < 0);
    const bool res_unsigned = (a_negative == b_negative);

    if (a_negative && val0 == INT_MIN64) {
      if (val1 == 1) {
        // Check if val0 is overflow
        bool unsigned_flag = false;
        if (res_type == kTypeBigInt) {
          unsigned_flag = (a.is_unsigned | b.is_unsigned);
        } else {
          assert(res_type == kTypeDouble);
          unsigned_flag = (a.is_unsigned & b.is_unsigned);
        }
        if ((unsigned_flag && !res_unsigned && val0 < 0) ||
            (!unsigned_flag && res_unsigned &&
             (uint64_t)val0 > (uint64_t)LLONG_MAX)) {
          return -1;
        } else {
          if (unsigned_flag) {
            res->value.val_uint64 = val0;
          } else {
            res->value.val_int64 = val0;
          }
        }
        res->is_unsigned = unsigned_flag;
        res->type = res_type;
        return 0;
      }
    }
    if (b_negative && val1 == INT_MIN64) {
      if (val0 == 1) {
        // Check if val1 is overflow
        bool unsigned_flag = false;
        if (res_type == kTypeBigInt) {
          unsigned_flag = (a.is_unsigned | b.is_unsigned);
        } else {
          assert(res_type == kTypeDouble);
          unsigned_flag = (a.is_unsigned & b.is_unsigned);
        }
        if ((unsigned_flag && !res_unsigned && val1 < 0) ||
            (!unsigned_flag && res_unsigned &&
             (uint64_t)val1 > (uint64_t)LLONG_MAX)) {
          return -1;
        } else {
          if (unsigned_flag) {
            res->value.val_uint64 = val1;
          } else {
            res->value.val_int64 = val1;
          }
        }
        res->is_unsigned = unsigned_flag;
        res->type = res_type;
        return 0;
      }
    }

    if (a_negative) {
      val0 = -val0;
    }
    if (b_negative) {
      val1 = -val1;
    }

    uint32_t a0 = 0xFFFFFFFFUL & val0;
    uint32_t a1 = static_cast<uint64_t>(val0) >> 32;
    uint32_t b0 = 0xFFFFFFFFUL & val1;
    uint32_t b1 = static_cast<uint64_t>(val1) >> 32;

    if (a1 && b1) {
      // overflow
      return -1;
    }

    res_val1 = static_cast<uint64_t>(a1) * b0 + static_cast<uint64_t>(a0) * b1;
    if (res_val1 > 0xFFFFFFFFUL) {
      // overflow
      return -1;
    }

    res_val1 = res_val1 << 32;
    res_val0 = static_cast<uint64_t>(a0) * b0;
    if (TestIfSumOverflowsUint64(res_val1, res_val0)) {
      // overflow
      return -1;
    } else {
      res_val = res_val1 + res_val0;
    }

    if (a_negative != b_negative) {
      if (static_cast<uint64_t>(res_val) > static_cast<uint64_t>(LLONG_MAX)) {
        // overflow
        return -1;
      } else {
        res_val = -res_val;
      }
    }

    // Check if res_val is overflow
    bool unsigned_flag = false;
    if (res_type == kTypeBigInt) {
      unsigned_flag = (a.is_unsigned | b.is_unsigned);
    } else {
      assert(res_type == kTypeDouble);
      unsigned_flag = (a.is_unsigned & b.is_unsigned);
    }
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
    double res_val = val0 * val1;
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

int32_t RegDivReg(const Register& a, const Register& b, Register* res) {
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble || b.type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt && b.type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (a.is_null || b.is_null) {
    SetRegisterNull(res);
    // NULL
    return 1;
  }

  if (res_type == kTypeBigInt) {
    int64_t val0 = a.value.val_int64;
    int64_t val1 = b.value.val_int64;
    bool val0_negative, val1_negative, res_negative, res_unsigned;
    uint64_t uval0, uval1, res_val;

    val0_negative = !a.is_unsigned && val0 < 0;
    val1_negative = !b.is_unsigned && val1 < 0;
    res_negative = val0_negative != val1_negative;
    res_unsigned = !res_negative;

    if (val1 == 0) {
      // Divide by zero
      if (res_unsigned) {
        SetRegisterNull(res);
      } else {
        res->value.val_int64 = 0;
      }
      res->type = res_type;
      res->is_unsigned = res_unsigned;
    }

    uval0 = static_cast<uint64_t>(val0_negative &&
                      val0 != LLONG_MIN ? -val0 : val0);
    uval1 = static_cast<uint64_t>(val1_negative &&
                      val1 != LLONG_MIN ? -val1 : val1);
    res_val = uval0 / uval1;
    if (res_negative) {
      if (res_val > static_cast<uint64_t>(LLONG_MAX)) {
        // overflow
        return -1;
      } else {
        res_val = static_cast<uint64_t>(-static_cast<int64_t>(res_val));
      }
    }
    // Check if res_val is overflow
    bool unsigned_flag = false;
    if (res_type == kTypeBigInt) {
      unsigned_flag = (a.is_unsigned | b.is_unsigned);
    } else {
      assert(res_type == kTypeDouble);
      unsigned_flag = (a.is_unsigned & b.is_unsigned);
    }
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
    if (val1 == 0) {
      // Divided by zero
      SetRegisterNull(res);
    } else {
      double res_val = val0 / val1;
      if (std::isfinite(res_val)) {
        res->value.val_double = res_val;
      } else {
        // overflow
        return -1;
      }
    }
  }
  res->type = res_type;
  if (res->is_null) {
    return 1;
  }
  return 0;
}

int32_t RegModReg(const Register& a, const Register& b, Register* res) {
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble || b.type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt && b.type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (a.is_null || b.is_null) {
    SetRegisterNull(res);
    // NULL
    return 1;
  }

  if (res_type == kTypeBigInt) {
    int64_t val0 = a.value.val_int64;
    int64_t val1 = b.value.val_int64;
    bool val0_negative, val1_negative, res_unsigned;
    uint64_t uval0, uval1, res_val;

    val0_negative = !a.is_unsigned && val0 < 0;
    val1_negative = !b.is_unsigned && val1 < 0;
    res_unsigned = !val0_negative;

    if (val1 == 0) {
      // Divide by zero
      if (res_unsigned) {
        res->value.val_uint64 = 0;
      } else {
        res->value.val_int64 = 0;
      }
      res->type = res_type;
      res->is_unsigned = res_unsigned;
    }

    uval0 = static_cast<uint64_t>(val0_negative &&
                      val0 != LLONG_MIN ? -val0 : val0);
    uval1 = static_cast<uint64_t>(val1_negative &&
                      val1 != LLONG_MIN ? -val1 : val1);
    res_val = uval0 % uval1;
    res_val = res_unsigned ? res_val : -res_val;

    // Check if res_val is overflow
    bool unsigned_flag = false;
    if (res_type == kTypeBigInt) {
      unsigned_flag = (a.is_unsigned | b.is_unsigned);
    } else {
      assert(res_type == kTypeDouble);
      unsigned_flag = (a.is_unsigned & b.is_unsigned);
    }
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
    if (val1 == 0) {
      // Divided by zero
      SetRegisterNull(res);
    } else {
      res->value.val_double = std::fmod(val0, val1);
    }
  }
  res->type = res_type;
  if (res->is_null) {
    return 1;
  }
  return 0;
}

int32_t Min(const Register& a, AggResItem* res) {
  assert(res != nullptr && (a.is_null || a.type == res->type));
  // assert(res != nullptr && a.is_unsigned == res->is_unsigned);
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (a.is_null) {
    // NULL
    return 1;
  }

  if (res_type == kTypeBigInt) {
    if (!a.is_unsigned && !res->is_unsigned) {
      if (res->inited == false) {
        res->value.val_int64 = LLONG_MAX;
        res->inited = true;
      }
      res->value.val_int64 = (a.value.val_int64 < res->value.val_int64) ?
                              a.value.val_int64 : res->value.val_int64;
    } else if (a.is_unsigned && res->is_unsigned) {
      if (res->inited == false) {
        res->value.val_uint64 = ULLONG_MAX;
        res->inited = true;
      }
      res->value.val_uint64 = (a.value.val_uint64 < res->value.val_uint64) ?
                              a.value.val_uint64 : res->value.val_uint64;
    } else if (a.is_unsigned && !res->is_unsigned) {
      if (res->inited == false) {
        res->value.val_int64 = LLONG_MAX;
        res->inited = true;
      }
      if (res->value.val_int64 < 0) {
      } else {
        res->value.val_uint64 = a.value.val_uint64 <
                static_cast<uint64_t>(res->value.val_int64) ?
                a.value.val_uint64 :
                static_cast<uint64_t>(res->value.val_int64);
        res->is_unsigned = true;
      }
    } else {
      assert(!a.is_unsigned && res->is_unsigned);
      if (res->inited == false) {
        res->value.val_uint64 = LLONG_MAX;
        res->inited = true;
      }
      if (a.value.val_int64 < 0) {
        res->value.val_int64 = a.value.val_int64;
        res->is_unsigned = false;
      } else {
        res->value.val_uint64 = static_cast<uint64_t>(a.value.val_int64) <
                                res->value.val_uint64 ?
                                static_cast<uint64_t>(a.value.val_int64) :
                                res->value.val_uint64;
      }
    }
  } else {
    assert(res_type == kTypeDouble);
    res->value.val_double = (a.value.val_double < res->value.val_double) ?
                             a.value.val_double : res->value.val_double;
  }
  return 0;
}

int32_t Max(const Register& a, AggResItem* res) {
  assert(res != nullptr && (a.is_null || a.type == res->type));
  // assert(res != nullptr && a.is_unsigned == res->is_unsigned);
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (a.is_null) {
    // NULL
    return 1;
  }

  if (res_type == kTypeBigInt) {
    if (!a.is_unsigned && !res->is_unsigned) {
      if (res->inited == false) {
        res->value.val_int64 = INT_MIN64;
        res->inited = true;
      }
      res->value.val_int64 = (a.value.val_int64 > res->value.val_int64) ?
                              a.value.val_int64 : res->value.val_int64;
    } else if (a.is_unsigned && res->is_unsigned) {
      if (res->inited == false) {
        res->value.val_uint64 = 0;
        res->inited = true;
      }
      res->value.val_uint64 = (a.value.val_uint64 > res->value.val_uint64) ?
                              a.value.val_uint64 : res->value.val_uint64;
    } else if (a.is_unsigned && !res->is_unsigned) {
      if (res->inited == false) {
        res->value.val_int64 = 0;
        res->inited = true;
      }
      if (res->value.val_int64 < 0) {
        res->value.val_uint64 = a.value.val_uint64;
      } else {
        res->value.val_uint64 = a.value.val_uint64 >
                static_cast<uint64_t>(res->value.val_int64) ?
                a.value.val_uint64 :
                static_cast<uint64_t>(res->value.val_int64);
      }
      res->is_unsigned = true;
    } else {
      assert(!a.is_unsigned && res->is_unsigned);
      if (res->inited == false) {
        res->value.val_uint64 = 0;
        res->inited = true;
      }
      if (a.value.val_int64 < 0) {
      } else {
        res->value.val_uint64 = static_cast<uint64_t>(a.value.val_int64) >
                                res->value.val_uint64;
      }
    }
  } else {
    assert(res_type == kTypeDouble);
    res->value.val_double = (a.value.val_double > res->value.val_double) ?
                             a.value.val_double : res->value.val_double;
  }

  return 0;
}

int32_t Count(const Register& a, AggResItem* res) {
  assert(res != nullptr);

  if (a.is_null) {
    // NULL
    return 1;
  } else {
    res->value.val_uint64 += 1;
    res->is_unsigned = true;
  }

  return 0;
}

int32_t Sum(const Register& a, AggResItem* res) {
  DataType res_type = kTypeUnknown;
  if (a.type == kTypeDouble || res->type == kTypeDouble) {
    res_type = kTypeDouble;
  } else {
    assert(a.type == kTypeBigInt && res->type == kTypeBigInt);
    res_type = kTypeBigInt;
  }

  if (a.is_null) {
    // NULL
    return 1;
  }

  if (res_type == kTypeBigInt) {
    int64_t val0 = a.value.val_int64;
    int64_t val1 = res->value.val_int64;
    int64_t res_val = static_cast<uint64_t>(val0) + static_cast<uint64_t>(val1);
    bool res_unsigned = false;

    if (a.is_unsigned) {
      if (res->is_unsigned || val1 >= 0) {
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
      if (res->is_unsigned) {
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
    bool unsigned_flag = false;
    if (res_type == kTypeBigInt) {
      unsigned_flag = (a.is_unsigned | res->is_unsigned);
    } else {
      assert(res_type == kTypeDouble);
      unsigned_flag = (a.is_unsigned & res->is_unsigned);
    }
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
    double val1 = (res->type == kTypeDouble) ?
                     res->value.val_double :
                     ((res->is_unsigned == true) ?
                       static_cast<double>(res->value.val_uint64) :
                       static_cast<double>(res->value.val_int64));
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
    gb_cols_info_ = new GBColInfo[n_gb_cols_];

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
      agg_results_[i].inited = false;  // used by Min/Max
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
      Column* col = rec->GetColumn(gb_cols_[i]);
      agg_rec_len += col->encoded_length();
    }
    agg_rec_len += (n_agg_results_ * sizeof(AggResItem));
    char* agg_rec = new char[agg_rec_len];
    memset(agg_rec, 0, agg_rec_len);

    uint32_t pos = 0;
    for (uint32_t i = 0; i < n_gb_cols_; i++) {
      Column* col = rec->GetColumn(gb_cols_[i]);
      memcpy(agg_rec + pos, col->buf(), col->encoded_length());
      pos += col->encoded_length();
      if (!gb_cols_type_inited_) {
        gb_cols_info_[i] = {col->type(), col->is_unsigned()};
      }
    }
    gb_cols_type_inited_ = true;
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
        if (ret < 0) {
          printf("Overflow, value is out of range\n");
        }
        assert(ret >= 0);
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
        if (ret < 0) {
          printf("Overflow, value is out of range\n");
        }
        assert(ret >= 0);
        break;

      case kOpMul:
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

        ret = RegMulReg(registers_[reg_index], registers_[reg_index2],
                              &registers_[reg_index]);
        if (ret < 0) {
          printf("Overflow, value is out of range\n");
        }
        assert(ret >= 0);
        break;

      case kOpDiv:
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

        ret = RegDivReg(registers_[reg_index], registers_[reg_index2],
                              &registers_[reg_index]);
        if (ret < 0) {
          printf("Overflow, value is out of range\n");
        }
        assert(ret >= 0);
        break;

      case kOpMod:
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

        ret = RegModReg(registers_[reg_index], registers_[reg_index2],
                              &registers_[reg_index]);
        if (ret < 0) {
          printf("Overflow, value is out of range\n");
        }
        assert(ret >= 0);

        break;

      case kOpLoadCol:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        reg_index = (value & 0x000F0000) >> 16;
        col_index = (value & 0x0000FFFF);

        col = rec->GetColumn(col_index);
        assert(type == CeilType(col->type()) &&
            col->raw_length() == sizeof(Register::value));

        ResetRegister(&registers_[reg_index]);
        registers_[reg_index].type = type;
        registers_[reg_index].is_unsigned = is_unsigned;
        // TODO(zhao song): registers_[reg_index].is_null = col->is_null();
        registers_[reg_index].is_null = false;
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

      case kOpCount:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        reg_index = (value & 0x000F0000) >> 16;
        agg_index = (value & 0x0000FFFF);
        assert(agg_results_[agg_index].type == kTypeUnknown ||
               agg_results_[agg_index].type == kTypeBigInt);
        ret = Count(registers_[reg_index], &agg_res_ptr[agg_index]);
        if (ret < 0) {
          printf("Overflow, value is out of range\n");
        }
        assert(ret >= 0);
        break;

       case kOpSum:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        reg_index = (value & 0x000F0000) >> 16;
        agg_index = (value & 0x0000FFFF);
        assert(type == agg_results_[agg_index].type);

        ret = Sum(registers_[reg_index], &agg_res_ptr[agg_index]);

        if (ret < 0) {
          printf("Overflow, value is out of range\n");
        }
        assert(ret >= 0);
        break;

       case kOpMax:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        reg_index = (value & 0x000F0000) >> 16;
        agg_index = (value & 0x0000FFFF);
        assert(type == agg_results_[agg_index].type);

        ret = Max(registers_[reg_index], &agg_res_ptr[agg_index]);

        if (ret < 0) {
          printf("Overflow, value is out of range\n");
        }
        assert(ret >= 0);
        break;

       case kOpMin:
        raw_type = (value & 0x03E00000) >> 21;
        is_unsigned = DecodeRawType(raw_type, &type);
        reg_index = (value & 0x000F0000) >> 16;
        agg_index = (value & 0x0000FFFF);
        assert(type == agg_results_[agg_index].type);

        ret = Min(registers_[reg_index], &agg_res_ptr[agg_index]);

        assert(ret >= 0);
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
        if (i != n_gb_cols_ - 1) {
          printf("%u ", gb_cols_[i]);
        } else {
          printf("%u", gb_cols_[i]);
        }
      }
      printf("]\n");
      printf("Num of groups: %lu\n", gb_map_->size());
      printf("Aggregation results:\n");

      for (auto iter = gb_map_->begin(); iter != gb_map_->end(); iter++) {
        int pos = 0;
        printf("(");
        for (int i = 0; i < n_gb_cols_; i++) {
          if (gb_cols_info_[i].type == kTypeBigInt) {
            if (gb_cols_info_[i].is_unsigned) {
              if (i != n_gb_cols_ - 1) {
                printf("%15lu, ", *(uint64_t*)(iter->first.ptr + pos));
              } else {
                printf("%15lu): ", *(uint64_t*)(iter->first.ptr + pos));
              }
            } else {
              if (i != n_gb_cols_ - 1) {
                printf("%15ld, ", *(int64_t*)(iter->first.ptr + pos));
              } else {
                printf("%15ld): ", *(int64_t*)(iter->first.ptr + pos));
              }
            }
            pos += sizeof(int64_t);
          } else if (gb_cols_info_[i].type == kTypeDouble) {
            if (i != n_gb_cols_ - 1) {
              printf("%.16f, ", *(double*)(iter->first.ptr + pos));
            } else {
              printf("%.16f): ", *(double*)(iter->first.ptr + pos));
            }
            pos += sizeof(double);
          } else {
            assert(gb_cols_info_[i].type == kTypeVarchar);
            uint32_t len = *(uint32_t*)(iter->first.ptr + pos);
            pos += sizeof(uint32_t);
            if (i != n_gb_cols_ - 1) {
              printf("%15s, ", (iter->first.ptr + pos));
            } else {
              printf("%15s): ", (iter->first.ptr + pos));
            }
          }
        }

        AggResItem* item = reinterpret_cast<AggResItem*>(iter->second.ptr);
        for (int i = 0; i < n_agg_results_; i++) {
          switch (item[i].type) {
            case kTypeBigInt:
              // printf("    (kTypeBigInt: %ld)\n", item[i].value.val_int64);
              printf("[%15ld]", item[i].value.val_int64);
              break;

            case kTypeDouble:
              // printf("    (kTypeDouble: %.16f)\n", item[i].value.val_double);
              printf("[%31.16f]", item[i].value.val_double);
              break;
            default:
              assert(0);
          }
        }
        printf("\n");
      }
    }
  } else {
    AggResItem* item = agg_results_;
    for (int i = 0; i < n_agg_results_; i++) {
      switch (item[i].type) {
        case kTypeBigInt:
          // printf("    (kTypeBigInt: %ld)\n", item[i].value.val_int64);
          printf("[%15ld]", item[i].value.val_int64);
          break;

        case kTypeDouble:
          // printf("    (kTypeDouble: %.16f)\n", item[i].value.val_double);
          printf("[%31.16f]", item[i].value.val_double);
          break;
        default:
          assert(0);
      }
    }
    printf("\n");
  }
}

