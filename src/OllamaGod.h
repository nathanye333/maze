#pragma once

#include "God.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace httplib {
class Client;
}

class OllamaGod : public God {
public:
    OllamaGod(std::string host, int port, std::string model);
    ~OllamaGod() override;

    void beginEvaluate(
        const WorldState& world, std::string_view trigger, std::string_view playerMessage) override;
    bool isBusy() const override;
    bool tryTakeDecision(GodDecision& out) override;
    void cancel() override;

private:
    void workerMain(std::string payloadJson);
    void finishWith(GodDecision decision);

    std::string host_;
    int port_ = 11434;
    std::string model_;

    std::atomic<bool> busy_{false};
    std::atomic<bool> cancelRequested_{false};
    std::mutex mutex_;
    std::optional<GodDecision> pending_;
    std::thread worker_;
    std::shared_ptr<httplib::Client> client_;
};
