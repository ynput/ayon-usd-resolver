#include <cstdint>
#include <memory>
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include <spdlog/spdlog.h>

#include "AyonCppApiCrossPlatformMacros.h"
/**
 * @class AyonLogger
 * @brief Simple Logger Class that wraps around spdlog in order to expose easy logging functions \n
 *  AyonLogger::getInstance(log_File_path.json) init code \n
 *  automaticly logs to file and console
 */
class AyonLogger {
    public:
        static AyonLogger &
        getInstance(const std::string &filepath) {
            static AyonLogger instance(filepath);
            return instance;
        }

        template<typename... Args>
        void
        error(const std::string &format, const Args &... args) {
            log("error", format, args...);
        }

        template<typename... Args>
        void
        info(const std::string &format, const Args &... args) {
            log("info", format, args...);
        }

        template<typename... Args>
        void
        warn(const std::string &format, const Args &... args) {
			
            log("warn", format, args...);
        }

        template<typename... Args>
        void
        critical(const std::string &format, const Args &... args) {
            log("critical", format, args...);
        }
        void
        LogLevlInfo(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::info);
            }
            console_logger_->set_level(spdlog::level::info);
        }
        void
        LogLevlError(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::err);
            }
            console_logger_->set_level(spdlog::level::err);
        }
        void
        LogLevlWarn(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::warn);
            }
            console_logger_->set_level(spdlog::level::warn);
        }
        void
        LogLevlCritical(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::critical);
            }
            console_logger_->set_level(spdlog::level::critical);
        }
        void
        LogLevlOff(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::off);
            }
            console_logger_->set_level(spdlog::level::off);
        }

    private:
        AyonLogger(const std::string &filepath) {
            // Initialize console logger
            console_logger_ = spdlog::stdout_color_mt("console");
            console_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

            // Initialize file logger
            file_logger_ = spdlog::basic_logger_mt<spdlog::async_factory>("fileLogger", filepath.c_str());
            file_logger_->set_pattern(
                "{\"timestamp\":\"%Y-%m-%d %H:%M:%S.%e\",\"level\":\"%l\",\"Thread "
                "Id\":\"%t\",\"Process Id\":\"%P\",\"message\":\"%v\"}");
        }

        /**
         * @brief this is the core logging function that is called fomr all other ones
         *
         * @tparam Args
         * @param level the log level for spdlog to use
         * @param format the massage text itself
         * @param args  things that will be included into the massage inplace off {}
         */
        template<typename... Args>
        void
        log(const std::string &level,const std::string &massage, const Args &... args) {
            
			//std::string formattedMassage = fmt::vformat(massage, args...);
			std::string formatted_message = fmt::vformat(massage, fmt::make_format_args(args...));

			file_logger_->log(spdlog::level::from_str(level), formatted_message);
            console_logger_->log(spdlog::level::from_str(level), formatted_message);
        }
        std::shared_ptr<spdlog::logger> console_logger_;
        std::shared_ptr<spdlog::logger> file_logger_;

        // std::map<uint8_t, std::string> logLevels = {{3, "error"}, {0, "info"}, {1, "warn"}, {4, "critical"}};
        // uint8_t logLvl = 4;
};
