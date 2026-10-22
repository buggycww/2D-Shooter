# CMakeList.txt : CMake project for 2D-Shooter, include source and define
# project specific logic here.
#
find_package(SDL3 REQUIRED)

# Add source to this project's executable.
add_executable (2D-Shooter "2D-Shooter.cpp" "2D-Shooter.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET 2D-Shooter PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
