/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "action_parser.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <regex>

namespace uitars {

ActionParser::ActionParser(std::shared_ptr<Logger> logger) 
    : logger_(logger) {
}

std::vector<ParsedAction> ActionParser::parse(const std::string& prediction) {
    std::vector<ParsedAction> actions;
    
    std::istringstream ss(prediction);
    std::string line;
    std::string currentThought;
    
    while (std::getline(ss, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        // Check for Thought:
        if (line.find("Thought:") == 0 || line.find("thought:") == 0) {
            size_t pos = line.find(":");
            currentThought = line.substr(pos + 1);
            currentThought.erase(0, currentThought.find_first_not_of(" \t"));
            continue;
        }
        
        // Check for Action:
        if (line.find("Action:") != std::string::npos || 
            line.find("action:") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                std::string actionStr = line.substr(pos + 1);
                actionStr.erase(0, actionStr.find_first_not_of(" \t"));
                actionStr.erase(actionStr.find_last_not_of(" \t") + 1);
                
                ParsedAction action = parseAction(actionStr);
                action.thought = currentThought;
                actions.push_back(action);
            }
        }
    }
    
    return actions;
}

ParsedAction ActionParser::parseAction(const std::string& actionStr) {
    ParsedAction action;
    
    // Find action type (before parenthesis)
    size_t parenPos = actionStr.find("(");
    if (parenPos != std::string::npos) {
        action.actionTypeStr = actionStr.substr(0, parenPos);
        // Trim
        action.actionTypeStr.erase(0, action.actionTypeStr.find_first_not_of(" \t"));
        action.actionTypeStr.erase(action.actionTypeStr.find_last_not_of(" \t") + 1);
        
        action.actionType = stringToActionType(action.actionTypeStr);
        
        // Extract parameters
        size_t closeParenPos = actionStr.rfind(")");
        if (closeParenPos > parenPos) {
            std::string params = actionStr.substr(parenPos + 1, closeParenPos - parenPos - 1);
            
            // Parse key=value pairs with regex
            std::regex paramRegex(R"((\w+)\s*=\s*'([^']*)')");
            std::smatch match;
            std::string::const_iterator searchStart(params.cbegin());
            
            while (std::regex_search(searchStart, params.cend(), match, paramRegex)) {
                std::string key = match[1].str();
                std::string value = match[2].str();
                action.actionInputs[key] = value;
                searchStart = match.suffix().first;
            }
            
            // Also try double quotes
            std::regex paramRegex2(R"((\w+)\s*=\s*\"([^\"]*)\")");
            searchStart = params.cbegin();
            while (std::regex_search(searchStart, params.cend(), match, paramRegex2)) {
                std::string key = match[1].str();
                std::string value = match[2].str();
                if (action.actionInputs.find(key) == action.actionInputs.end()) {
                    action.actionInputs[key] = value;
                }
                searchStart = match.suffix().first;
            }
        }
    } else {
        action.actionTypeStr = actionStr;
        action.actionType = stringToActionType(actionStr);
    }
    
    logger_->debug("Parsed action: " + action.actionTypeStr + " with " + 
                   std::to_string(action.actionInputs.size()) + " inputs");
    
    return action;
}

Box ActionParser::extractBox(const std::string& boxStr) {
    Box box = {0, 0, 0, 0};
    
    // Parse "[x1, y1, x2, y2]" format
    std::regex boxRegex(R"(\[\s*(\d+(?:\.\d+)?)\s*,\s*(\d+(?:\.\d+)?)\s*,\s*(\d+(?:\.\d+)?)\s*,\s*(\d+(?:\.\d+)?)\s*\])");
    std::smatch match;
    
    if (std::regex_search(boxStr, match, boxRegex)) {
        box.x1 = static_cast<int>(std::round(std::stod(match[1].str())));
        box.y1 = static_cast<int>(std::round(std::stod(match[2].str())));
        box.x2 = static_cast<int>(std::round(std::stod(match[3].str())));
        box.y2 = static_cast<int>(std::round(std::stod(match[4].str())));
    }
    
    return box;
}

Point ActionParser::parseBoxToScreenCoords(const std::string& boxStr, int screenWidth, int screenHeight) {
    Box box = extractBox(boxStr);
    
    // If coordinates are normalized (0-1000 range), convert to screen coordinates
    double x1 = box.x1;
    double y1 = box.y1;
    double x2 = box.x2;
    double y2 = box.y2;
    
    // Check if normalized (typical VLM output uses 0-1000 scale)
    if (x1 <= 1000 && y1 <= 1000 && x2 <= 1000 && y2 <= 1000) {
        x1 = (x1 / 1000.0) * screenWidth;
        y1 = (y1 / 1000.0) * screenHeight;
        x2 = (x2 / 1000.0) * screenWidth;
        y2 = (y2 / 1000.0) * screenHeight;
    }
    
    // Return center point
    int centerX = static_cast<int>((x1 + x2) / 2);
    int centerY = static_cast<int>((y1 + y2) / 2);
    
    logger_->debug("Box " + boxStr + " -> screen coords (" + 
                   std::to_string(centerX) + ", " + std::to_string(centerY) + ")");
    
    return {centerX, centerY};
}

std::string ActionParser::extractKey(const std::string& keyStr) {
    // Remove quotes and trim
    std::string result = keyStr;
    result.erase(std::remove(result.begin(), result.end(), '\''), result.end());
    result.erase(std::remove(result.begin(), result.end(), '"'), result.end());
    result.erase(0, result.find_first_not_of(" \t"));
    result.erase(result.find_last_not_of(" \t") + 1);
    return result;
}

std::string ActionParser::extractContent(const std::string& contentStr) {
    return extractKey(contentStr);  // Same logic
}

std::string ActionParser::extractDirection(const std::string& dirStr) {
    std::string result = extractKey(dirStr);
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

} // namespace uitars
