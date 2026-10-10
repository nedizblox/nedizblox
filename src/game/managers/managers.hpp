#pragma once

// IWYU pragma: begin_exports
#ifdef CLIENT_BUILD
#include "asset_manager.hpp"

#include "render_manager.hpp"
#endif

#ifdef SERVER_BUILD
#include "server_instance_manager.hpp"
#endif
// IWYU pragma: end_exports