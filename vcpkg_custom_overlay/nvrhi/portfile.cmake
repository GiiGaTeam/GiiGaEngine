vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO nvidia-rtx/nvrhi
    REF a3fb078d5deda744f0e9f103c734302a3a5f120c
    SHA512 762b3314af8aabdedf8c4841ba33e6ec46b0756d8d4977c08582cb2aefd95459118bba26271f6bf1b61a81ff92d81ba97e561fb2f4d70a757ef97535f819d5e6
    HEAD_REF main
)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DNVRHI_INSTALL=ON
        -DNVRHI_INSTALL_EXPORTS=ON
        -DNVRHI_WITH_VALIDATION=ON
        -DNVRHI_WITH_VULKAN=OFF
        -DNVRHI_BUILD_SHARED=OFF
        -DNVRHI_WITH_DX11=OFF
        -DNVRHI_WITH_DX12=ON
        -DNVRHI_WITH_NVAPI=OFF
        -DNVRHI_WITH_RTXMU=OFF
        -DNVRHI_WITH_AFTERMATH=OFF
        ${WINDOWS_STATIC_RUNTIME}
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/nvrhi)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")