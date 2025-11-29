/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_SCREENSHOT_H
#define UI_TARS_SCREENSHOT_H

#include <string>
#include <memory>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "types.h"
#include "logger.h"

namespace uitars {

class Screenshot {
public:
    Screenshot(std::shared_ptr<Logger> logger);
    ~Screenshot();

    // Capture the entire screen
    ScreenshotOutput capture();

    // Get screen dimensions
    int getScreenWidth() const;
    int getScreenHeight() const;
    double getScaleFactor() const;

private:
    std::shared_ptr<Logger> logger_;
    Display* display_;
    Window root_;
    int screenWidth_;
    int screenHeight_;
    double scaleFactor_;

    bool initX11();
    void cleanup();
    std::string encodeBase64(const std::vector<uint8_t>& data);
    std::vector<uint8_t> convertToPNG(XImage* image);
};

} // namespace uitars

#endif // UI_TARS_SCREENSHOT_H
