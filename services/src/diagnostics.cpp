/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "diagnostics.h"

#include "spark_wiring_vector.h"

#include "system_error.h"

#include <algorithm>

namespace {

using namespace spark;

class Diagnostics {
public:
    Diagnostics() :
            enabled_(0) { // The service is disabled initially
        srcs_.reserve(32);
    }

    int registerSource(const diag_source* src) {
        if (enabled_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        const int index = indexForId(src->id);
        if (index < srcs_.size() && srcs_.at(index)->id == src->id) {
            return SYSTEM_ERROR_ALREADY_EXISTS;
        }
        if (!srcs_.insert(index, src)) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        return SYSTEM_ERROR_NONE;
    }

    int enumSources(diag_enum_sources_callback callback, size_t* count, void* data) {
        if (!enabled_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (callback) {
            for (const diag_source* src: srcs_) {
                callback(src, data);
            }
        }
        if (count) {
            *count = srcs_.size();
        }
        return SYSTEM_ERROR_NONE;
    }

    int getSource(uint16_t id, const diag_source** src) {
        if (!enabled_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        const int index = indexForId(id);
        if (index >= srcs_.size() || srcs_.at(index)->id != id) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        if (src) {
            *src = srcs_.at(index);
        }
        return SYSTEM_ERROR_NONE;
    }

    int command(int cmd, void* data) {
        switch (cmd) {
#if PLATFORM_ID == 3
        case DIAG_CMD_RESET:
            srcs_.clear();
            enabled_ = 0;
            break;
#endif
        case DIAG_CMD_ENABLE:
            enabled_ = 1;
            break;
        default:
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        return SYSTEM_ERROR_NONE;
    }

private:
    Vector<const diag_source*> srcs_;
    volatile uint8_t enabled_;

    int indexForId(uint16_t id) const {
        return std::distance(srcs_.begin(), std::lower_bound(srcs_.begin(), srcs_.end(), id,
                [](const diag_source* src, uint16_t id) {
                    return (src->id < id);
                }));
    }
};

Diagnostics g_diag;

} // namespace

int diag_register_source(const diag_source* src, void* reserved) {
    return g_diag.registerSource(src);
}

int diag_enum_sources(diag_enum_sources_callback callback, size_t* count, void* data, void* reserved) {
    return g_diag.enumSources(callback, count, data);
}

int diag_get_source(uint16_t id, const diag_source** src, void* reserved) {
    return g_diag.getSource(id, src);
}

int diag_service_cmd(int cmd, void* data, void* reserved) {
    return g_diag.command(cmd, data);
}
