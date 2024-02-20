// Copyright (C) 2018-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "shared_test_classes/single_op/gru_sequence.hpp"

namespace ov {
namespace test {
TEST_P(GRUSequenceTest, DISABLED_Inference) {
    run();
};
}  // namespace test
}  // namespace ov
