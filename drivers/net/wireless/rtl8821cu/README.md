在Jetson Nano上直接编译会失败，因为在系统中还做了一些修改。可以clone后，用 sudo make install 直接安装使用。

我刚买了Tenda U9，算比较便宜，在WIN系统上正常，速度还可以。但在JETSON NANO上没有驱动可用，驱动是为x86_64设计的。 直接编译不成功，我用非常笨拙的方法修改了下，编译成功，使用正常。 请参考 https://github.com/MingxuZhang/rtl8821cu 里的安装方法。重点是：

Plug your USB-wifi-adapter into your PC If wifi can be detected, congratulations. 
If not, maybe you need to switch your device usb mode by the following steps in terminal:

find your usb-wifi-adapter device ID, like "0bda:1a2b", by type: lsusb 

switch the mode by type: (the device ID must be yours.) 
sudo usb_modeswitch -KW -v 0bda -p 1a2b It should work.

使用时要切换一下网卡的模式，才可工作。实际上可以用脚本，检测到此设备插入时，自动执行usb_modeswitch。

以下为原信息。

# Realtek RTL8811CU/RTL8821CU USB wifi adapter driver version 5.4.1 for Linux 4.4.x up to 5.1.x


Before build this driver make sure `make`, `gcc`, `linux-header` and `git` have been installed.

## First, clone this repository
```
mkdir -p ~/build
cd ~/build
git clone https://github.com/brektrou/rtl8821CU.git
```
## Build and install with DKMS

DKMS is a system which will automatically recompile and install a kernel module when a new kernel gets installed or updated. To make use of DKMS, install the dkms package.

### Debian/Ubuntu:
```
sudo apt-get install dkms
```
### Arch Linux/Manjaro:
```
sudo pacman -S dkms
```
To make use of the **DKMS** feature with this project, just run:
```
./dkms-install.sh
```
If you later on want to remove it, run:
```
./dkms-remove.sh
```
## Build and install without DKMS
Use following commands:
```
cd ~/build/rtl8821CU
make
sudo make install
```
If you later on want to remove it, do the following:
```
cd ~/build/rtl8821CU
sudo make uninstall
```
## Checking installed driver
If you successfully install the driver, the driver is installed on `/lib/modules/<linux version>/kernel/drivers/net/wireless/realtek/rtl8821cu`. Check the driver with the `ls` command:
```
ls /lib/modules/$(uname -r)/kernel/drivers/net/wireless/realtek/rtl8821cu
```
Make sure `8821cu.ko` file present on that directory

### Check with **DKMS** (if installing via **DKMS**):

``
sudo dkms status
``
