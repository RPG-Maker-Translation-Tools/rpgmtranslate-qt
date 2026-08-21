#pragma once

#include "Hasher.hpp"
#include "Utils.hpp"

#include <glaze/glaze.hpp>

template <>
struct glz::meta<HashMap<QString, QString>> {
    static constexpr auto value =
        glz::custom<[](HashMap<QString, QString>& out, const vector<pair<QString, QString>>& input) -> void {
        out.reserve(input.size());
        for (const auto& [key, value] : input) {
            out.insert({ key, value });
        }
    }, [](const HashMap<QString, QString>& input) -> vector<pair<QString, QString>> {
        vector<pair<QString, QString>> out;
        out.reserve(input.size());
        for (const auto& [key, value] : input) {
            out.emplace_back(key, value);
        }
        return out;
    }>;
};

template <>
struct glz::meta<HashMap<FilenameArray, u64>> {
    static constexpr auto value =
        glz::custom<[](HashMap<FilenameArray, u64>& out, const std::map<string, u64>& input) -> void {
        out.reserve(input.size());
        for (const auto& [key, value] : input) {
            FilenameArray filename{};
            copyFilenameArray(key.data(), key.size(), filename.data());
            out.insert({ filename, value });
        }
    }, [](const HashMap<FilenameArray, u64>& input) -> std::map<string, u64> {
        std::map<string, u64> out;
        for (const auto& [key, value] : input) {
            out[string(key.data())] = value;
        }
        return out;
    }>;
};

template <>
struct glz::meta<QString> {
    static constexpr auto value = glz::custom<[](QString& out, const string& input) -> void {
        out = QString::fromStdString(input);
    }, [](const QString& input) -> string { return input.toStdString(); }>;
};