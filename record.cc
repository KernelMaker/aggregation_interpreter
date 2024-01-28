/*
 * Copyright [2024] <Copyright Hopsworks AB>
 *
 * Author: Zhao Song
 */
#include <stdio.h>

#include "record.h"

void Record::Print() {
  printf("------Record------\n");
  for (int i = 0; i < n_cols; i++) {
    switch (cols_type_[i]) {
      case kTypeBigInt:
        if (cols_[i]->is_unsigned()) {
          printf("  column [%u], raw_length: %u, encoded_length: %u, "
                 "type: UNSIGNED BIGINT, value: %ld\n", i,
                 cols_[i]->raw_length(), cols_[i]->encoded_length(),
                 *reinterpret_cast<const uint64_t* >(cols_[i]->data()));
        } else {
          printf("  column [%u], raw_length: %u, encoded_length: %u, "
                 "type: BIGINT, value: %ld\n", i,
                 cols_[i]->raw_length(), cols_[i]->encoded_length(),
                 *reinterpret_cast<const int64_t* >(cols_[i]->data()));
        }
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
