/**
 * triplet_challenge is an application that extracts the top 3 triplet words of a file
 *
 * Copyright (C) 2018 Pablo Marcos Oltra
 *
 * This file is part of triplet_challenge.
 *
 * triplet_challenge is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * triplet_challenge is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with triplet_challenge.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "triplet_challenge.h"

#include <unordered_map>
#include <thread>
#include <vector>
#include <atomic>

//#define logDebug(...) fprintf(stderr, "debug: " __VA_ARGS__)
#define logDebug(...)

bool shouldSkipCharacter(const char c) {
    return !(std::isalnum(c) || c == '\'');
}

std::size_t findGeneric(std::string_view buffer, FindType type) {
    auto method = [type](auto c) {
        if (type == FindType::FIRST_CHARACTER) {
            return shouldSkipCharacter(c);
        } else {
            return !shouldSkipCharacter(c);
        }
    };

    std::size_t offset = 0;
    for (const auto& c : buffer) {
        if (method(c)) {
            offset++;
        } else {
            break;
        }
    }

    return offset;
}

std::size_t findFirstCharacter(std::string_view buffer) {
    return findGeneric(buffer, FindType::FIRST_CHARACTER);
}

std::size_t findFirstNonCharacter(std::string_view buffer) {
    return findGeneric(buffer, FindType::FIRST_NON_CHARACTER);
}

std::size_t jumpNextWord(std::string_view buffer) {
    // Find the current word and store it in the variable
    auto offset = findFirstNonCharacter(buffer);

    // Skip characters until the beginning of the next word
    if (offset <= buffer.size()) {
        buffer = buffer.substr(offset);
    }
    offset += findFirstCharacter(buffer);

    return offset;
}

// Ensures the buffer is filled with lowercase words separated only with whitespaces
// Stores into `words` the number of words processed
// Returns the new length of the buffer which is always less or equal than the original length
std::size_t sanitizeBuffer(char* buffer, const std::size_t length, std::size_t& words) {
    char* read = buffer;
    char* write = buffer;
    words = 0;

    while (read < buffer + length) {
        const char c = *read;
        read++;
        std::string_view sv{read, static_cast<std::size_t>(buffer + length - read)};
        const auto readOffset = findFirstCharacter(sv);

        if (!shouldSkipCharacter(c)) {
            *write =  std::tolower(c);
            write++;
        }

        read += readOffset;
        if (readOffset > 0 || shouldSkipCharacter(c)) {
            *write = ' ';
            write++;
            words++;
        }
    }
    words++;

    return write - buffer;
}

static std::size_t g_numThreads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 1;
using TripletMap = std::unordered_map<std::string_view, std::atomic_uint>;

void calculateTripletsForOneThread(size_t threadNumber, std::string_view buffer, TripletMap& map) {
    (void)threadNumber;
    std::size_t offset = 0;

    logDebug("[%zu] Buffer passed with size %zu bytes\n", threadNumber, buffer.size());

    while (offset < buffer.size()) {
        const auto secondWordOffset = offset + jumpNextWord(buffer.substr(offset, buffer.size() - offset));
        const auto thirdWordOffset = secondWordOffset + jumpNextWord(buffer.substr(secondWordOffset, buffer.size() - secondWordOffset));
        const auto tripletEndOffset = thirdWordOffset + findFirstNonCharacter(buffer.substr(thirdWordOffset, buffer.size() - thirdWordOffset));
        std::string_view tripletKey = buffer.substr(offset, tripletEndOffset - offset);

        logDebug("[%zu] Triplet extracted: %.*s\n", threadNumber, static_cast<int>(tripletKey.size()), tripletKey.data());

        if (offset != secondWordOffset && thirdWordOffset != secondWordOffset && tripletEndOffset != thirdWordOffset) {
            auto count = map[tripletKey].fetch_add(1) + 1;
            (void)count;
            logDebug("[%zu] Increasing triplet \"%.*s\" count to %u\n", threadNumber, static_cast<int>(tripletKey.size()), tripletKey.data(), count);
        }

        offset = secondWordOffset;
    }
}

TripletResult calculateTriplets(char* buffer, std::size_t length) {
    TripletResult result;
    std::size_t startOffset, endOffset = 0;
    std::size_t numberOfWords = 0;
    std::vector<std::unique_ptr<std::thread>> threads(g_numThreads);

    logDebug("Buffer passed with size %zu bytes\n", length);
    length = sanitizeBuffer(buffer, length, numberOfWords);

    // We need to ensure that the hash map will never be resized to avoid regenerating the whole table
    const auto maxNumberOfBuckets = numberOfWords - 2;
    TripletMap map(maxNumberOfBuckets);
    map.max_load_factor(maxNumberOfBuckets);

    std::string_view text{buffer, length};
    std::size_t chunkSize = text.size() / g_numThreads;

    // Split buffer into chunks of similar size
    for (std::size_t nThread = 0; nThread < g_numThreads; ++nThread) {
        startOffset = endOffset;
        endOffset += chunkSize;
        std::size_t chunkOffset = endOffset;
        if (endOffset < text.size()) {
            endOffset += jumpNextWord(text.substr(endOffset, text.size() - endOffset));
            chunkOffset = endOffset;
            for (std::size_t i = 0; i < 2; ++i) {
                chunkOffset += jumpNextWord(text.substr(chunkOffset, text.size() - chunkOffset));
            }
        } else {
            endOffset = text.size();
            chunkOffset = endOffset;
        }
        const auto bufferSize = chunkOffset - startOffset;
        if (bufferSize > 0) {
            auto threadBuffer = text.substr(startOffset, bufferSize);
            logDebug("Chunk for thread %zu: %.*s\n", nThread, static_cast<int>(threadBuffer.size()), threadBuffer.data());
            threads[nThread] = std::make_unique<std::thread>([nThread, threadBuffer, &map] {
                calculateTripletsForOneThread(nThread, threadBuffer, map);
            });
        }
    }

    for (auto& thread : threads) {
        if (thread != nullptr) {
            thread->join();
        }
    }

    // Take the 3 triplets with higher count
    for (const auto& value : map) {
        const auto& tripletKey = value.first;
        const auto& count = value.second.load();

        for (std::size_t i = 0; i < result.size(); ++i) {
            if (count > result[i].count) {
                auto triplet = Triplet{std::string{tripletKey}, count};
                logDebug("Updating top triplet \"%.*s\" with count %u\n", static_cast<int>(tripletKey.size()), tripletKey.data(), count);

                for (auto j = i; j < result.size(); ++j) {
                    triplet = std::exchange(result[j], triplet);
                    if (triplet.words == tripletKey)
                        break;
                }

                /*for (const auto& triplet : result) {
                        logDebug("\t\t\t%s:%zu\n", triplet.words.c_str(), triplet.count);
                    }*/
                break;
            }
        }
    }

    fprintf(stderr, "Used %zu threads\n"
                    "Number of words: %zu\n"
                    "Number of triplets: %zu\n"
                    "Map state: bucket_count %zu, max_bucket_count %zu, load_factor %f\n\n",
            g_numThreads, numberOfWords, map.size(), map.bucket_count(), map.max_bucket_count(), map.load_factor());

    return result;
}
