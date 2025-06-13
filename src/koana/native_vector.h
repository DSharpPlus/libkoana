#pragma once

#include<cstdint>
#include<cstring>
#include<vector>

#include "common.h"

namespace koana
{

struct native_vector
{
    uint8_t* data;
    int32_t length;
    int32_t oom;

public:
    native_vector(std::vector<uint8_t> vec)
    {
        this->length = vec.size();
        this->oom = 0;
        this->data = (uint8_t*)malloc(this->length);

        if (!this->data)
        {
            this->oom = 1;
            return;
        }

        memcpy(this->data, &vec[0], vec.size());
    }

    ~native_vector()
    {
        free(this->data);
    }
};

// destroys the given vector
k_export void koana_destroy_vector(native_vector* vector)
{
    delete vector;
}

} // namespace