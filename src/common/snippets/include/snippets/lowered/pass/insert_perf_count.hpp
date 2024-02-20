// Copyright (C) 2018-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//
#ifdef SNIPPETS_DEBUG_CAPS

#pragma once

#include "pass.hpp"

#include "snippets/op/perf_count.hpp"

namespace ov {
namespace snippets {
namespace lowered {
namespace pass {

/**
 * @interface InsertPerfCount
 * @brief Insert PerfCountBegin node after last parameter and insert PerfCountEnd node before first result.
 *  This is a illustration transformation to enable perf count in snippets.
 *  Developers could modify this to insert perf count pairs around interested sequence of nodes.
 * @ingroup snippets
 */
class InsertPerfCount: public Pass {
public:
    OPENVINO_RTTI("InsertPerfCount", "Pass")
    InsertPerfCount(std::map<std::string, std::string> boundary_op_names);
    bool run(LinearIR& linear_ir) override;

private:
    std::map<std::string, std::string> m_boundary_op_names;
};

} // namespace pass
} // namespace lowered
} // namespace snippets
} // namespace ov
#endif  // SNIPPETS_DEBUG_CAPS
