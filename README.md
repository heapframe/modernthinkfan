# ModernThinkFan

ModernThinkFan is a Linux fan-control utility for supported Lenovo ThinkPads.

Instead of relying on Linux ACPI interfaces or hoping that the firmware exposes a usable fan-control interface, ModernThinkFan reuses Lenovo's own fan-control implementation from their Linux diagnostic software.

The program dynamically loads Lenovo's `module_fan.so` and directly calls its exported C++ functions to:

* Detect whether the current ThinkPad supports fan-speed control
* Communicate with the Embedded Controller (EC)
* Read the current fan RPM
* Set the fan to Lenovo's predefined slow, medium, high, full-speed, or automatic modes
* Handle Lenovo's model-specific fan-control logic without reimplementing the EC protocol

This means the actual EC communication and model-specific behaviour are handled by Lenovo's code rather than being reimplemented in ModernThinkFan.

## How it works

ModernThinkFan uses three files extracted from Lenovo's Linux diagnostic environment:

```text
module_fan.so
libsal.so
fan_database.sqlite
```

`module_fan.so` contains Lenovo's fan-control implementation. ModernThinkFan loads this library at runtime with `dlopen()` and resolves the required C++ functions with `dlsym()`.

The library contains an `EmbeddedControllerComponentLinux` implementation which communicates directly with the ThinkPad's Embedded Controller. Functions such as `SetSlowRotation()`, `SetHighRotation()`, `SetFullRotation()`, and `SetAutoRotation()` are then called directly from ModernThinkFan.

The accompanying `fan_database.sqlite` is used by Lenovo's software to determine whether a particular model supports fan-speed control and which model-specific control path should be used.

A Qt `QCoreApplication` is also initialised because Lenovo's module uses `QCoreApplication::applicationDirPath()` to locate `fan_database.sqlite`.

## Dependencies

### Runtime dependencies

ModernThinkFan requires:

* Linux x86-64
* Root privileges
* Qt 5 Core
* `libsqlite3`
* `module_fan.so`
* `libsal.so`
* `fan_database.sqlite`

The Lenovo libraries and database must be present in the program's working directory:

```text
lenovofan
module_fan.so
libsal.so
fan_database.sqlite
```

The program must currently be run from this directory because the Lenovo module is loaded using:

```cpp
dlopen("./module_fan.so", RTLD_NOW);
```

### Build dependencies

To compile ModernThinkFan, you need:

- CMake
- A C++ compiler with C++11 support
- Qt 5 development headers

On Fedora:

```bash
sudo dnf install cmake gcc-c++ qt5-qtbase-devel
```

## Lenovo diagnostic files

The following files were extracted from Lenovo's Linux diagnostic environment:

```text
ab2f3b8d9c187f4ad3630af080e53326373c27153506cf71de3b08d3bd55c03c  libsal.so

9ceea6f128dfe8208b6df00f21b27e7dc5b75bb6e66d6fa443e78f7f7e2b334b  module_fan.so

a13d1d64c8708a5852584f799a6b251d9c09736ec14ff0367d37a94965dbe0fd  fan_database.sqlite

80c0da6b9c07486967ae08bd5ecd77da5d071de1845560e9fbbf2a0858138393  ldiag_4.64.5_linux.iso
```

The files can be found in the extracted diagnostic filesystem at:

```text
_ldiag_4.64.5_linux.iso.extracted/iso-root/live/_filesystem.squashfs.extracted/squashfs-root/opt/lenovo/ldiag
```

The diagnostic ISO can be obtained from Lenovo's support site:

[Lenovo ThinkPad X260 Diagnostics](https://pcsupport.lenovo.com/gb/en/products/laptops-and-netbooks/thinkpad-x-series-laptops/thinkpad-x260/downloads/ds025448?category=Diagnostic)

The X260 is the model selected on the download page, although the `fan_database.sqlite` bundled with the diagnostic software contains entries for many different Lenovo models, suggesting that the fan module is intended to be shared across multiple ThinkPad generations.

The ISO can be extracted with:

```bash
binwalk -Me ldiag_4.64.5_linux.iso
```

The exact versions of `module_fan.so` and `libsal.so` matter because ModernThinkFan resolves Lenovo's C++ functions using their mangled symbol names. Different versions of the module may therefore require the symbol mappings to be updated.

## Supported devices

A CSV version of Lenovo's fan database is included as [supportedDevices.csv](https://github.com/heapframe/modernthinkfan/blob/master/supportedDevices.csv).

This is provided as a convenient way to check whether your ThinkPad appears in Lenovo's database before running ModernThinkFan.

The CSV is **not required for program execution and is not a substitute**. The actual `fan_database.sqlite` is required because Lenovo's `module_fan.so` queries it at runtime.

## Installation

Clone the repository and enter the project directory:

```bash
git clone <repository-url>
cd lenovofan
```

Install the build dependencies:

```bash
sudo dnf install cmake gcc-c++ qt5-qtbase-devel
```

Make sure the Lenovo runtime files are present:

```text
libsal.so
module_fan.so
fan_database.sqlite
```

Then configure and build:

```bash
cmake -B build
cmake --build build
```

The resulting binary will be:

```text
build/lenovofan
```

For convenience, you can copy it alongside the Lenovo runtime files:

```bash
cp build/lenovofan .
```

The directory should then look approximately like:

```text
lenovofan
├── fan_database.sqlite
├── libsal.so
├── module_fan.so
└── lenovofan
```

## Usage

Display the help menu:

```text
➜  lenovofan git:(master) ✗ sudo ./lenovofan -h

  ./lenovofan {OPTIONS}

    Modern Thinkpad Fan Control

  OPTIONS:

      -h, --help                        Display this help menu

      -r, --read                        Read fan speed

      -s[preset], --set=[preset]        Set Fan Speed [slow/med/high/full/auto]
```

### Read fan speed

```text
➜  lenovofan git:(master) ✗ sudo ./lenovofan -r

battery_manufacturer_date: 2024/10/26
Fan controllable: 1
GetRotationSpeed result: 2
RPM: 0
```

### Set full speed

```text
➜  lenovofan git:(master) ✗ sudo ./lenovofan -s full

battery_manufacturer_date: 2024/10/26
```

The fan can then be checked again:

```text
➜  lenovofan git:(master) ✗ sudo ./lenovofan -r

battery_manufacturer_date: 2024/10/26
Fan controllable: 1
GetRotationSpeed result: 2
RPM: 8064
```

### Return to automatic control

```text
➜  lenovofan git:(master) ✗ sudo ./lenovofan -s auto

battery_manufacturer_date: 2024/10/26
```

Then:

```text
➜  lenovofan git:(master) ✗ sudo ./lenovofan -r

battery_manufacturer_date: 2024/10/26
Fan controllable: 1
GetRotationSpeed result: 2
RPM: 0
```

Available presets are:

```text
slow
med
high
full
auto
```

## Why does `battery_manufacturer_date` appear every time?

This output originates from Lenovo's `module_fan.so`, not ModernThinkFan itself.

It appears to be produced by Lenovo's internal initialisation code when the embedded controller component is constructed. Removing it would require modifying or intercepting Lenovo's module rather than changing the ModernThinkFan code.

## Why won't you post the required libraries and database directly here?

Not too sure on its distribution rules, so I'd rather just play it safe.


## Function hooking

ModernThinkFan does not reimplement Lenovo's EC communication.

Instead, it resolves the required C++ symbols directly from `module_fan.so`:

```text
FanComponentManager::IsFanSpeedControllableModel()
EmbeddedControllerComponentLinux::EmbeddedControllerComponentLinux()
EmbeddedControllerComponentLinux::~EmbeddedControllerComponentLinux()
EmbeddedControllerComponent::GetRotationSpeed()
EmbeddedControllerComponent::SetSlowRotation()
EmbeddedControllerComponent::SetMediumRotation()
EmbeddedControllerComponent::SetHighRotation()
EmbeddedControllerComponent::SetFullRotation()
EmbeddedControllerComponent::SetAutoRotation()
```

The symbol names are C++ mangled names and are therefore tied to the specific `module_fan.so` build being used.

The current implementation was developed against:

```text
module_fan.so
SHA256: 9ceea6f128dfe8208b6df00f21b27e7dc5b75bb6e66d6fa443e78f7f7e2b334b

libsal.so
SHA256: ab2f3b8d9c187f4ad3630af080e53326373c27153506cf71de3b08d3bd55c03c
```

If Lenovo releases a different version of the diagnostic module, the exported symbols and ABI should be checked before using it with ModernThinkFan.

## Root privileges

Root privileges are required because Lenovo's Linux EC implementation performs direct I/O operations against the Embedded Controller.

Running without root will result in:

```text
Root permissions are required as this program works with the EC.
```

ModernThinkFan intentionally refuses to continue without root privileges.

## Warning

This software directly controls the ThinkPad's Embedded Controller through Lenovo's diagnostic implementation.

Incorrect EC commands can potentially cause undesirable hardware behaviour. The predefined fan presets are taken directly from Lenovo's own implementation, but this project is still using that implementation outside of its original diagnostic environment.

Use it at your own risk.
