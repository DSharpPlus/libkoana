#pragma once

namespace dpp
{

/**
 * @brief Log levels
 */
enum loglevel {
	/**
	 * @brief Trace
	 */
	ll_trace = 0,

	/**
	 * @brief Debug
	 */
	ll_debug,

	/**
	 * @brief Information
	 */
	ll_info,

	/**
	 * @brief Warning
	 */
	ll_warning,

	/**
	 * @brief Error
	 */
	ll_error,

	/**
	 * @brief Critical
	 */
	ll_critical
};

} // namespace