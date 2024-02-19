#pragma once

#include "AyonCppApiCrossPlatformMacros.h"
#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <mutex>
#include <thread>
#include <future>

struct ProfileResult {
        std::string Name;
        long long Start, End;
        uint32_t ThreadID;
};

struct InstrumentationSession {
        std::string Name;
};

class Instrumentor {
    private:
        InstrumentationSession* m_CurrentSession;
        std::ofstream m_OutputStream;
        int m_ProfileCount;
        std::mutex m_Mutex;   // Mutex for synchronization

    public:
        Instrumentor(): m_CurrentSession(nullptr), m_ProfileCount(0) {
        }

        void
        BeginSession(const std::string &name, const std::string &filepath = "results.json") {
            std::lock_guard<std::mutex> lock(m_Mutex);   // Lock mutex for critical section
            m_OutputStream.open(filepath);
            WriteHeader();
            m_CurrentSession = new InstrumentationSession{name};
        }

        void
        EndSession() {
            WriteFooter();
            m_OutputStream.close();
            delete m_CurrentSession;
            m_CurrentSession = nullptr;
            m_ProfileCount = 0;
        }

        void
        WriteProfileAsync(const ProfileResult &result) {
            std::future<void> async = std::async(std::launch::async, [this, result]() { WriteProfile(result); });
            async.get();
        }

        void
        WriteProfile(const ProfileResult &result) {
            std::lock_guard<std::mutex> lock(m_Mutex);   // Lock mutex for critical section
            if (m_ProfileCount++ > 0)
                m_OutputStream << ",";

            std::string name = result.Name;
            std::replace(name.begin(), name.end(), '"', '\'');

            m_OutputStream << "{";
            m_OutputStream << "\"cat\":\"function\",";
            m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
            m_OutputStream << "\"name\":\"" << name << "\",";
            m_OutputStream << "\"ph\":\"X\",";
            m_OutputStream << "\"pid\":0,";
            m_OutputStream << "\"tid\":" << result.ThreadID << ",";
            m_OutputStream << "\"ts\":" << result.Start;
            m_OutputStream << "}";

            m_OutputStream.flush();
        }

        void
        WriteHeader() {
            m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
            m_OutputStream.flush();
        }

        void
        WriteFooter() {
            m_OutputStream << "]}";
            m_OutputStream.flush();
        }

        static Instrumentor &
        Get() {
            static Instrumentor instance;
            return instance;
        }
};

class InstrumentationTimer {
    public:
        InstrumentationTimer(const char* name): m_Name(name), m_Stopped(false) {
            m_StartTimepoint = std::chrono::high_resolution_clock::now();
        }

        ~InstrumentationTimer() {
            if (!m_Stopped)
                Stop();
        }

        void
        Stop() {
            auto endTimepoint = std::chrono::high_resolution_clock::now();

            long long start
                = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
            long long end
                = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

            uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());

            Instrumentor::Get().WriteProfileAsync({m_Name, start, end, threadID});

            m_Stopped = true;
        }

    private:
        const char* m_Name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
        bool m_Stopped;
};
