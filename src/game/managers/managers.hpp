#pragma once

// IWYU pragma: begin_exports
#ifdef CLIENT_BUILD
#include "instance_manager.hpp"

#include "asset_manager.hpp"

#include "render_manager.hpp"
#else
#include "server_instance_manager.hpp"
#endif
// IWYU pragma: end_exports