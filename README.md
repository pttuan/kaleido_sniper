# kaleido_sniper
Add Artisan support for Kaleido sniper standard version. Use it as your own risk. I am not responsible for any usage of code in this project.

# HW requirement
- ESP32 WROOM dev kit. The one used for this project: https://www.amazon.com/dp/B07Q9YDS2V?psc=1&ref=ppx_yo2ov_dt_b_product_details
- USB Male to Dupont - 1P: https://www.aliexpress.us/item/3256802826910871.html?spm=a2g0o.order_list.order_list_main.40.4ef91802piKYQh&gatewayAdapt=glo2usa. Order 2 if want to connect Kaleido Pad to ESP32 so Artisan and Kaleido Pad can work at the same time.

# Connect Kaleido to ESP32 Dev Kit
- Connect USB Male to USB Female on Kaleido
- Connect Dupont pin (green) to IO16 of ESP32.
- Connect Dupont pin (white) to IO17 of ESP32.
- Connect Dupont pin (black) to GND of ESP32.
- Connect Dupont pin (red) to 5V of ESP32.

# Connect Kaleido Pad to ESP32 (Optional)
- Connect USB Male to USB Female on Kaleido Pad.
- Connect Dupont pin (green) to IO26 of ESP32.
- Connect Dupont pin (white) to IO25 of ESP32.
- Connect Dupont pin (black) to GND of ESP32.
- Connect Dupont pin (red) to 5V of ESP32.

# Flash ESP32
- Connect ESP32 to computer using provided USB cable.
- Download and install Arduino IDE.
- Open Arduino IDE, go to Tools/Board managers and select to install esp32 boards.
- Set board to esp32/DOIT ESP32 DEVKIT V1 with the one at Amazon above.
- Select Port that Windows assigned to ESP32.
- Select Flash Frequency "80MHz", Upload Speed "921600".
- Select File/New Sketch and copy the code from kaleido.c to the new sketch.
- Select Sketch/Upload to flash the code to ESP32.
- Now you ready to use Artisan.

# Artisan usage
- Download Artisan setting file from https://kaleidoroasters.com/pages/setting-up-your-artisan-enabled-roaster-pro-dual-systems for Dual Systems.
- Open Artisan and load the setting file downloaded above.
- On Windows, open bluetooth manager. You should see device name "Kaleido-M2", connect to it and enter "1234" password if being asked.
- Go to Device manager of Windows, find out which COM that Windows assigned for the bluetooth device. If there are two new COMs added, select the first one.
- On Artisan, after loading the downloaded setting files, open Config/Port. Type COM port such as COM3 to Comm Port, baud rate 115200, byte size 8, parity N, Stopbits 1 then click OK
- Now Artisan ready to connect to Kaleido. Select ON, Artisan should display current BT, ET of Kaleido
- Please ignore those buttons "Power Auto", "Power Manu", "Start heating", "Stop heating" for now.
- The steps for how to roast
        + Click "Start"
        + Slide SV to wanted temperature. Even current SV is the correct one, please slide it to other values and slide back to make Artisan sends
        command to Kaleido.
        + Kaleido should start heating now
        + Slide Roller to wanted value
        + Click on Power buttons such as Power100 during pre-heat
        + After BT near desire temp, drop beans and click "Charge", the power will be reduced to 20 (behavior can be changed in the code).
        + After set back temp event, increase power
        + Change smoke as needed.
        + After done roasting, click "Drop", kaleido will automatically start cooling and drop power (behavior can be changed in the code).
        + Click Cool end to stop cooling fan.