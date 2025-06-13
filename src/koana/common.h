#pragma once

#ifdef _MSC_VER
    #define k_export extern "C" __declspec(dllexport)
#else
    #define k_export extern "C"
#endif

namespace koana
{

enum koana_error
{
    success = 0,
    out_of_memory = 1,

    encryption_failure = 100,

    mls_commit_reset_error = 1000,
    mls_commit_ignorable_error = 1001,
};

} // namespace