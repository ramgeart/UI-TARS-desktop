/*
 * Copyright (c) 2025 Bytedance, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef UI_TARS_GUI_AGENT_H
#define UI_TARS_GUI_AGENT_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "types.h"
#include "vlm_client.h"
#include "screenshot.h"
#include "input_controller.h"
#include "action_parser.h"
#include "config.h"
#include "logger.h"

namespace uitars {

class GUIAgent {
public:
    GUIAgent(const Config& config);
    ~GUIAgent();

    // Run the agent with a given instruction
    bool run(const std::string& instruction);

    // Control methods
    void pause();
    void resume();
    void stop();

    // Status
    bool isRunning() const;
    bool isPaused() const;

    // Callbacks
    using DataCallback = std::function<void(const AgentData&)>;
    using ErrorCallback = std::function<void(const AgentError&)>;

    void setDataCallback(DataCallback callback);
    void setErrorCallback(ErrorCallback callback);

private:
    Config config_;
    std::unique_ptr<VLMClient> vlmClient_;
    std::unique_ptr<Screenshot> screenshot_;
    std::unique_ptr<InputController> inputController_;
    std::unique_ptr<ActionParser> actionParser_;
    std::shared_ptr<Logger> logger_;

    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> stopped_{false};
    std::mutex pauseMutex_;
    std::condition_variable pauseCV_;

    DataCallback dataCallback_;
    ErrorCallback errorCallback_;

    int loopCount_{0};
    static constexpr int MAX_LOOP_COUNT = 50;
    static constexpr int MAX_SNAPSHOT_ERR_CNT = 3;

    std::string buildSystemPrompt() const;
    bool executeAction(const ParsedAction& action, int screenWidth, int screenHeight);
    void notifyData(const AgentData& data);
    void notifyError(const AgentError& error);
};

} // namespace uitars

#endif // UI_TARS_GUI_AGENT_H
