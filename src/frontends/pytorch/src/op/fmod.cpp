// Copyright (C) 2018-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "openvino/frontend/pytorch/node_context.hpp"
#include "openvino/op/mod.hpp"
#include "utils.hpp"

namespace ov {
namespace frontend {
namespace pytorch {
namespace op {

OutputVector translate_fmod(const NodeContext& context) {
    // aten::fmod with schema aten::fmod.Tensor(Tensor self, Tensor other) -> Tensor
    num_inputs_check(context, 2, 3);
    auto x = context.get_input(0);
    auto y = context.get_input(1);
    align_eltwise_input_types(context, x, y, true);

    auto res = context.mark_node(std::make_shared<ov::op::v1::Mod>(x, y));
    if (!context.input_is_none(2)) {
        context.mutate_input(2, res);
    }
    return {res};
};

}  // namespace op
}  // namespace pytorch
}  // namespace frontend
}  // namespace ov