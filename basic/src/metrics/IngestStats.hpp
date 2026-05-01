#pragma once

#include <cstdint>

struct IngestStats
{
    uint64_t filesDiscovered = 0;
    uint64_t filesOpened = 0;
    uint64_t filesFailed = 0;

    uint64_t rowsRead = 0;
    uint64_t rowsLoaded = 0;
    uint64_t rowsParseFailed = 0;
};