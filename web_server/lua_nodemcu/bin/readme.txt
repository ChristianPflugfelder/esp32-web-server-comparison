Settings:
LWIP_MAX_SOCKETS = 252
net.c line 1068 > keep_idle is set to 7200000


Flash:
esptool.py --port COM3 erase_flash
esptool.py --chip esp32 --port COM3 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 ./bootloader.bin 0x10000 ./NodeMCU.bin 0x8000 ./partitions.bin