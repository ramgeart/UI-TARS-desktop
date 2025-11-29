/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "input_controller.h"
#include <stdexcept>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace uitars {

InputController::InputController(std::shared_ptr<Logger> logger) 
    : logger_(logger), display_(nullptr) {
    if (!initX11()) {
        throw std::runtime_error("Failed to initialize X11 for input control");
    }
    initKeyMap();
}

InputController::~InputController() {
    cleanup();
}

bool InputController::initX11() {
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        logger_->error("Failed to open X display for input");
        return false;
    }
    
    // Check if XTest extension is available
    int event_base, error_base;
    int major_version, minor_version;
    if (!XTestQueryExtension(display_, &event_base, &error_base, &major_version, &minor_version)) {
        logger_->error("XTest extension not available");
        XCloseDisplay(display_);
        display_ = nullptr;
        return false;
    }
    
    logger_->info("XTest extension initialized");
    return true;
}

void InputController::cleanup() {
    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

void InputController::initKeyMap() {
    // Standard keys
    keyMap_["return"] = XK_Return;
    keyMap_["enter"] = XK_Return;
    keyMap_["tab"] = XK_Tab;
    keyMap_["space"] = XK_space;
    keyMap_["backspace"] = XK_BackSpace;
    keyMap_["delete"] = XK_Delete;
    keyMap_["escape"] = XK_Escape;
    keyMap_["esc"] = XK_Escape;
    
    // Arrow keys
    keyMap_["up"] = XK_Up;
    keyMap_["down"] = XK_Down;
    keyMap_["left"] = XK_Left;
    keyMap_["right"] = XK_Right;
    keyMap_["arrowup"] = XK_Up;
    keyMap_["arrowdown"] = XK_Down;
    keyMap_["arrowleft"] = XK_Left;
    keyMap_["arrowright"] = XK_Right;
    
    // Modifier keys
    keyMap_["ctrl"] = XK_Control_L;
    keyMap_["control"] = XK_Control_L;
    keyMap_["shift"] = XK_Shift_L;
    keyMap_["alt"] = XK_Alt_L;
    keyMap_["meta"] = XK_Super_L;
    keyMap_["super"] = XK_Super_L;
    keyMap_["win"] = XK_Super_L;
    keyMap_["command"] = XK_Super_L;
    keyMap_["cmd"] = XK_Super_L;
    
    // Function keys
    for (int i = 1; i <= 12; i++) {
        keyMap_["f" + std::to_string(i)] = XK_F1 + i - 1;
    }
    
    // Page keys
    keyMap_["pageup"] = XK_Page_Up;
    keyMap_["pagedown"] = XK_Page_Down;
    keyMap_["page up"] = XK_Page_Up;
    keyMap_["page down"] = XK_Page_Down;
    keyMap_["home"] = XK_Home;
    keyMap_["end"] = XK_End;
    keyMap_["insert"] = XK_Insert;
    
    // Common punctuation
    keyMap_[","] = XK_comma;
    keyMap_["."] = XK_period;
    keyMap_["/"] = XK_slash;
    keyMap_["\\"] = XK_backslash;
    keyMap_["-"] = XK_minus;
    keyMap_["="] = XK_equal;
    keyMap_["["] = XK_bracketleft;
    keyMap_["]"] = XK_bracketright;
    keyMap_[";"] = XK_semicolon;
    keyMap_["'"] = XK_apostrophe;
    keyMap_["`"] = XK_grave;
}

KeySym InputController::stringToKeySym(const std::string& key) {
    std::string lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
    
    // Check key map first
    auto it = keyMap_.find(lowerKey);
    if (it != keyMap_.end()) {
        return it->second;
    }
    
    // Single character
    if (key.length() == 1) {
        char c = key[0];
        if (std::isalpha(c)) {
            return XK_a + (std::tolower(c) - 'a');
        } else if (std::isdigit(c)) {
            return XK_0 + (c - '0');
        }
    }
    
    // Try XStringToKeysym
    KeySym sym = XStringToKeysym(key.c_str());
    if (sym != NoSymbol) {
        return sym;
    }
    
    logger_->warn("Unknown key: " + key);
    return NoSymbol;
}

void InputController::moveTo(int x, int y) {
    if (!display_) return;
    
    XTestFakeMotionEvent(display_, -1, x, y, CurrentTime);
    XFlush(display_);
    
    logger_->debug("Mouse moved to (" + std::to_string(x) + ", " + std::to_string(y) + ")");
}

void InputController::mouseClick(int button) {
    if (!display_) return;
    
    XTestFakeButtonEvent(display_, button, True, CurrentTime);
    XFlush(display_);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    XTestFakeButtonEvent(display_, button, False, CurrentTime);
    XFlush(display_);
}

void InputController::mousePress(int button) {
    if (!display_) return;
    
    XTestFakeButtonEvent(display_, button, True, CurrentTime);
    XFlush(display_);
}

void InputController::mouseRelease(int button) {
    if (!display_) return;
    
    XTestFakeButtonEvent(display_, button, False, CurrentTime);
    XFlush(display_);
}

void InputController::click(int x, int y) {
    moveTo(x, y);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mouseClick(Button1);
    
    logger_->info("Click at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
}

void InputController::doubleClick(int x, int y) {
    moveTo(x, y);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mouseClick(Button1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    mouseClick(Button1);
    
    logger_->info("Double click at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
}

void InputController::rightClick(int x, int y) {
    moveTo(x, y);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mouseClick(Button3);
    
    logger_->info("Right click at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
}

void InputController::drag(int startX, int startY, int endX, int endY) {
    moveTo(startX, startY);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mousePress(Button1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Smooth drag movement
    int steps = 20;
    for (int i = 1; i <= steps; i++) {
        int x = startX + (endX - startX) * i / steps;
        int y = startY + (endY - startY) * i / steps;
        moveTo(x, y);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mouseRelease(Button1);
    
    logger_->info("Drag from (" + std::to_string(startX) + ", " + std::to_string(startY) + 
                  ") to (" + std::to_string(endX) + ", " + std::to_string(endY) + ")");
}

void InputController::scroll(int x, int y, const std::string& direction, int amount) {
    moveTo(x, y);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    int button;
    if (direction == "up") {
        button = Button4;
    } else if (direction == "down") {
        button = Button5;
    } else if (direction == "left") {
        button = 6;  // Horizontal scroll left
    } else if (direction == "right") {
        button = 7;  // Horizontal scroll right
    } else {
        logger_->warn("Unknown scroll direction: " + direction);
        return;
    }
    
    for (int i = 0; i < amount; i++) {
        mouseClick(button);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    logger_->info("Scroll " + direction + " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
}

void InputController::keyPress(KeySym key) {
    if (!display_ || key == NoSymbol) return;
    
    KeyCode keycode = XKeysymToKeycode(display_, key);
    if (keycode == 0) {
        logger_->warn("No keycode for keysym");
        return;
    }
    
    XTestFakeKeyEvent(display_, keycode, True, CurrentTime);
    XFlush(display_);
}

void InputController::keyRelease(KeySym key) {
    if (!display_ || key == NoSymbol) return;
    
    KeyCode keycode = XKeysymToKeycode(display_, key);
    if (keycode == 0) return;
    
    XTestFakeKeyEvent(display_, keycode, False, CurrentTime);
    XFlush(display_);
}

void InputController::type(const std::string& text) {
    if (!display_) return;
    
    std::string content = text;
    bool submitAfter = false;
    
    // Check for newline at end
    if (content.size() >= 2 && content.substr(content.size() - 2) == "\\n") {
        content = content.substr(0, content.size() - 2);
        submitAfter = true;
    } else if (!content.empty() && content.back() == '\n') {
        content = content.substr(0, content.size() - 1);
        submitAfter = true;
    }
    
    for (char c : content) {
        bool needShift = false;
        KeySym keysym = NoSymbol;
        
        if (std::isupper(c)) {
            needShift = true;
            keysym = XK_a + (std::tolower(c) - 'a');
        } else if (std::islower(c)) {
            keysym = XK_a + (c - 'a');
        } else if (std::isdigit(c)) {
            keysym = XK_0 + (c - '0');
        } else {
            // Handle special characters
            switch (c) {
                case ' ': keysym = XK_space; break;
                case '\t': keysym = XK_Tab; break;
                case '!': keysym = XK_1; needShift = true; break;
                case '@': keysym = XK_2; needShift = true; break;
                case '#': keysym = XK_3; needShift = true; break;
                case '$': keysym = XK_4; needShift = true; break;
                case '%': keysym = XK_5; needShift = true; break;
                case '^': keysym = XK_6; needShift = true; break;
                case '&': keysym = XK_7; needShift = true; break;
                case '*': keysym = XK_8; needShift = true; break;
                case '(': keysym = XK_9; needShift = true; break;
                case ')': keysym = XK_0; needShift = true; break;
                case '-': keysym = XK_minus; break;
                case '_': keysym = XK_minus; needShift = true; break;
                case '=': keysym = XK_equal; break;
                case '+': keysym = XK_equal; needShift = true; break;
                case '[': keysym = XK_bracketleft; break;
                case '{': keysym = XK_bracketleft; needShift = true; break;
                case ']': keysym = XK_bracketright; break;
                case '}': keysym = XK_bracketright; needShift = true; break;
                case '\\': keysym = XK_backslash; break;
                case '|': keysym = XK_backslash; needShift = true; break;
                case ';': keysym = XK_semicolon; break;
                case ':': keysym = XK_semicolon; needShift = true; break;
                case '\'': keysym = XK_apostrophe; break;
                case '"': keysym = XK_apostrophe; needShift = true; break;
                case ',': keysym = XK_comma; break;
                case '<': keysym = XK_comma; needShift = true; break;
                case '.': keysym = XK_period; break;
                case '>': keysym = XK_period; needShift = true; break;
                case '/': keysym = XK_slash; break;
                case '?': keysym = XK_slash; needShift = true; break;
                case '`': keysym = XK_grave; break;
                case '~': keysym = XK_grave; needShift = true; break;
                default:
                    logger_->warn("Unknown character: " + std::string(1, c));
                    continue;
            }
        }
        
        if (keysym == NoSymbol) continue;
        
        if (needShift) {
            keyPress(XK_Shift_L);
        }
        
        keyPress(keysym);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        keyRelease(keysym);
        
        if (needShift) {
            keyRelease(XK_Shift_L);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    if (submitAfter) {
        keyPress(XK_Return);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        keyRelease(XK_Return);
    }
    
    logger_->info("Typed: " + text);
}

void InputController::hotkey(const std::string& keyCombo) {
    if (!display_) return;
    
    std::vector<KeySym> keys;
    std::istringstream ss(keyCombo);
    std::string token;
    
    // Split by '+' or ' '
    while (std::getline(ss, token, '+')) {
        // Also handle space delimiter
        std::istringstream ss2(token);
        std::string subtoken;
        while (ss2 >> subtoken) {
            KeySym sym = stringToKeySym(subtoken);
            if (sym != NoSymbol) {
                keys.push_back(sym);
            }
        }
    }
    
    if (keys.empty()) {
        logger_->warn("No valid keys in hotkey: " + keyCombo);
        return;
    }
    
    // Press all keys
    for (const auto& key : keys) {
        keyPress(key);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    // Release all keys in reverse order
    for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
        keyRelease(*it);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    logger_->info("Hotkey: " + keyCombo);
}

void InputController::pressKey(const std::string& key) {
    KeySym sym = stringToKeySym(key);
    if (sym != NoSymbol) {
        keyPress(sym);
    }
}

void InputController::releaseKey(const std::string& key) {
    KeySym sym = stringToKeySym(key);
    if (sym != NoSymbol) {
        keyRelease(sym);
    }
}

void InputController::wait(int milliseconds) {
    logger_->info("Waiting for " + std::to_string(milliseconds) + "ms");
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

} // namespace uitars
