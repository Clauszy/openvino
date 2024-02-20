﻿// Copyright (C) 2018-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "fully_connected_kernel_base.h"

namespace kernel_selector {

class FullyConnected_bf_io_GEMM : public FullyConnectedKernelBase {
public:
    using Parent = FullyConnectedKernelBase;
    FullyConnected_bf_io_GEMM() : Parent("fully_connected_gpu_bf_io_gemm") {}

    KernelsData GetKernelsData(const Params& params, const optional_params& options) const override;
    KernelsPriority GetKernelsPriority(const Params& params, const optional_params& options) const override;
    ParamsKey GetSupportedKey() const override;

protected:
    DispatchData SetDefault(const fully_connected_params& params, int autoTuneIndex = -1, int kernel_number = 0) const override;
    JitConstants GetJitConstants(const fully_connected_params& params, const DispatchData& dispatchData) const override;
    bool Validate(const Params& p, const optional_params& o) const override;
};
}  // namespace kernel_selector
