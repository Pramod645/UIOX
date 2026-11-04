# UIOX
UIOX as an whole system (Applications + system calls + core functoinality(kernel))

what type of kernel UIOX:



==============
boot command


cd /Users/pramodkumar/Hack/WS/UIOX && qemu-system-aarch64 \
  -M virt -cpu cortex-a57 -m 1G -nographic \
  -kernel 30_KIX/build/arm64/uiox_kernel.elf \
  -append "console=ttyAMA0" 2>&1 | tee boot.log
