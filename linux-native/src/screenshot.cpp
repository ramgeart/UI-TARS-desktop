/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "screenshot.h"
#include <cstring>
#include <stdexcept>
#include <png.h>
#include <X11/Xresource.h>

namespace uitars {

// Base64 encoding table
static const char base64Chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

Screenshot::Screenshot(std::shared_ptr<Logger> logger) 
    : logger_(logger), display_(nullptr), root_(0), screenWidth_(0), screenHeight_(0), scaleFactor_(1.0) {
    if (!initX11()) {
        throw std::runtime_error("Failed to initialize X11");
    }
}

Screenshot::~Screenshot() {
    cleanup();
}

bool Screenshot::initX11() {
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        logger_->error("Failed to open X display");
        return false;
    }
    
    int screen = DefaultScreen(display_);
    root_ = RootWindow(display_, screen);
    screenWidth_ = DisplayWidth(display_, screen);
    screenHeight_ = DisplayHeight(display_, screen);
    
    // Try to get DPI scale factor
    char* resourceString = XResourceManagerString(display_);
    if (resourceString) {
        XrmDatabase db = XrmGetStringDatabase(resourceString);
        if (db) {
            char* type = nullptr;
            XrmValue value;
            if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
                if (value.addr) {
                    int dpi = atoi(value.addr);
                    scaleFactor_ = dpi / 96.0;
                }
            }
            XrmDestroyDatabase(db);
        }
    }
    
    logger_->info("X11 initialized: " + std::to_string(screenWidth_) + "x" + 
                  std::to_string(screenHeight_) + " @ " + std::to_string(scaleFactor_) + "x");
    
    return true;
}

void Screenshot::cleanup() {
    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

ScreenshotOutput Screenshot::capture() {
    if (!display_) {
        throw std::runtime_error("X11 not initialized");
    }
    
    XImage* image = XGetImage(display_, root_, 0, 0, screenWidth_, screenHeight_, AllPlanes, ZPixmap);
    if (!image) {
        throw std::runtime_error("Failed to capture screenshot");
    }
    
    std::vector<uint8_t> pngData = convertToPNG(image);
    std::string base64Data = encodeBase64(pngData);
    
    XDestroyImage(image);
    
    ScreenshotOutput output;
    output.imageData = std::move(pngData);
    output.base64 = std::move(base64Data);
    output.width = screenWidth_;
    output.height = screenHeight_;
    output.scaleFactor = scaleFactor_;
    
    logger_->info("Screenshot captured: " + std::to_string(output.width) + "x" + std::to_string(output.height));
    
    return output;
}

std::vector<uint8_t> Screenshot::convertToPNG(XImage* image) {
    std::vector<uint8_t> pngData;
    
    // Create PNG structures
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        throw std::runtime_error("Failed to create PNG write struct");
    }
    
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        throw std::runtime_error("Failed to create PNG info struct");
    }
    
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        throw std::runtime_error("PNG encoding error");
    }
    
    // Set up custom write function to write to vector
    png_set_write_fn(png, &pngData, 
        [](png_structp png_ptr, png_bytep data, png_size_t length) {
            auto* vec = static_cast<std::vector<uint8_t>*>(png_get_io_ptr(png_ptr));
            vec->insert(vec->end(), data, data + length);
        },
        nullptr
    );
    
    png_set_IHDR(png, info, image->width, image->height, 8,
                 PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    
    png_write_info(png, info);
    
    // Convert XImage to RGBA
    std::vector<uint8_t> rowData(image->width * 4);
    
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            
            // Extract RGB (X11 uses BGR format typically)
            rowData[x * 4 + 0] = (pixel >> 16) & 0xFF;  // R
            rowData[x * 4 + 1] = (pixel >> 8) & 0xFF;   // G
            rowData[x * 4 + 2] = pixel & 0xFF;           // B
            rowData[x * 4 + 3] = 0xFF;                   // A
        }
        png_write_row(png, rowData.data());
    }
    
    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    
    return pngData;
}

std::string Screenshot::encodeBase64(const std::vector<uint8_t>& data) {
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);
    
    size_t i = 0;
    while (i < data.size()) {
        uint32_t octet_a = i < data.size() ? data[i++] : 0;
        uint32_t octet_b = i < data.size() ? data[i++] : 0;
        uint32_t octet_c = i < data.size() ? data[i++] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        result += base64Chars[(triple >> 18) & 0x3F];
        result += base64Chars[(triple >> 12) & 0x3F];
        result += base64Chars[(triple >> 6) & 0x3F];
        result += base64Chars[triple & 0x3F];
    }
    
    // Add padding
    size_t mod = data.size() % 3;
    if (mod == 1) {
        result[result.size() - 1] = '=';
        result[result.size() - 2] = '=';
    } else if (mod == 2) {
        result[result.size() - 1] = '=';
    }
    
    return result;
}

int Screenshot::getScreenWidth() const {
    return screenWidth_;
}

int Screenshot::getScreenHeight() const {
    return screenHeight_;
}

double Screenshot::getScaleFactor() const {
    return scaleFactor_;
}

} // namespace uitars
