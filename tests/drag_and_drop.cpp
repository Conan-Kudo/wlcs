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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "data_device.h"
#include "helpers.h"
#include "in_process_server.h"

#include <gmock/gmock.h>

#include <memory>

using namespace testing;
using namespace wlcs;

namespace
{

auto static const any_width = 100;
auto static const any_height = 100;
auto static const any_mime_type = "AnyMimeType";
int  static const any_actions = WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY | WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
struct wl_surface* const null_icon = nullptr;

struct DnDOriginator : Client
{
    // Can't use "using Client::Client;" because Xenial
    DnDOriginator(Server& server) : Client{server} {}

    Surface surface{create_visible_surface(any_width, any_height)};
    DataDevice device{wl_data_device_manager_get_data_device(data_device_manager(), seat())};
    DataSource source{wl_data_device_manager_create_data_source(data_device_manager())};

    void offer(char const* mime_type)
    {
        wl_data_source_offer(source, mime_type);
        roundtrip();
    }

    void set_actions(int actions)
    {
        wl_data_source_set_actions(source, actions);
    }

    void start_drag(struct wl_surface* icon)
    {
        wl_data_device_start_drag(device, source, surface, icon, drag_serial);
    }

    uint32_t drag_serial{0};
};

struct MockDataDeviceListener : DataDeviceListener
{
    using DataDeviceListener::DataDeviceListener;

    MOCK_METHOD2(data_offer, void(struct wl_data_device* wl_data_device, struct wl_data_offer* id));
};

struct MockDataOfferListener : DataOfferListener
{
    using DataOfferListener::DataOfferListener;

    MOCK_METHOD2(offer, void(struct wl_data_offer* data_offer, char const* MimeType));
};

struct DnDConsumer : Client
{
    // Can't use "using Client::Client;" because Xenial
    DnDConsumer(Server& server) : Client{server} {}

    Surface surface{create_visible_surface(any_width, any_height)};
    DataDevice sink_data{wl_data_device_manager_get_data_device(data_device_manager(), seat())};
    MockDataDeviceListener listener{sink_data};
};

struct DragAndDrop : StartedInProcessServer
{
    DnDConsumer target{the_server()};
    DnDOriginator originator{the_server()};
    Pointer cursor{the_server().create_pointer()};

    MockDataOfferListener mdol;

    void SetUp() override
    {
        // We need some idea where the surfaces are
        the_server().move_surface_to(originator.surface, 0, 0);
        the_server().move_surface_to(target.surface, position_target_x, position_target_y);

        // originator needs a serial number to start the drag gesture
        bool button_down{false};
        originator.add_pointer_button_notification([&](uint32_t serial, uint32_t, bool is_down) -> bool 
            {
                originator.drag_serial = serial;
                button_down = is_down;
                return true;
            });

        cursor.left_button_down();
        originator.dispatch_until([&](){ return button_down; });

        originator.offer(any_mime_type);
        originator.set_actions(any_actions);
        originator.start_drag(null_icon);
    }

    void TearDown() override
    {
        originator.roundtrip();
        target.roundtrip();
        StartedInProcessServer::TearDown();
    }

    static const int position_target_x = any_width;
    static const int position_target_y = any_height;
    static const int somewhere_on_target_x = position_target_x + 10;
    static const int somewhere_on_target_y = position_target_y + 10;
};
}

TEST_F(DragAndDrop, given_originator_starts_drag_when_cursor_moves_over_target_it_sees_offer)
{
    EXPECT_CALL(mdol, offer(_, StrEq(any_mime_type)));
    EXPECT_CALL(target.listener, data_offer(_,_))
        .WillOnce(Invoke([&](struct wl_data_device*, struct wl_data_offer* id){ mdol.listen_to(id); }));

    cursor.move_to(somewhere_on_target_x, somewhere_on_target_y);
}
