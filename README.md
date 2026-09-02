# ModernThinkFan

ModernThinkFan is a Linux fan-control utility for [supported Lenovo ThinkPads](https://github.com/heapframe/modernthinkfan/blob/master/supportedDevices.csv).

Rather than reimplementing the Embedded Controller (EC) protocol, ModernThinkFan dynamically loads Lenovo's own `module_fan.so` from their Linux diagnostic software and calls its exported functions directly. The actual EC communication and model-specific behaviour are handled entirely by Lenovo's code.

> **Warning:** This software directly controls the ThinkPad's Embedded Controller. Preventing the EC from managing the fan can result in hardware damage during overheating. Always set the fan back to `auto` when you are done. Use at your own risk.

## Usage

```text
$ sudo ./modernthinkfan -h
  ./modernthinkfan {OPTIONS}

    Modern Thinkpad Fan Control

  OPTIONS:

      -h, --help                        Display this help menu
      -r, --read                        Read fan speed
      --stream                          Repeatedly read fan speed
      -s[preset], --set=[preset]        Set Fan Speed [slow/med/high/full/auto]
      -n, --nofan                       Stops the EC from interfering with the
                                        fan speed/sets fanspeed to 0
                                        (experimental)
      --iDontCare                       Ignore EC read failures & Try set fan
                                        speed anyways on unsupported models

    Disclamer:
    Set fan speed to auto to hand control back to the EC.
    Preventing the EC from controlling the fan can result in hardware damage
    during overheating
```

Root privileges are required. The program will refuse to run without them.

### Read fan speed

```text
$ sudo ./modernthinkfan -r
battery_manufacturer_date: 2024/10/26
RPM: 0
```

### Set fan speed

```text
$ sudo ./modernthinkfan -s full
battery_manufacturer_date: 2024/10/26
```

```text
$ sudo ./modernthinkfan -r
battery_manufacturer_date: 2024/10/26
RPM: 8064
```

Set the fan back to automatic control when done:

```text
$ sudo ./modernthinkfan -s auto
```

### Available presets

| Preset | Description |
|--------|-------------|
| `slow` | Lenovo's low-speed preset |
| `med`  | Lenovo's medium-speed preset |
| `high` | Lenovo's high-speed preset |
| `full` | Maximum fan speed |
| `auto` | Hand control back to the EC |

### Stream fan speed

Continuously prints the current RPM with a Unix timestamp, polling every 100ms:

```text
$ sudo ./modernthinkfan --stream
1738000000 | RPM: 3200
1738000000 | RPM: 3200
1738000001 | RPM: 3250
...
```

### Disable the fan (experimental)

The `-n` flag sets the fan speed to 0 and stops the EC from changing it:

```text
$ sudo ./modernthinkfan -n
```

Set the fan speed back to `auto` to hand control back to the EC.

### Bypass unsupported model check

The `--iDontCare` flag ignores EC read failures and allows setting the fan speed on models that are not in Lenovo's fan database:

```text
$ sudo ./modernthinkfan --iDontCare -s full
```

## Installation

### Build dependencies

- CMake (3.16+)
- A C++ compiler with C++17 support
- Qt 5 development headers

On Fedora:

```bash
sudo dnf install cmake gcc-c++ qt5-qtbase-devel
```

### Lenovo runtime files

ModernThinkFan requires three proprietary files extracted from Lenovo's Linux diagnostic environment:

```text
module_fan.so    Lenovo's fan-control library
libsal.so        Supporting Lenovo library
fan_database.sqlite   Model/fan capability database
```

These files must be placed in the same directory as the binary. They are not included in this repository. See [Obtaining the Lenovo files](#obtaining-the-lenovo-files) for extraction instructions.

### Build

```bash
git clone https://github.com/heapframe/modernthinkfan
cd modernthinkfan
cmake -B build
cmake --build build
cp build/modernthinkfan .
```

The working directory should then contain:

```text
modernthinkfan/
├── fan_database.sqlite
├── libsal.so
├── module_fan.so
└── modernthinkfan
```

The program must be run from this directory because the Lenovo module is loaded with `dlopen("./module_fan.so", RTLD_NOW)`.

## Supported devices

ModernThinkFan does not guarantee support for every ThinkPad. Support depends on whether the model appears in Lenovo's `fan_database.sqlite`.

A CSV export of the database is included as [supportedDevices.csv](https://github.com/heapframe/modernthinkfan/blob/master/supportedDevices.csv) for reference. The CSV is **not used at runtime**; the actual `fan_database.sqlite` is required because `module_fan.so` queries it directly.

## How it works

ModernThinkFan loads `module_fan.so` at runtime with `dlopen()` and resolves the required C++ functions by their mangled symbol names using `dlsym()`. It then constructs an `EmbeddedControllerComponentLinux` instance in-place and calls Lenovo's own methods (`SetSlowRotation`, `SetFullRotation`, `GetRotationSpeed`, etc.) to communicate with the EC.

The accompanying `fan_database.sqlite` is queried by Lenovo's code to determine whether a model supports fan-speed control and which control path to use. A Qt `QCoreApplication` is initialised because Lenovo's module uses `QCoreApplication::applicationDirPath()` to locate this database.

### Resolved symbols

The following symbols are resolved from `module_fan.so`:

```text
FanComponentManager::IsFanSpeedControllableModel()
FanComponentManager::IsType3SupportedModel()
EmbeddedControllerComponentLinux::EmbeddedControllerComponentLinux()
EmbeddedControllerComponentLinux::~EmbeddedControllerComponentLinux()
EmbeddedControllerComponent::GetRotationSpeed()
EmbeddedControllerComponent::SetSlowRotation()
EmbeddedControllerComponent::SetMediumRotation()
EmbeddedControllerComponent::SetHighRotation()
EmbeddedControllerComponent::SetFullRotation()
EmbeddedControllerComponent::SetAutoRotation()
EmbeddedControllerComponent::WriteFanControlBit4()
EmbeddedControllerComponent::ReadFanControlStatus()
EmbeddedControllerComponent::WriteToEC()
```

Because these are C++ mangled names, they are tied to a specific build of `module_fan.so`. A different version of the library may export different symbols and would require the mappings to be updated.

## Obtaining the Lenovo files

The required files can be extracted from Lenovo's Linux diagnostic ISO, available from Lenovo's support site:

[Lenovo ThinkPad X260 Diagnostics](https://pcsupport.lenovo.com/gb/en/products/laptops-and-netbooks/thinkpad-x-series-laptops/thinkpad-x260/downloads/ds025448?category=Diagnostic)

The X260 is the model selected on the download page, but the bundled `fan_database.sqlite` contains entries for many ThinkPad generations.

Extract the ISO:

```bash
binwalk -e ldiag_4.64.5_linux.iso
cd _ldiag_4.64.5_linux.iso.extracted/iso-root/live
unsquashfs -d ldiag filesystem.squashfs opt/lenovo/ldiag
cd ldiag/opt/lenovo/ldiag
```

Copy `module_fan.so`, `libsal.so`, and `fan_database.sqlite` into the ModernThinkFan directory.

### Expected file hashes

```text
ab2f3b8d9c187f4ad3630af080e53326373c27153506cf71de3b08d3bd55c03c  libsal.so
9ceea6f128dfe8208b6df00f21b27e7dc5b75bb6e66d6fa443e78f7f7e2b334b  module_fan.so
a13d1d64c8708a5852584f799a6b251d9c09736ec14ff0367d37a94965dbe0fd  fan_database.sqlite
80c0da6b9c07486967ae08bd5ecd77da5d071de1845560e9fbbf2a0858138393  ldiag_4.64.5_linux.iso
```

### Why aren't the Lenovo files included in this repository?

Not too sure on the distribution rules, so I'd rather just play it safe.

## FAQ

### Why does `battery_manufacturer_date` appear in the output?

This comes from Lenovo's `module_fan.so`, not from ModernThinkFan. It is produced by Lenovo's internal initialisation code when the EC component is constructed. Suppressing it would require modifying or intercepting Lenovo's module.

### Why are root privileges required?

Lenovo's EC implementation performs direct I/O operations against the Embedded Controller, which requires root access. ModernThinkFan will refuse to run without it.

## License

[GPL-3.0](LICENSE)
