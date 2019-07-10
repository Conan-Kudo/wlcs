/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "layer_shell_v1.h"

wlcs::LayerSurfaceV1::LayerSurfaceV1(wlcs::Client& client, wlcs::Surface& surface)
{
    if (!client.layer_shell_v1())
        throw std::runtime_error("Layer shell unstanble V1 not supported by compositor");
    layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        client.layer_shell_v1(),
        surface,
        NULL,
        ZWLR_LAYER_SHELL_V1_LAYER_TOP,
        "wlcs");
    static struct zwlr_layer_surface_v1_listener const listener {
        +[](void */*data*/,
            struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1,
            uint32_t serial,
            uint32_t /*width*/,
            uint32_t /*height*/)
            {
                zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);
            },
        +[](void */*data*/,
            struct zwlr_layer_surface_v1 */*zwlr_layer_surface_v1*/)
            {
            }
    };
    zwlr_layer_surface_v1_add_listener(layer_surface, &listener, this);
}

wlcs::LayerSurfaceV1::~LayerSurfaceV1()
{
    zwlr_layer_surface_v1_destroy(layer_surface);
}
