import os
import subprocess
Import("env")


def merge_factory_binary(source, target, env):
    build_dir = env.subst("$BUILD_DIR")

    firmware_bin   = os.path.join(build_dir, "firmware.bin")
    bootloader_bin = os.path.join(build_dir, "bootloader.bin")
    partitions_bin = os.path.join(build_dir, "partitions.bin")
    factory_bin    = os.path.join(build_dir, "firmware-factory.bin")

    framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    boot_app0_bin = os.path.join(framework_dir, "tools", "partitions", "boot_app0.bin")

    if not os.path.isfile(boot_app0_bin):
        print("WARNING: boot_app0.bin not found — skipping factory binary merge")
        return

    esptool_dir = env.PioPlatform().get_package_dir("tool-esptoolpy")
    esptool_py  = os.path.join(esptool_dir, "esptool.py")
    python      = env.subst("$PYTHONEXE")

    mcu = env.subst("$BOARD_MCU")

    # Original ESP32 bootloader sits at 0x1000; all newer chips (S3, C3, S2, C6, H2) use 0x0
    bootloader_offset = "0x1000" if mcu == "esp32" else "0x0"

    flash_mode  = env.BoardConfig().get("build.flash_mode", "dio")
    f_flash_raw = env.BoardConfig().get("build.f_flash", "80000000L")
    flash_freq  = str(int(str(f_flash_raw).rstrip("L")) // 1_000_000) + "m"

    cmd = [
        python, esptool_py,
        "--chip", mcu,
        "merge_bin",
        "--flash_mode", flash_mode,
        "--flash_freq", flash_freq,
        "--flash_size", "4MB",
        "--output", factory_bin,
        bootloader_offset, bootloader_bin,
        "0x8000",           partitions_bin,
        "0xe000",           boot_app0_bin,
        "0x10000",          firmware_bin,
    ]

    print(f"Merging factory binary ({mcu}, {flash_mode}/{flash_freq}) -> {factory_bin}")
    result = subprocess.run(cmd, cwd=build_dir)
    if result.returncode != 0:
        print("WARNING: esptool merge_bin failed — firmware-factory.bin not produced")
    else:
        size = os.path.getsize(factory_bin)
        print(f"firmware-factory.bin created ({size:,} bytes)")


env.AddPostAction("$BUILD_DIR/firmware.bin", merge_factory_binary)
