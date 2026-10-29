# CMakeList.txt : CMake project for 2D-Shooter, include source and define
# project specific logic here.
#
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(glm REQUIRED)
find_package(SDL3_mixer REQUIRED)
find_package(tinyxml2 REQUIRED)

# Add source to this project's executable.
add_executable (2D-Shooter "2D-Shooter.cpp" "2D-Shooter.h" "AssetPaths.h" "GameObject.h" "Timer.h" "Animation.h"  "GlobalConstants.h" "Resources.h" "Resources.cpp" "GameStates.h" "tmx.h" "tmx.cpp" "HealthBar.h" "GameObjectFactory.h" "GameObjectFactory.cpp"  "ProfilingStats.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET 2D-Shooter PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
target_link_libraries(2D-Shooter PRIVATE SDL3::SDL3 SDL3_image::SDL3_image glm::glm SDL3_mixer::SDL3_mixer tinyxml2::tinyxml2)