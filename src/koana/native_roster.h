#pragma once

#include<cstdint>
#include<cstdlib>
#include<cstring>

#include "dave/common.h"

#include "common.h"

using namespace dpp::dave;

// represents a wrapper around a std::map<uint64_t, std::vector<uint8_t>> used to keep track of users in the MLS group.
struct native_roster
{
    uint64_t* keys;
    uint8_t** values;
    int32_t* valueLengths;
    int32_t length;
    int32_t error;

public:
    native_roster(roster_map map)
    {
        this->error = 0;
        this->length = map.size();

        this->keys = (uint64_t*)malloc(8 * this->length);
        this->valueLengths = (int32_t*)malloc(4 * this->length);
        this->values = (uint8_t**)malloc(8 * this->length);

        if (!this->values || !this->valueLengths || !this->keys)
        {
            this->error = 1;
            return;
        }

        int i = 0;

        for (const auto& [key, vec] : map)
        {
            this->keys[i] = key;

            uint8_t* data = (uint8_t*)malloc(vec.size());

            if (!data)
            {
                this->error = 1;
                return;
            }

            memcpy(data, &vec[0], vec.size());
            this->values[i] = data;
            this->valueLengths[i] = vec.size();

            ++i;
        }

        if (++i != this->length)
        {
            this->error = 2;
            return;
        }
    }

    ~native_roster()
    {
        free(this->keys);
        free(this->valueLengths);

        for (int i = 0; i < this->length; ++i)
        {
            free(this->values[i]);
        }

        free(this->values);
    }
};

// destroys the roster
k_export void koana_destroy_roster(native_roster* roster)
{
    delete roster;
}