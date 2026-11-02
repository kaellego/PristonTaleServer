#pragma once

#include "Shared/datatypes.h"

#include <optional>
#include <string>
#include <vector>
#include <mutex>

class CharacterFileRepository final {
public:
    explicit CharacterFileRepository(const std::string& dataPath);

    std::optional<CharacterData> load(const std::string& characterName) const;
    bool exists(const std::string& characterName) const;
    bool save(const CharacterData& character) const;
    bool remove(const std::string& characterName) const;
    std::vector<std::string> names() const;

private:
    void migrateLegacyCharacters();

    std::string m_characterPath;
    mutable std::mutex m_mutex;
};
