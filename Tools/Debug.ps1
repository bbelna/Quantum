Write-Host "`n=== Launching QEMU (Windows) ==="

qemu-system-i386 `
    -fda ../Build/Quantum.img `
    -m 64M `
    -monitor stdio `
    -serial none `
    -parallel none `
    -no-reboot `
    -no-shutdown `
    -d in_asm,exec,int,mmu,guest_errors,cpu_reset `
    -D qemu.log

Write-Host "`nQEMU exited."
