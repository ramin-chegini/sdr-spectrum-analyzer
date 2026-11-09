# آماده‌سازی PetaLinux برای P201PRO

این پوشه، ورودی‌های پروژهٔ قبلی **Ubuntu 18.04 / PetaLinux 2019.2** را
نگهداری می‌کند. نسخهٔ جدید برای تست ساخت در Ubuntu آماده شده؛ هنوز ساخت
ایمیج و بوت این روش جدید تأیید نشده است.

## تغییرات انتقال

- تنظیمات سیستم و rootfs، device tree، تنظیمات کرنل و U-Boot حفظ شده‌اند.
- نسخهٔ Linux ADI و دو کتابخانهٔ `libiio` و `libad9361-iio` روی commit ثابت‌اند.
- طبق تأیید صاحب پروژه، patch دستی برای سورس کرنل نداریم.
- recipe برنامه، سورس فعلی پوشهٔ `sdr_app` همین مخزن را کامپایل می‌کند.
- پوشهٔ کاری تازه ساخته می‌شود و مسیرهای وابسته به سیستم در همان‌جا نوشته می‌شوند.

## ۱. بسته و محیط

کل مخزن را به لینوکس منتقل کن؛ `sdr_app` باید کنار `zynq_sdrBo` باشد.
فایل‌ها و پوشهٔ خروجی روی فایل‌سیستم خود لینوکس باشند و مسیرشان فاصله نداشته باشد.
تنظیمات نصب خودت را فعال کن، مثلاً با `source /path/to/petalinux/2019.2/settings.sh`.

از ریشهٔ مخزن:

```bash
python3 zynq_sdrBo/scripts/check_project.py
python3 -m unittest discover -s zynq_sdrBo/tests -v
```

## ۲. XSA مناسب

برای هماهنگی با طراحی‌ای که تازه بیت‌استریمش ساخته شد، در همان پروژهٔ Vivado
2019.2 یک XSA جدید همراه بیت‌استریم صادر کن. نمونه در Tcl Console:

```tcl
open_run [current_run -implementation]
write_hw_platform -fixed -include_bit /home/rgc/Desktop/p201pro-current.xsa
```

فایل مقصد باید جدید باشد. XSA قبلی هم در `hardware/system.xsa` با checksum
ثبت‌شده نگهداری شده است. برای بازسازی سخت‌افزار قبلی می‌توان آگاهانه از گزینهٔ
`--use-baseline-xsa` استفاده کرد؛ تطابق آن با خروجی FPGA جدید هنوز بررسی نشده است.

## ۳. ساخت پوشهٔ کاری جدید

```bash
python3 zynq_sdrBo/scripts/prepare_project.py \
  --output /home/rgc/work/sdr-clean/zynq_sdrBo \
  --local-sources /home/rgc/work/Analogdevicesinc \
  --xsa /home/rgc/Desktop/p201pro-current.xsa
```

`--output` باید پوشه‌ای جدید باشد؛ پروژهٔ اصلی را به آن نده. مسیر
`--local-sources` باید سه مخزن Git به نام‌های `linux`، `libiio` و
`libad9361-iio` را داشته باشد. از آن‌ها clone جدا با commit ثابت ساخته می‌شود؛
پوشهٔ کاری اصلی آن‌ها تغییر نمی‌کند. فضای دیسک کافی برای cloneها و build لازم است.
اگر این گزینه را حذف کنی، سورس‌ها از مخزن اصلی GitHub دریافت می‌شوند.

اسکریپت پروژه را با ابزار 2019.2 می‌سازد، سخت‌افزار را وارد می‌کند، device tree
سفارشی را برمی‌گرداند و سورس برنامه را در recipe پروژهٔ کاری قرار می‌دهد.
کپی خودکار خروجی به `/tftpboot` در پروژهٔ کاری غیرفعال می‌شود؛ روش بوت SD حفظ می‌شود.

## ۴. ساخت ایمیج

```bash
python3 zynq_sdrBo/scripts/check_project.py \
  --prepared /home/rgc/work/sdr-clean/zynq_sdrBo
cd /home/rgc/work/sdr-clean/zynq_sdrBo
petalinux-build
petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf \
  --fpga images/linux/system.bit --u-boot --force
```

تنظیم فعلی U-Boot فایل‌های `zImage` و `system.dtb` را از پارتیشن اول SD
می‌خواند و rootfs روی `/dev/mmcblk0p2` است. بنابراین روش نصب SD قبلی را با
خروجی‌های جدید ادامه بده؛ صرفاً `image.ub` را جایگزین این روش نکن.

پس از بوت، شناسایی AD9361، دستگاه‌های IIO و UIO، رزرو حافظهٔ DMA، Ethernet،
USB و اجرای برنامه را بررسی می‌کنیم. لاگ آماده‌سازی و اولین خطای build، اگر
رخ داد، برای ادامهٔ رفع اشکال کافی است.

جزئیات نسخه‌ها، تنظیمات و مرز آزمون‌ها در [راهنمای انگلیسی](README.md) آمده است.
