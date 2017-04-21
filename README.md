
# Test programme to demonstrate bug in libopencm3 USB packet handling

## Structure of the firmware

The test firmware has the following features:

* It exposes the following interfaces: 1xCDC-ACM and 1xDFU.

* It does the USB initialization and configuration as normal, but once configuration is complete it adds an artificial
  delay between calls to `usbd_poll()`. (Note that the error manifests differently to the PC if the delay is added
  before the USB configuration completes. This is related to the issue 

* It registers a vendor-control callback.

The test script does the following:

1. Resets the device with OpenOCD.

2. Waits for the `/dev/ttyACM0` device node to disappear and reappear.

3. Tries to invoke the vendor control callback. When the Error Conditions are met, this step will fail.

4. Repeats the test.

## Usage

0. On Ubuntu family, install `libnewlib-arm-none-eabi` and `gcc-arm-none-eabi`.

1. `git submodule update --init`

2. `make`

3. Connect an STM32F042 board with OpenOCD. Programme the firmware:
   ```
   > reset halt
   > flash write_image erase /path/to/dio.fw 0x08000000
   ```

4. Keep OpenOCD running in the background and disconnect all telnet sessions. Run `dmesg -w` in the background to
   monitor the PC state. Run `test.sh`.

5. To break-point on the error, or toggle a test GPIO, the relevant line of code for the error condition is
   `libopencm3/lib/usb/usb_control.c:282` - the call to `stall_transaction` inside `_usbd_control_out`.

## Error Conditions

For the error to occur the following conditions must be met:

1. libopencm3 must have the `master` branch checked out. If the commit from PR #777 (`zyp/fix_ctr_rx@f309a6c`) is
   checked out the test will work without error.

2. The device descriptor must be above a critical length. Both the CDC-ACM and DFU interfaces are needed so that that
   device desciptor is lengthy enough to be split into multiple segments.

3. `const int delay` must be set to roughly 64 or greater. If `delay` is set to 16, the test will work without error.
   If the delay value is set somewhere in between, errors will occur sporadically.
