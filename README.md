# vita-kbl-loader

PSP2 kbl loader (Non-secure).

# What is this plugin

A plugin that executes nskbl saved on the memory card and reboot the software.

# Installation

**1**. Copy the decrypted `nskbl.bin` to one of the paths.

`host0:data/kbl-loader/nskbl.bin` for devkit

`sd0:data/kbl-loader/nskbl.bin` for advanced user (If installed the Manufacturing mode)

`ux0:data/kbl-loader/nskbl.bin` for end user

**2**. If you need software reboot with enso, Copy the `enso_second.bin` to one of the paths.

`host0:data/kbl-loader/enso_second.bin` for devkit

`sd0:data/kbl-loader/enso_second.bin` for advanced user (If installed the Manufacturing mode)

`ux0:data/kbl-loader/enso_second.bin` for end user

note : We developers need to have enso_second.bin for enso and non enso. for sector redirect patch.

**3**. Run kbl-loader.skprx.

# Development note

NSKBL has a different offset for each fw, so adjustment is required when using it with 3.65 etc.

Like enso 3.60 and 3.65.

Also because it needs cleanup, not yet compatible with "dumped" NSKBL.

Also, if you accidentally execute broken code and get stuck with a software reboot, you can easily recover by resetting the hardware because the original NSKBL etc, will be loaded.

# Building

Define `TARGET_FW` as either `FW_360` or `FW_365` in `CMakeLists.txt`.

Define `FORCE_SD0_BOOT_PATCHES` to 1 to enable forced os0->sd0 redirection.

When building for firmware 3.65, you may need to build extra stubs (`mkdir extra && vita-libs-gen -c import_365.yml extra`) and link against resulting `.a`.

NOTE: When `FORCE_SD0_BOOT_PATCHES` is defined as 1, the following behaviour is changed in `sysstatemgr.skprx`:
    * Text format system configuration scripts are allowed.
    * `sd0:psp2config.skprx` and `ux0:psp2config.skprx` are attempted to be used as system configuration script.
    * The `MANUFACTURING_MODE` predicate always evaluates to 1.

# Information

[Resume](https://wiki.henkaku.xyz/vita/Suspend#Rebooting_with_Patches)

Check out the [original repository](https://github.com/xerpi/vita-baremetal-loader) for more details.

# Credits

Thanks to everybody who contributes to [wiki.henkaku.xyz](https://wiki.henkaku.xyz/) and helps reverse engineering the PSVita OS.

Also xerpi who created the original [vita-baremetal-loader](https://github.com/xerpi/vita-baremetal-loader).
