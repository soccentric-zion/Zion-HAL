#include "server/module_registry_all.h"
#include "server/typed_module_handler.h"

// ============================================================
// HAL module headers  (from src/ — included via the acu_full_test include path)
// ============================================================
#include "audio_interfaces/audio_interfaces.h"
#include "camera_interfaces_lib/camera_interfaces_lib.h"
#include "camera_module3_lib/camera_module3_lib.h"
#include "clock_and_timing_lib/clock_and_timing_lib.h"
#include "communication_interface_lib/communication_interface_lib.h"
#include "cpu_lib/cpu_lib.h"
#include "debug_and_development_interfaces/debug_and_development_interfaces.h"
#include "deep_learning_accelerator_lib/deep_learning_accelerator_lib.h"
#include "display_interfaces/display_interfaces.h"
#include "environmental_lib/environmental_lib.h"
#include "ethernet_lib/ethernet_lib.h"
#include "ethernet_switch_lib/ethernet_switch_lib.h"
#include "external_connector_lib/external_connector_lib.h"
#include "functional_safety_lib/functional_safety_lib.h"
#include "gnss_lib/gnss_lib.h"
#include "gpio_lib/gpio_lib.h"
#include "gpu_lib/gpu_lib.h"
#include "imu_lib/imu_lib.h"
#include "mcu_interface_lib/mcu_interface_lib.h"
#include "pcie_lib/pcie_lib.h"
#include "poe_hat_lib/poe_hat_lib.h"
#include "power_management_lib/power_management_lib.h"
#include "reliability_lib/reliability_lib.h"
#include "security_features_lib/security_features_lib.h"
#include "sense_hat_lib/sense_hat_lib.h"
#include "serializer_deserializer_lib/serializer_deserializer_lib.h"
#include "software_interface_lib/software_interface_lib.h"
#include "storage_lib/storage_lib.h"
#include "system_integration_lib/system_integration_lib.h"
#include "system_memory_lib/system_memory_lib.h"
#include "thermal_management_lib/thermal_management_lib.h"
#include "usb_lib/usb_lib.h"
#include "video_encode_decode_lib/video_encode_decode_lib.h"
#include "vision_accelerator_lib/vision_accelerator_lib.h"
#include "watchdog_and_reset_lib/watchdog_and_reset_lib.h"

// ============================================================
// Generated proto headers  (from ${PROTO_BINARY_DIR}/modules/ — via hal_protos include path)
// ============================================================
#include "modules/audio_interfaces.pb.h"
#include "modules/camera_interfaces.pb.h"
#include "modules/camera_module3.pb.h"
#include "modules/clock_and_timing.pb.h"
#include "modules/communication_interface.pb.h"
#include "modules/cpu.pb.h"
#include "modules/debug_and_development_interfaces.pb.h"
#include "modules/deep_learning_accelerator.pb.h"
#include "modules/display_interfaces.pb.h"
#include "modules/environmental.pb.h"
#include "modules/ethernet.pb.h"
#include "modules/ethernet_switch.pb.h"
#include "modules/external_connector.pb.h"
#include "modules/functional_safety.pb.h"
#include "modules/gnss.pb.h"
#include "modules/gpio.pb.h"
#include "modules/gpu.pb.h"
#include "modules/imu.pb.h"
#include "modules/mcu_interface.pb.h"
#include "modules/pcie.pb.h"
#include "modules/poe_hat.pb.h"
#include "modules/power_management.pb.h"
#include "modules/reliability.pb.h"
#include "modules/security_features.pb.h"
#include "modules/sense_hat.pb.h"
#include "modules/serializer_deserializer.pb.h"
#include "modules/software_interface.pb.h"
#include "modules/storage.pb.h"
#include "modules/system_integration.pb.h"
#include "modules/system_memory.pb.h"
#include "modules/thermal_management.pb.h"
#include "modules/usb.pb.h"
#include "modules/video_encode_decode.pb.h"
#include "modules/vision_accelerator.pb.h"
#include "modules/watchdog_and_reset.pb.h"

namespace hal {
namespace server {

// Convenience macro: register one TypedModuleHandler in three lines.
// Args: registry ptr, module-id string, HAL class, version int, source path,
//       proto package namespace (without leading ::).
#define HAL_REGISTER(reg, id, HalClass, ver, src, PkgNs)           \
    (reg)->Register((id),                                           \
        std::make_unique<TypedModuleHandler<                        \
            HalClass,                                               \
            PkgNs::Config,                                          \
            PkgNs::Result>>((id), (ver), (src)))

std::unique_ptr<ModuleRegistry> CreateDefaultRegistry() {
    auto reg = std::make_unique<ModuleRegistry>();

    HAL_REGISTER(reg, "audio_interfaces",
                 AudioInterfaces, 1,
                 "src/audio_interfaces/audio_interfaces.h",
                 hal::modules::audio_interfaces);

    HAL_REGISTER(reg, "camera_interfaces",
                 CameraInterfacesTesting, 1,
                 "src/camera_interfaces_lib/camera_interfaces_lib.h",
                 hal::modules::camera_interfaces);

    HAL_REGISTER(reg, "camera_module3",
                 CameraModule3Testing, 1,
                 "src/camera_module3_lib/camera_module3_lib.h",
                 hal::modules::camera_module3);

    HAL_REGISTER(reg, "clock_and_timing",
                 ClockAndTimingTesting, 1,
                 "src/clock_and_timing_lib/clock_and_timing_lib.h",
                 hal::modules::clock_and_timing);

    HAL_REGISTER(reg, "communication_interface",
                 CommunicationInterfaceTesting, 1,
                 "src/communication_interface_lib/communication_interface_lib.h",
                 hal::modules::communication_interface);

    HAL_REGISTER(reg, "cpu",
                 CpuTesting, 1,
                 "src/cpu_lib/cpu_lib.h",
                 hal::modules::cpu);

    HAL_REGISTER(reg, "debug_and_development_interfaces",
                 DebugAndDevelopmentInterfaces, 1,
                 "src/debug_and_development_interfaces/debug_and_development_interfaces.h",
                 hal::modules::debug_and_development_interfaces);

    HAL_REGISTER(reg, "deep_learning_accelerator",
                 DeepLearningAcceleratorTesting, 1,
                 "src/deep_learning_accelerator_lib/deep_learning_accelerator_lib.h",
                 hal::modules::deep_learning_accelerator);

    HAL_REGISTER(reg, "display_interfaces",
                 DisplayInterfaces, 1,
                 "src/display_interfaces/display_interfaces.h",
                 hal::modules::display_interfaces);

    HAL_REGISTER(reg, "environmental",
                 EnvironmentalTesting, 1,
                 "src/environmental_lib/environmental_lib.h",
                 hal::modules::environmental);

    HAL_REGISTER(reg, "ethernet",
                 EthernetTesting, 1,
                 "src/ethernet_lib/ethernet_lib.h",
                 hal::modules::ethernet);

    HAL_REGISTER(reg, "ethernet_switch",
                 EthernetSwitchTesting, 1,
                 "src/ethernet_switch_lib/ethernet_switch_lib.h",
                 hal::modules::ethernet_switch);

    HAL_REGISTER(reg, "external_connector",
                 ExternalConnectorTesting, 1,
                 "src/external_connector_lib/external_connector_lib.h",
                 hal::modules::external_connector);

    HAL_REGISTER(reg, "functional_safety",
                 FunctionalSafetyTesting, 1,
                 "src/functional_safety_lib/functional_safety_lib.h",
                 hal::modules::functional_safety);

    HAL_REGISTER(reg, "gnss",
                 GnssTesting, 1,
                 "src/gnss_lib/gnss_lib.h",
                 hal::modules::gnss);

    HAL_REGISTER(reg, "gpio",
                 GpioTesting, 1,
                 "src/gpio_lib/gpio_lib.h",
                 hal::modules::gpio);

    HAL_REGISTER(reg, "gpu",
                 GpuTesting, 1,
                 "src/gpu_lib/gpu_lib.h",
                 hal::modules::gpu);

    HAL_REGISTER(reg, "imu",
                 ImuTesting, 1,
                 "src/imu_lib/imu_lib.h",
                 hal::modules::imu);

    HAL_REGISTER(reg, "mcu_interface",
                 McuInterfaceTesting, 1,
                 "src/mcu_interface_lib/mcu_interface_lib.h",
                 hal::modules::mcu_interface);

    HAL_REGISTER(reg, "pcie",
                 PcieTesting, 1,
                 "src/pcie_lib/pcie_lib.h",
                 hal::modules::pcie);

    HAL_REGISTER(reg, "poe_hat",
                 PoeHatTesting, 1,
                 "src/poe_hat_lib/poe_hat_lib.h",
                 hal::modules::poe_hat);

    HAL_REGISTER(reg, "power_management",
                 PowerManagementTesting, 1,
                 "src/power_management_lib/power_management_lib.h",
                 hal::modules::power_management);

    HAL_REGISTER(reg, "reliability",
                 ReliabilityTesting, 1,
                 "src/reliability_lib/reliability_lib.h",
                 hal::modules::reliability);

    HAL_REGISTER(reg, "security_features",
                 SecurityFeaturesTesting, 1,
                 "src/security_features_lib/security_features_lib.h",
                 hal::modules::security_features);

    HAL_REGISTER(reg, "sense_hat",
                 SenseHatTesting, 1,
                 "src/sense_hat_lib/sense_hat_lib.h",
                 hal::modules::sense_hat);

    HAL_REGISTER(reg, "serializer_deserializer",
                 SerializerDeserializerTesting, 1,
                 "src/serializer_deserializer_lib/serializer_deserializer_lib.h",
                 hal::modules::serializer_deserializer);

    HAL_REGISTER(reg, "software_interface",
                 SoftwareInterfaceTesting, 1,
                 "src/software_interface_lib/software_interface_lib.h",
                 hal::modules::software_interface);

    HAL_REGISTER(reg, "storage",
                 StorageTesting, 1,
                 "src/storage_lib/storage_lib.h",
                 hal::modules::storage);

    HAL_REGISTER(reg, "system_integration",
                 SystemIntegrationTesting, 1,
                 "src/system_integration_lib/system_integration_lib.h",
                 hal::modules::system_integration);

    HAL_REGISTER(reg, "system_memory",
                 SystemMemoryTesting, 1,
                 "src/system_memory_lib/system_memory_lib.h",
                 hal::modules::system_memory);

    HAL_REGISTER(reg, "thermal_management",
                 ThermalManagementTesting, 1,
                 "src/thermal_management_lib/thermal_management_lib.h",
                 hal::modules::thermal_management);

    HAL_REGISTER(reg, "usb",
                 UsbTesting, 1,
                 "src/usb_lib/usb_lib.h",
                 hal::modules::usb);

    HAL_REGISTER(reg, "video_encode_decode",
                 VideoEncodeDecodeTesting, 1,
                 "src/video_encode_decode_lib/video_encode_decode_lib.h",
                 hal::modules::video_encode_decode);

    HAL_REGISTER(reg, "vision_accelerator",
                 VisionAcceleratorTesting, 1,
                 "src/vision_accelerator_lib/vision_accelerator_lib.h",
                 hal::modules::vision_accelerator);

    HAL_REGISTER(reg, "watchdog_and_reset",
                 WatchdogAndResetTesting, 1,
                 "src/watchdog_and_reset_lib/watchdog_and_reset_lib.h",
                 hal::modules::watchdog_and_reset);

#undef HAL_REGISTER

    return reg;
}

} // namespace server
} // namespace hal
