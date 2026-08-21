#include "OllamaGod.h"

#include "WorldState.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <utility>

namespace {

constexpr const char* kSystemPrompt =
    "You are an unseen God in a 2D maze game. Reply with ONE JSON object only (no markdown):\n"
    "{\"action\":\"none|send_message|spawn_enemy|teleport_player|regenerate_maze\","
    "\"parameter\":\"\",\"message\":\"\",\"power_cost\":0,\"favor_delta\":0,\"reason\":\"\"}\n"
    "Rules:\n"
    "- Prefer action none unless intervention is justified.\n"
    "- Never spend more than god.power. Costs: send_message 0, spawn_enemy 1, "
    "teleport_player 2, regenerate_maze 4.\n"
    "- spawn_enemy parameter: random | near_player | far_from_player\n"
    "- teleport_player parameter: random_safe | dead_end | far_from_exit | near_enemy\n"
    "- favor_delta must be an integer from -5 to 5.\n"
    "- If trigger is shrine and player_message is set, usually answer with send_message.\n"
    "- If trigger is ambient, intervene sparingly.\n"
    "- Keep message short when speaking.";

void godLog(const std::string& line) {
    std::cerr << "[OllamaGod] " << line << std::endl;
}

}  // namespace

OllamaGod::OllamaGod(std::string host, int port, std::string model)
    : host_(std::move(host)), port_(port), model_(std::move(model)) {
    godLog("ready host=" + host_ + ":" + std::to_string(port_) + " model=" + model_);
}

OllamaGod::~OllamaGod() {
    cancel();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void OllamaGod::beginEvaluate(
    const WorldState& world, std::string_view trigger, std::string_view playerMessage) {
    if (busy_.exchange(true)) {
        godLog("beginEvaluate ignored; already busy");
        return;
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    cancelRequested_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.reset();
    }

    const nlohmann::json payload = toJson(world, trigger, playerMessage);
    godLog(
        std::string("request trigger=") + std::string(trigger) +
        " player_message=" + (playerMessage.empty() ? "(none)" : std::string(playerMessage)));
    worker_ = std::thread(&OllamaGod::workerMain, this, payload.dump());
}

bool OllamaGod::isBusy() const {
    return busy_.load();
}

bool OllamaGod::tryTakeDecision(GodDecision& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!pending_) {
        return false;
    }
    out = *pending_;
    pending_.reset();
    return true;
}

void OllamaGod::cancel() {
    cancelRequested_ = true;
    std::shared_ptr<httplib::Client> client;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        client = client_;
    }
    if (client) {
        client->stop();
    }
}

void OllamaGod::finishWith(GodDecision decision) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_ = decision;
        client_.reset();
    }
    busy_ = false;
}

void OllamaGod::workerMain(std::string payloadJson) {
    GodDecision decision;

    if (cancelRequested_) {
        godLog("cancelled before send");
        finishWith(decision);
        return;
    }

    nlohmann::json messages = nlohmann::json::array();
    messages.push_back({{"role", "system"}, {"content", kSystemPrompt}});
    messages.push_back({{"role", "user"}, {"content", std::move(payloadJson)}});

    nlohmann::json request = {
        {"model", model_},
        {"stream", true},
        {"format", "json"},
        {"messages", messages},
    };

    auto client = std::make_shared<httplib::Client>(host_, port_);
    // Local LLM first-token latency can be long; keep the socket alive between chunks.
    client->set_connection_timeout(5, 0);
    client->set_read_timeout(120, 0);
    client->set_write_timeout(10, 0);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        client_ = client;
    }

    std::string streamBuffer;
    std::string content;

    const auto processLines = [&]() {
        size_t pos = 0;
        while (true) {
            const size_t nl = streamBuffer.find('\n', pos);
            if (nl == std::string::npos) {
                break;
            }
            std::string line = streamBuffer.substr(pos, nl - pos);
            pos = nl + 1;
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                continue;
            }
            try {
                const nlohmann::json chunk = nlohmann::json::parse(line);
                if (chunk.contains("error") && chunk["error"].is_string()) {
                    godLog("ollama error chunk: " + chunk["error"].get<std::string>());
                }
                if (chunk.contains("message") && chunk["message"].is_object()) {
                    const auto& msg = chunk["message"];
                    if (msg.contains("content") && msg["content"].is_string()) {
                        content += msg["content"].get<std::string>();
                    }
                }
                // Some Ollama builds also emit top-level response fragments.
                if (chunk.contains("response") && chunk["response"].is_string()) {
                    content += chunk["response"].get<std::string>();
                }
            } catch (const std::exception& ex) {
                godLog(std::string("chunk parse skip: ") + ex.what());
            }
        }
        if (pos > 0) {
            streamBuffer.erase(0, pos);
        }
    };

    httplib::Request req;
    req.method = "POST";
    req.path = "/api/chat";
    req.set_header("Content-Type", "application/json");
    req.body = request.dump();
    req.content_receiver = [&](const char* data, size_t dataLength, uint64_t, uint64_t) {
        if (cancelRequested_) {
            return false;
        }
        streamBuffer.append(data, dataLength);
        processLines();
        return true;
    };

    godLog("POST /api/chat ...");
    const auto result = client->send(req);

    if (cancelRequested_) {
        godLog("cancelled during/after request");
    } else if (!result) {
        godLog(std::string("HTTP failed: ") + httplib::to_string(result.error()));
    } else {
        godLog("HTTP status=" + std::to_string(result->status) + " bytes_content=" + std::to_string(content.size()));
        if (result->status >= 400) {
            godLog("body: " + result->body.substr(0, 400));
        }
    }

    if (!cancelRequested_ && !streamBuffer.empty()) {
        streamBuffer.push_back('\n');
        processLines();
    }

    if (!cancelRequested_ && !content.empty()) {
        godLog("raw model content: " + content);
        if (!parseGodDecisionJson(content, decision)) {
            godLog("failed to parse GodDecision JSON; treating as none");
            decision = GodDecision{};
        } else {
            godLog(
                "parsed action=" + std::to_string(static_cast<int>(decision.action)) +
                " parameter=" + decision.parameter + " message=" + decision.message +
                " favor_delta=" + std::to_string(decision.favorDelta));
        }
    } else if (!cancelRequested_) {
        godLog("empty model content");
    }

    finishWith(decision);
}
