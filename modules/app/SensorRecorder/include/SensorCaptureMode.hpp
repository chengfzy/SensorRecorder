#pragma once

/**
 * @brief Capture mode for sensor
 */
enum class SensorCaptureMode {
    All,   // capture all sensor readings
    Once,  // capture one sensor reading
    None   // capture nothing
};