#pragma once

namespace LoRaDefaults {
#ifdef LORA_DEFAULT_FREQUENCY
constexpr float frequency = LORA_DEFAULT_FREQUENCY;
#else
constexpr float frequency = 868.0f;
#endif
static_assert(frequency >= 150.0f && frequency <= 960.0f, "LoRa default frequency outside supported range");

#ifdef DEVICE_TBEAM_SUPREME_S3
constexpr float tcxoVoltage = 1.6f;
#else
constexpr float tcxoVoltage = 1.8f;
#endif
}
