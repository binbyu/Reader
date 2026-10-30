#pragma once
#include "Types.h"
#include "cJSON.h"

class JsonSerialize
{
public:
    static bool enserialize(const Data& data, std::vector<uint8_t>& bytes);
    static bool deserialize(const std::vector<uint8_t>& bytes, Data& data);

    static bool enserialize(const std::vector<BookSource>& bookSrcs, std::vector<uint8_t>& bytes);
    static bool deserialize(const std::vector<uint8_t>& bytes, std::vector<BookSource>& bookSrcs);

    static bool enserialize(const VersionList& list, std::vector<uint8_t>& bytes);
    static bool deserialize(const std::vector<uint8_t>& bytes, VersionList& list);

    static bool enserialize(const WebShelfItem& data, std::vector<uint8_t>& bytes);
    static bool deserialize(const std::vector<uint8_t>& bytes, WebShelfItem& data);

    static bool enserialize(const WebShelfList& list, std::vector<uint8_t>& bytes);
    static bool deserialize(const std::vector<uint8_t>& bytes, WebShelfList& list);

    static bool enserialize(const WebChapterList& list, std::vector<uint8_t>& bytes);
    static bool deserialize(const std::vector<uint8_t>& bytes, WebChapterList& list);

    static bool enserialize(const WebContent& data, std::vector<uint8_t>& bytes);
    static bool deserialize(const std::vector<uint8_t>& bytes, WebContent& data);

    static bool enserialize(const WebSaveBookProgress& data, std::vector<uint8_t>& bytes);
    static bool deserialize(const std::vector<uint8_t>& bytes, WebSaveBookProgress& data);

private:
    static bool deserializeFromOldBookSource(cJSON* bookSources, std::vector<BookSource>& bookSrcs);
};