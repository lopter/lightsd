#pragma once

struct lifxd_client {
    LIST_ENTRY(lifxd_client)    link;
};
LIST_HEAD(lifxd_client_list, lifxd_client);
