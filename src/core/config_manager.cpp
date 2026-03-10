#include "sspm/module.h"
#include <map>
#include <string>

namespace sspm {

class DefaultConfigManager : public ConfigManager {
public:
    std::string get(const std::string& key, const std::string& default_val) const override {
        auto it = config_.find(key);
        return it != config_.end() ? it->second : default_val;
    }

    void set(const std::string& key, const std::string& val) override {
        config_[key] = val;
    }

    std::map<std::string, std::string> get_all_for_module(const std::string& module_id) const override {
        std::map<std::string, std::string> res;
        std::string prefix = module_id + ".";
        for (auto& [k, v] : config_) {
            if (k.starts_with(prefix)) {
                res[k.substr(prefix.length())] = v;
            }
        }
        return res;
    }

private:
    std::map<std::string, std::string> config_;
};

} // namespace sspm
