// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "rms_kernel_bfyx_opt.h"
#include "kernel_selector_utils.h"
#include <string>

namespace kernel_selector {
static constexpr size_t subgroup_size = 16;
ParamsKey RMSKernelBfyxOpt::GetSupportedKey() const {
    ParamsKey k;
    k.EnableInputDataType(Datatype::F16);
    k.EnableInputDataType(Datatype::F32);
    k.EnableOutputDataType(Datatype::F16);
    k.EnableOutputDataType(Datatype::F32);
    k.EnableInputLayout(DataLayout::bfyx);
    k.EnableInputLayout(DataLayout::bfzyx);
    k.EnableOutputLayout(DataLayout::bfyx);
    k.EnableOutputLayout(DataLayout::bfzyx);
    k.EnableTensorOffset();
    k.EnableTensorPitches();
    k.EnableBatching();
    k.EnableDifferentTypes();
    k.EnableDynamicShapesSupport();
    return k;
}

DeviceFeaturesKey RMSKernelBfyxOpt::get_required_device_features_key(const Params& params) const {
    DeviceFeaturesKey k;
    k.requires_subgroups();
    k.requires_subgroup_reduce();
    k.requires_reqd_subgroup_size();

    return k;
}

JitConstants RMSKernelBfyxOpt::GetJitConstants(const rms_params& params, DispatchData dispatchData) const {
    auto jit = Parent::GetJitConstants(params, dispatchData);

    if (params.has_dynamic_tensors()) {
        const auto& input = params.inputs[0];
        DimensionAccessHelperJit dims(input);
        std::string data_size;
        switch (params.ov_input_rank) {
            case 1 :
                data_size = dims.b();
                break;
            case 2 :
                data_size = dims.f();
                break;
            case 3 :
                data_size = dims.y();
                break;
            default:
                data_size = dims.x();
                break;
        }

        const std::string lws_0 = "get_local_size(0)";
        // It can be expected that the maximum possible itemsNum will not exceed 32
        // Therefore, in dynamic shape, stack_size including additional buffer is set to 33
        constexpr size_t stack_size = 33;
        jit.AddConstants({
            MakeJitConstant("DATA_SIZE", data_size),
            MakeJitConstant("LWS", lws_0),
            MakeJitConstant("SLM_SIZE", dispatchData.maxSlmSize),
            MakeJitConstant("STACK_SIZE", stack_size)
        });
    } else {
        jit.AddConstants({
            MakeJitConstant("ITEMS_NUM", dispatchData.itemsNum),
            MakeJitConstant("DATA_SIZE", dispatchData.dataSize),
            MakeJitConstant("LWS", dispatchData.lws[0]),
            MakeJitConstant("SLM_SIZE", dispatchData.lws[0]),
            MakeJitConstant("LEFTOVERS", dispatchData.leftovers),
            MakeJitConstant("STACK_SIZE", dispatchData.itemsNum + 1)
        });
    }
    jit.AddConstant(MakeJitConstant("SUB_GROUP_SIZE", subgroup_size));
    jit.AddConstant(MakeJitConstant("SUBGROUP_BLOCK_SIZE", dispatchData.subgroupBlockSize));

    return jit;
}

RMSKernelBase::DispatchData RMSKernelBfyxOpt::SetDefault(const rms_params& params) const {
    DispatchData dispatchData;
    const auto& input = params.inputs[0];

    auto local_mem_per_wi = 2 * BytesPerElement(input.GetDType());
    auto max_lws = std::min(params.engineInfo.maxWorkGroupSize, params.engineInfo.maxLocalMemSize / local_mem_per_wi);
    dispatchData.maxSlmSize = max_lws;
    if (!params.has_dynamic_tensors()) {
        // data size to be processed within a LWG
        switch (params.ov_input_rank) {
            case 1:
                dispatchData.dataSize = input.Batch().v;
                dispatchData.dataCount = 1;
                break;
            case 2:
                dispatchData.dataSize = input.Feature().v;
                dispatchData.dataCount = input.Batch().v;
                break;
            case 3:
                dispatchData.dataSize = input.Y().v;
                dispatchData.dataCount = input.Batch().v * input.Feature().v;
                break;
            default:
                dispatchData.dataSize = input.X().v;
                dispatchData.dataCount = input.Batch().v * input.Feature().v * input.Z().v * input.Y().v;
                break;
        }
        dispatchData.gws[0] = 1;
        dispatchData.gws[1] = dispatchData.dataCount;
        dispatchData.gws[2] = 1;

        dispatchData.lws[0] = 1;
        dispatchData.lws[1] = 1;
        dispatchData.lws[2] = 1;

        dispatchData.itemsNum = dispatchData.dataSize;
        // Compute maximum possible LWS that does not exceed device capabilities and optimizes number of global memory reads
        while ((dispatchData.itemsNum > 32 || dispatchData.lws[0] < dispatchData.itemsNum) && (2 * dispatchData.lws[0] <= max_lws)) {
            dispatchData.lws[0] *= 2;
            dispatchData.itemsNum /= 2;
        }
        dispatchData.gws[0] = dispatchData.lws[0];
        dispatchData.leftovers = dispatchData.dataSize % dispatchData.lws[0];

        if (dispatchData.itemsNum >> 3)
            dispatchData.subgroupBlockSize = 8;
        else if (dispatchData.itemsNum >> 2)
            dispatchData.subgroupBlockSize = 4;
        else if (dispatchData.itemsNum >> 1)
            dispatchData.subgroupBlockSize = 2;
        else
            dispatchData.subgroupBlockSize = 1;
    } else {
        dispatchData.subgroupBlockSize = 8;
    }
    return dispatchData;
}

bool RMSKernelBfyxOpt::Validate(const Params& p) const {
    if (!Parent::Validate(p))
        return false;

    const rms_params& params = static_cast<const rms_params&>(p);
    const auto& gamma = params.inputs[1];

    if (!gamma.is_dynamic()) {
        size_t data_size = gamma.LogicalSize();
        if (data_size < subgroup_size) {
            return false;
        }
    }
    return true;
}

KernelsData RMSKernelBfyxOpt::GetKernelsData(const Params& params) const {
    return GetCommonKernelsData(params);
}

KernelsPriority RMSKernelBfyxOpt::GetKernelsPriority(const Params& /*params*/) const {
    return FORCE_PRIORITY_7;
}
}  // namespace kernel_selector
