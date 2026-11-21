#pragma once

#include <digitalcam/core/Result.h>

namespace digitalcam::virtual_camera::windows {

bool isElevated();
digitalcam::core::Result installCamera();
digitalcam::core::Result uninstallCamera();

}
