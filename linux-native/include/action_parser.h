/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_ACTION_PARSER_H
#define UI_TARS_ACTION_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <regex>

#include "types.h"
#include "logger.h"

namespace uitars {

class ActionParser {
public:
    ActionParser(std::shared_ptr<Logger> logger);
    ~ActionParser() = default;

    // Parse VLM prediction into actions
    std::vector<ParsedAction> parse(const std::string& prediction);

    // Parse box string to screen coordinates
    Point parseBoxToScreenCoords(const std::string& boxStr, int screenWidth, int screenHeight);

private:
    std::shared_ptr<Logger> logger_;

    // Parse individual action
    ParsedAction parseAction(const std::string& actionStr);
    
    // Extract box coordinates from string like "[x1, y1, x2, y2]"
    Box extractBox(const std::string& boxStr);
    
    // Extract key from hotkey action
    std::string extractKey(const std::string& keyStr);
    
    // Extract content from type action
    std::string extractContent(const std::string& contentStr);
    
    // Extract direction from scroll action
    std::string extractDirection(const std::string& dirStr);
};

} // namespace uitars

#endif // UI_TARS_ACTION_PARSER_H
