Write-Host "`n=== Launching QEMU (Windows) ==="

qemu-system-i386 `
    -fda build/Quantum.img `
    -m 64M `
    -monitor stdio `
    -serial none `
    -parallel none

Write-Host "`nQEMU exited."
