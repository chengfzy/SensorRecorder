#pragma once

/**
 * @brief Image save format
 */
enum class ImageSaveFormat {
    Kalibr,  // kalibr format, <timestamp(ns)>
    Index,   // index
};

// declare type
Q_DECLARE_METATYPE(ImageSaveFormat);