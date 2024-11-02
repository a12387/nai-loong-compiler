#pragma once

#include <unordered_map>
#include "koopa.h"
#include <string.h>

void addItemToSlice(koopa_raw_slice_t &slice, void *item);
koopa_raw_type_kind_t *createTypeKind(koopa_raw_type_tag_t tag);
koopa_raw_value_data_t *createValueData(koopa_raw_value_tag_t tag, const char *name, koopa_raw_type_t ty, koopa_raw_slice_item_kind_t used_by_kind);