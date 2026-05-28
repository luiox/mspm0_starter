set_project("mspmo_demo")
set_version("0.1")
set_xmakever("3.0.0")

set_languages("c11")
set_languages("cxx17")

add_rules("mode.debug", "mode.release")
-- 语法高亮没有可以用xmake project -k compile_commands
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})

-- 包含arm编译器工具链配置
includes("arm-none-eabi-custom.lua")

target("mspmo_demo")
    set_kind("binary")

    set_plat("cross")
    set_arch("arm")

    set_toolchains("arm-none-eabi-custom")
    add_rules("util.convert_bin_hex")

    ---------------------------------------------------------------------------
    -- 库配置
    local root_dir = os.scriptdir()
    local third_party_path = path.join(root_dir, "third_party")
    -- driverlib
    local driverlib_path = path.join(third_party_path, "ti", "driverlib")
    add_includedirs("third_party")
    add_files(path.join(driverlib_path, "*.c"))
    add_files(path.join(driverlib_path, "m0p/dl_interrupt.c"))
    add_files(path.join(driverlib_path, "m0p/sysctl/dl_sysctl_mspm0g1x0x_g3x0x.c"))
    -- cmsis
    add_includedirs(path.join(third_party_path, "CMSIS/Core/Include"))
    -- dsp
    local dsp_path = path.join(third_party_path, "CMSIS/DSP/Source")
    add_files(path.join(dsp_path, "**.c"))
    add_includedirs(path.join(third_party_path, "CMSIS/DSP/Include"))
    add_includedirs(path.join(third_party_path, "CMSIS/DSP/PrivateInclude"))
    -- freertos
    local freertos_path = path.join(third_party_path, "FreeRTOS/Source")
    add_includedirs(path.join(freertos_path, "include"))
    add_includedirs(path.join(freertos_path, "portable/GCC/ARM_CM0"))
    add_files(path.join(freertos_path, "*.c"))
    add_files(path.join(freertos_path, "portable/GCC/ARM_CM0/*.c"))
    add_files(path.join(freertos_path, "portable/MemMang/heap_4.c"))
    add_includedirs("src/port/freertos")

    ---------------------------------------------------------------------------
    -- 编译配置

    -- 添加芯片定义宏
    add_defines("__MSPM0G3507__")

    -- 添加启动文件
    add_files("startup_mspm0g350x_gcc.c")

    -- 链接脚本配置
    add_ldflags("-T" .. "device_linker.lds", {force = true})

    -- Map 文件配置
    add_ldflags("-Wl,-Map=" .. path.join("build", "output.map"), {force = true})

    ---------------------------------------------------------------------------
    -- 编译与链接标志
    add_cflags({
        "-mcpu=cortex-m0plus",
        "-mthumb",
        "-Os",
        "-std=c11",
        "-Wall",
        "-fdata-sections",
        "-ffunction-sections",
        "-mfloat-abi=soft"
    }, {force = true})

    add_asflags({
        "-mcpu=cortex-m0plus",
        "-mthumb",
        "-mfloat-abi=soft"
    }, {force = true})

    add_ldflags({
        "-mcpu=cortex-m0plus",
        "-mthumb",
        "-Wl,--gc-sections",
        "--specs=nosys.specs",
        "-mfloat-abi=soft",
        "-Wl,--print-memory-usage",
        "-lm"
    }, {linker = true, force = true})

    ---------------------------------------------------------------------------
    -- 5. 用户源码
    add_files("src/**.c")
    add_includedirs("src")

