// One resolver session with no Python in the process, the way a DCC calls the resolver.
//
// Takes the same --steps file and --out file as lib/probe.py and supports these of its operations:
// resolve, open, open_in_background, join, handshake, clear_cache and delete. A clear_cache or delete
// step also reports open_finished: whether the background open had already returned when it did.
// hammer_start (uris, threads) resolves the URIs in a loop on that many threads until hammer_stop.
// drop_cache gives a second context its own cache instance; a clear_cache or delete step with
// "context": "second" then runs on that context.

#include <pxr/pxr.h>
#include <pxr/base/js/json.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/stage.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// The exported methods this probe needs. The real header pulls in the AYON API and libmemcached headers,
// which the runner image does not have. A changed signature fails at link time. The storage only has to be
// at least as large as the real class.
class AyonUsdResolverContext {
    public:
        AyonUsdResolverContext();
        ~AyonUsdResolverContext();
        void ClearCache();
        void DropCache();
        void DeleteFromCache(const std::string &key);

    private:
        alignas(std::max_align_t) char m_storage[4096];
};

namespace {

bool
fileExists(const std::string &path) {
    return std::ifstream(path).good();
}

JsArray
runSteps(const JsArray &steps, JsObject &merged) {
    ArResolver &resolver = ArGetResolver();
    AyonUsdResolverContext first;
    AyonUsdResolverContext second;
    JsArray outcome;
    std::thread background;
    std::vector<std::thread> hammers;
    std::atomic<bool> stopHammers{false};
    std::atomic<bool> openFinished{false};
    UsdStageRefPtr backgroundStage;

    for (const JsValue &stepValue: steps) {
        const JsObject &step = stepValue.GetJsObject();
        const std::string op = step.at("op").GetString();
        JsObject entry;
        entry["op"] = JsValue(op);
        AyonUsdResolverContext &context = step.count("context") != 0 ? second : first;
        if (op == "resolve") {
            JsObject paths;
            for (const JsValue &uri: step.at("uris").GetJsArray()) {
                const std::string path = resolver.Resolve(uri.GetString()).GetPathString();
                paths[uri.GetString()] = JsValue(path);
                merged[uri.GetString()] = JsValue(path);
            }
            entry["resolved"] = JsValue(paths);
        }
        else if (op == "open") {
            UsdStage::Open(step.at("root").GetString());
        }
        else if (op == "open_in_background") {
            const std::string root = step.at("root").GetString();
            background = std::thread([root, &backgroundStage, &openFinished]() {
                backgroundStage = UsdStage::Open(root);
                openFinished = true;
            });
        }
        else if (op == "join") {
            background.join();
        }
        else if (op == "handshake") {
            const std::string path = step.at("path").GetString();
            std::ofstream(path).close();
            while (!fileExists(path + ".ack")) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
        else if (op == "clear_cache") {
            context.ClearCache();
            entry["open_finished"] = JsValue(openFinished.load());
        }
        else if (op == "drop_cache") {
            second.DropCache();
        }
        else if (op == "hammer_start") {
            std::vector<std::string> uris;
            for (const JsValue &uri: step.at("uris").GetJsArray()) {
                uris.push_back(uri.GetString());
            }
            stopHammers = false;
            for (int i = 0; i < step.at("threads").GetInt(); ++i) {
                hammers.emplace_back([uris, &resolver, &stopHammers]() {
                    while (!stopHammers) {
                        for (const std::string &uri: uris) {
                            resolver.Resolve(uri);
                        }
                    }
                });
            }
        }
        else if (op == "hammer_stop") {
            stopHammers = true;
            for (std::thread &hammer: hammers) {
                hammer.join();
            }
            hammers.clear();
        }
        else if (op == "delete") {
            context.DeleteFromCache(step.at("uri").GetString());
            entry["open_finished"] = JsValue(openFinished.load());
        }
        else {
            throw std::runtime_error("unknown step " + op);
        }
        outcome.push_back(JsValue(entry));
    }
    if (background.joinable()) {
        background.join();
    }
    return outcome;
}

}   // namespace

int
main(int argc, char **argv) {
    std::string stepsPath;
    std::string outPath;
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string name = argv[i];
        if (name == "--steps") {
            stepsPath = argv[i + 1];
        }
        else if (name == "--out") {
            outPath = argv[i + 1];
        }
    }

    JsObject result;
    result["ok"] = JsValue(false);
    try {
        std::ifstream stepsFile(stepsPath);
        JsParseError parseError;
        const JsValue steps = JsParseStream(stepsFile, &parseError);
        if (!steps.IsArray()) {
            throw std::runtime_error("bad steps file: " + parseError.reason);
        }
        JsObject merged;
        result["steps"] = JsValue(runSteps(steps.GetJsArray(), merged));
        result["resolved"] = JsValue(merged);
        result["ok"] = JsValue(true);
    }
    catch (const std::exception &error) {
        result["error"] = JsValue(std::string(error.what()));
    }

    {
        std::ofstream out(outPath);
        JsWriteToStream(JsValue(result), out);
    }
    const bool ok = result["ok"].GetBool();
    std::cout.flush();
    std::fflush(nullptr);
    // Skips static destructors: their order is not what this probe tests.
    std::_Exit(ok ? 0 : 1);
}
