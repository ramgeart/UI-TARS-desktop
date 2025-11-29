/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_INPUT_CONTROLLER_H
#define UI_TARS_INPUT_CONTROLLER_H

#include <string>
#include <memory>
#include <map>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

#include "types.h"
#include "logger.h"

namespace uitars {

class InputController {
public:
    InputController(std::shared_ptr<Logger> logger);
    ~InputController();

    // Mouse operations
    void moveTo(int x, int y);
    void click(int x, int y);
    void doubleClick(int x, int y);
    void rightClick(int x, int y);
    void drag(int startX, int startY, int endX, int endY);
    void scroll(int x, int y, const std::string& direction, int amount = 5);

    // Keyboard operations
    void type(const std::string& text);
    void hotkey(const std::string& keyCombo);
    void pressKey(const std::string& key);
    void releaseKey(const std::string& key);

    // Wait
    void wait(int milliseconds);

private:
    std::shared_ptr<Logger> logger_;
    Display* display_;
    std::map<std::string, KeySym> keyMap_;

    bool initX11();
    void cleanup();
    void initKeyMap();
    KeySym stringToKeySym(const std::string& key);
    void mouseClick(int button);
    void mousePress(int button);
    void mouseRelease(int button);
    void keyPress(KeySym key);
    void keyRelease(KeySym key);
};

} // namespace uitars

#endif // UI_TARS_INPUT_CONTROLLER_H
