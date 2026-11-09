# پروژهٔ SDR Spectrum Analyzer

این مخزن، طراحی FPGA و برنامهٔ لینوکسی برد **PUZHI PZSDR P201PRO** با
**Zynq-7020 و AD9361** را نگهداری می‌کند.

محیط مبنای توسعه **Ubuntu 18.04، Vivado 2019.2 و PetaLinux 2019.2** است.
ساخت بیت‌استریم نسخهٔ محلی پروژه در تاریخ **۲۰۲۶-۱۱-۰۹** توسط صاحب پروژه
در Ubuntu و Vivado 2019.2 تأیید شد. اجرای کامل برنامه و تست RF این نسخه
در جریان این انتقال دوباره بررسی نشده‌اند.

## ساختار مخزن

| پوشه | محتوا |
| --- | --- |
| [P201Pro](P201Pro/README.md) | بلاک‌دیزاین، IPهای اختصاصی، سورس‌های لازم ADI و اسکریپت بازسازی |
| [sdr_app](sdr_app/README.md) | برنامهٔ ++C برای کنترل AD9361، رجیسترهای FPGA، DMA و ارتباط شبکه |
| [zynq_sdrBo](zynq_sdrBo/README.fa.md) | تنظیمات PetaLinux 2019.2 و آماده‌سازی پروژهٔ کاری تازه |

برنامه‌ای که قبلاً در ریشهٔ مخزن بود، اکنون داخل `sdr_app/` قرار دارد.
ورودی‌های PetaLinux برای تست ساخت در Ubuntu آماده‌اند؛ رابط گرافیکی میزبان در مخزن نیست.

## ساخت FPGA

ابتدا محیط Vivado 2019.2 را با `settings64.sh` مربوط به نصب خودت فعال کن.
سپس از ریشهٔ مخزن اجرا کن:

```bash
cd P201Pro
python3 scripts/check_adi_vendor.py
python3 -m unittest discover -s tests -v
vivado -mode batch -source create_project.tcl \
  -tclargs --output_dir build/full_build --build --jobs 4
```

پوشهٔ خروجی باید جدید یا خالی باشد. پروژهٔ بازسازی‌شده در
`build/full_build/pzp351.xpr` و بیت‌استریم معمولاً در
`build/full_build/pzp351.runs/impl_1/system_top.bit` قرار می‌گیرد.

تمام فایل‌های ADI لازم داخل `P201Pro/third_party/adi_hdl` هستند؛ نیازی به
پوشهٔ بیرونی `Analogdevicesinc` یا دریافت آن از اینترنت نیست. IPها را به
نسخهٔ دیگری ارتقا نده. راهنمای بازکردن پروژه در GUI در
[مستندات سخت‌افزار](P201Pro/README.md) آمده است.

## ساخت برنامه

با GNU Make و کامپایلر `arm-linux-gnueabihf-g++` در Ubuntu:

```bash
cd sdr_app
make
```

خروجی `bin/sdr_app` برای اجرای لینوکس ARM روی برد است. درخت دستگاه،
درایورهای IIO و UIO، آدرس‌های DMA و بیت‌استریم باید با سورس برنامه هماهنگ باشند.
رابط شبکهٔ فعلی `eth0` است و از فایل `include/network_config.h` انتخاب می‌شود.

نمونهٔ دستورها، **روی برد** پس از نصب برنامه:

```bash
sdr_app --help
sdr_app ad9361 status
sdr_app status
sdr_app audio capture 5 /tmp/test.wave
sdr_app audio sinad /tmp/test.wave
```

سورس فعلی، ضبط WAVE و محاسبهٔ SINAD برای تون نزدیک ۱ کیلوهرتز را هم دارد.
جزئیات استقرار، رجیسترها و پروتکل در
[راهنمای برنامه](sdr_app/README.md) توضیح داده شده است.

## آماده‌سازی لینوکس

[راهنمای PetaLinux](zynq_sdrBo/README.fa.md) روش ساخت یک پروژهٔ کاری جدا
با وابستگی‌های ثابت و XSA منتخب را توضیح می‌دهد. recipe برنامه از سورس
`sdr_app` همین مخزن ساخته می‌شود. آزمون‌های آماده‌سازی پاس شده‌اند؛ ساخت
ایمیج و بوت این روش جدید هنوز باید در Ubuntu 18.04 / PetaLinux 2019.2 تأیید شوند.
فایل‌های تولیدشده و کش‌ها در Git نگهداری نمی‌شوند.

مجوزها و اعلان‌های اصلی کدهای ADI و libiio همراه فایل‌های مربوطه حفظ شده‌اند.
