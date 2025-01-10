vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO GiiGaTeam/pybind11
    REF "smart_holder"
    SHA512 4e475c4151beec4e5bcee3809f8cb7e4e499590b8e73a884a159ca57da2a6dd037ee08f03f6ab99a67febb88873c478989569626eab2e4d0d18944e5c9c5b59c
    HEAD_REF "smart_holder"
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DPYBIND11_TEST=OFF
        # Disable all Python searching, Python required only for tests
        -DPYBIND11_NOPYTHON=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH "share/cmake/pybind11")
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
