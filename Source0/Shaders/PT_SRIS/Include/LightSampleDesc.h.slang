#pragma once

#include "LightSample.h.slang"

[[vk_binding(0, 6)]]
ConstantBuffer<LightSampleDesc, ScalarDataLayout> light_sample_desc;

[[vk_binding(1, 6)]]
RWStructuredBuffer<LightSample> light_samples_4096;